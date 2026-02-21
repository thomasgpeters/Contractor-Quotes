#include "data/ApiDataProvider.h"
#include <Wt/Json/Parser.h>
#include <Wt/Json/Object.h>
#include <Wt/Json/Array.h>
#include <Wt/Json/Value.h>
#include <Wt/Json/Serializer.h>
#include <boost/asio.hpp>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <regex>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <set>
#include <map>

using boost::asio::ip::tcp;

// ── Construction ─────────────────────────────────────────────

ApiDataProvider::ApiDataProvider(const AppConfig& config)
    : baseUrl_(config.apiBaseUrl)
    , timeoutSec_(config.requestTimeout)
    , apiKeyHeader_(config.apiKeyHeader)
    , apiKeyValue_(config.apiKeyValue)
{
}

// ── URL / HTTP helpers ───────────────────────────────────────

void ApiDataProvider::parseUrl(std::string& host, std::string& port,
                               std::string& basePath) const
{
    // Parse "http://host:port/path"
    std::regex re(R"(https?://([^:/]+):?(\d+)?(/.*)?)" );
    std::smatch m;
    if (std::regex_match(baseUrl_, m, re)) {
        host     = m[1].str();
        port     = m[2].matched ? m[2].str() : "80";
        basePath = m[3].matched ? m[3].str() : "";
    } else {
        host     = "localhost";
        port     = "5667";
        basePath = "/api";
    }
    // Remove trailing slash from basePath
    if (!basePath.empty() && basePath.back() == '/')
        basePath.pop_back();
}

std::string ApiDataProvider::httpRequest(const std::string& method,
                                         const std::string& path,
                                         const std::string& body) const
{
    std::string host, port, basePath;
    parseUrl(host, port, basePath);

    std::string fullPath = basePath + path;

    boost::asio::io_context io;
    tcp::resolver resolver(io);
    tcp::socket socket(io);

    auto endpoints = resolver.resolve(host, port);
    boost::asio::connect(socket, endpoints);

    // Build HTTP request
    std::ostringstream req;
    req << method << " " << fullPath << " HTTP/1.1\r\n";
    req << "Host: " << host << ":" << port << "\r\n";
    req << "Accept: application/json\r\n";
    req << "Content-Type: application/json\r\n";

    if (!apiKeyHeader_.empty() && !apiKeyValue_.empty())
        req << apiKeyHeader_ << ": " << apiKeyValue_ << "\r\n";

    if (!body.empty())
        req << "Content-Length: " << body.size() << "\r\n";
    else
        req << "Content-Length: 0\r\n";

    req << "Connection: close\r\n";
    req << "\r\n";

    if (!body.empty())
        req << body;

    std::string reqStr = req.str();
    boost::asio::write(socket, boost::asio::buffer(reqStr));

    // Read response
    boost::asio::streambuf response;
    boost::system::error_code ec;

    // Read headers
    boost::asio::read_until(socket, response, "\r\n\r\n", ec);

    std::istream responseStream(&response);
    std::string statusLine;
    std::getline(responseStream, statusLine);

    // Parse status code
    int statusCode = 0;
    {
        std::regex statusRe(R"(HTTP/\d\.\d\s+(\d+))");
        std::smatch sm;
        if (std::regex_search(statusLine, sm, statusRe))
            statusCode = std::stoi(sm[1].str());
    }

    // Parse headers
    std::string header;
    while (std::getline(responseStream, header) && header != "\r") {
        // consume headers
    }

    // Read body (whatever is already buffered + remainder)
    std::ostringstream bodyStream;
    if (response.size() > 0)
        bodyStream << &response;

    // Read remaining data
    while (boost::asio::read(socket, response,
           boost::asio::transfer_at_least(1), ec)) {
        bodyStream << &response;
    }

    std::string responseBody = bodyStream.str();

    if (statusCode >= 400) {
        std::cerr << "[ApiDataProvider] HTTP " << statusCode
                  << " " << method << " " << fullPath << "\n"
                  << responseBody.substr(0, 500) << "\n";
    }

    return responseBody;
}

std::string ApiDataProvider::httpGet(const std::string& path) const {
    return httpRequest("GET", path);
}

std::string ApiDataProvider::httpPost(const std::string& path, const std::string& jsonBody) const {
    return httpRequest("POST", path, jsonBody);
}

std::string ApiDataProvider::httpPatch(const std::string& path, const std::string& jsonBody) const {
    return httpRequest("PATCH", path, jsonBody);
}

std::string ApiDataProvider::httpDelete(const std::string& path) const {
    return httpRequest("DELETE", path);
}

// ── JSON:API helpers ─────────────────────────────────────────

// ApiLogicServer uses SAFRS / JSON:API format.  A collection response looks like:
//   { "data": [ { "attributes": { ... }, "id": "1", "type": "Product" }, ... ] }
// A single-object response:
//   { "data": { "attributes": { ... }, "id": "1", "type": "Product" } }

// Wt::Json::Value uses implicit conversion operators:
//   std::string s = value;       // operator std::string()
//   double d = value;            // operator double()
//   int i = value;               // operator int()
//   bool b = value;              // operator bool()
//   const Array& a = value;      // operator const Array&()
//   const Object& o = value;     // operator const Object&()
// Use orIfNull() for fallback on null values.

static std::string jsonStr(const Wt::Json::Object& obj, const std::string& key) {
    if (obj.contains(key) && !obj.get(key).isNull())
        return obj.get(key).orIfNull("");
    return "";
}

static double jsonDbl(const Wt::Json::Object& obj, const std::string& key) {
    if (obj.contains(key) && !obj.get(key).isNull()) {
        const auto& v = obj.get(key);
        if (v.type() == Wt::Json::Type::Number)
            return v.orIfNull(0.0);
        // Sometimes numbers come as strings from JSON:API
        std::string s = v.toString().orIfNull("");
        if (!s.empty()) {
            try { return std::stod(s); } catch (...) {}
        }
    }
    return 0.0;
}

static int jsonInt(const Wt::Json::Object& obj, const std::string& key) {
    if (obj.contains(key) && !obj.get(key).isNull()) {
        const auto& v = obj.get(key);
        if (v.type() == Wt::Json::Type::Number)
            return v.orIfNull(0);
        std::string s = v.toString().orIfNull("");
        if (!s.empty()) {
            try { return std::stoi(s); } catch (...) {}
        }
    }
    return 0;
}

static bool jsonBool(const Wt::Json::Object& obj, const std::string& key) {
    if (obj.contains(key) && !obj.get(key).isNull()) {
        const auto& v = obj.get(key);
        if (v.type() == Wt::Json::Type::Bool)
            return v.orIfNull(false);
        if (v.type() == Wt::Json::Type::Number)
            return v.orIfNull(0) != 0;
    }
    return false;
}

static long long jsonId(const Wt::Json::Object& obj) {
    if (obj.contains("id") && !obj.get("id").isNull()) {
        const auto& v = obj.get("id");
        if (v.type() == Wt::Json::Type::Number)
            return v.orIfNull(0LL);
        // JSON:API often sends id as string
        std::string s = v.toString().orIfNull("");
        if (!s.empty()) {
            try { return std::stoll(s); } catch (...) {}
        }
    }
    return 0;
}

static const Wt::Json::Object& getAttr(const Wt::Json::Object& item) {
    return static_cast<const Wt::Json::Object&>(item.get("attributes"));
}

static Wt::Json::Array getDataArray(const std::string& json) {
    Wt::Json::Object root;
    Wt::Json::parse(json, root);
    if (root.contains("data")) {
        const auto& data = root.get("data");
        if (data.type() == Wt::Json::Type::Array) {
            const Wt::Json::Array& arr = data;
            return arr;
        }
    }
    return Wt::Json::Array();
}

static Wt::Json::Object getDataObject(const std::string& json) {
    Wt::Json::Object root;
    Wt::Json::parse(json, root);
    if (root.contains("data")) {
        const auto& data = root.get("data");
        if (data.type() == Wt::Json::Type::Object) {
            const Wt::Json::Object& obj = data;
            return obj;
        }
    }
    return Wt::Json::Object();
}

static int getTotalCount(const std::string& json) {
    Wt::Json::Object root;
    Wt::Json::parse(json, root);
    if (root.contains("meta")) {
        const Wt::Json::Object& meta = root.get("meta");
        if (meta.contains("count"))
            return jsonInt(meta, "count");
        if (meta.contains("total"))
            return jsonInt(meta, "total");
    }
    // Fall back to counting the array
    if (root.contains("data")) {
        const auto& data = root.get("data");
        if (data.type() == Wt::Json::Type::Array) {
            const Wt::Json::Array& arr = data;
            return static_cast<int>(arr.size());
        }
    }
    return 0;
}

// ── Serialization helpers ────────────────────────────────────

static std::string buildJsonBody(const std::string& type,
                                  const Wt::Json::Object& attributes,
                                  long long id = 0)
{
    Wt::Json::Object data;
    data["type"] = Wt::Json::Value(Wt::WString::fromUTF8(type));
    if (id > 0)
        data["id"] = Wt::Json::Value(Wt::WString::fromUTF8(std::to_string(id)));
    data["attributes"] = Wt::Json::Value(attributes);

    Wt::Json::Object root;
    root["data"] = Wt::Json::Value(data);
    return Wt::Json::serialize(root);
}

// Forward declarations for static parse helpers (needed by enrichment methods)
static ProductDTO          parseProduct(const Wt::Json::Object& item);
static SupplierDTO         parseSupplier(const Wt::Json::Object& item);
static SupplierProductDTO  parseSupplierProduct(const Wt::Json::Object& item);
static ClientDTO           parseClient(const Wt::Json::Object& item);
static QuoteDTO            parseQuote(const Wt::Json::Object& item);
static QuoteLineItemDTO    parseLineItem(const Wt::Json::Object& item);

// ── Products ─────────────────────────────────────────────────

static ProductDTO parseProduct(const Wt::Json::Object& item) {
    ProductDTO dto;
    dto.id = jsonId(item);
    const auto& attr = getAttr(item);
    dto.name           = jsonStr(attr, "name");
    dto.category       = jsonStr(attr, "category");
    dto.unit           = jsonStr(attr, "unit");
    dto.specifications = jsonStr(attr, "specifications");
    dto.sku            = jsonStr(attr, "sku");
    return dto;
}

std::vector<ProductDTO> ApiDataProvider::findAllProducts()
{
    auto json = httpGet("/Product/?page[limit]=1000");
    auto arr  = getDataArray(json);

    std::vector<ProductDTO> result;
    for (const auto& v : arr)
        result.push_back(parseProduct(static_cast<const Wt::Json::Object&>(v)));
    enrichProducts(result);
    return result;
}

std::optional<ProductDTO> ApiDataProvider::findProductById(long long id)
{
    try {
        auto json = httpGet("/Product/" + std::to_string(id) + "/");
        auto obj  = getDataObject(json);
        if (obj.empty()) return std::nullopt;
        auto dto = parseProduct(obj);
        std::vector<ProductDTO> v{dto};
        enrichProducts(v);
        return v[0];
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<std::string> ApiDataProvider::getDistinctCategories()
{
    auto products = findAllProducts();
    std::set<std::string> cats;
    for (auto& p : products)
        cats.insert(p.category);
    return {cats.begin(), cats.end()};
}

// ── Suppliers ────────────────────────────────────────────────

static SupplierDTO parseSupplier(const Wt::Json::Object& item) {
    SupplierDTO dto;
    dto.id = jsonId(item);
    const auto& attr = getAttr(item);
    dto.name         = jsonStr(attr, "name");
    dto.address      = jsonStr(attr, "address");
    dto.city         = jsonStr(attr, "city");
    dto.state        = jsonStr(attr, "state");
    dto.zipCode      = jsonStr(attr, "zip_code");
    dto.phone        = jsonStr(attr, "phone");
    dto.email        = jsonStr(attr, "email");
    dto.website      = jsonStr(attr, "website");
    dto.latitude     = jsonDbl(attr, "latitude");
    dto.longitude    = jsonDbl(attr, "longitude");
    dto.rating       = jsonDbl(attr, "rating");
    dto.leadTimeDays = jsonInt(attr, "lead_time_days");
    return dto;
}

std::vector<SupplierDTO> ApiDataProvider::findAllSuppliers()
{
    auto json = httpGet("/Supplier/?page[limit]=1000");
    auto arr  = getDataArray(json);

    std::vector<SupplierDTO> result;
    for (const auto& v : arr)
        result.push_back(parseSupplier(static_cast<const Wt::Json::Object&>(v)));
    enrichSuppliers(result);
    return result;
}

std::optional<SupplierDTO> ApiDataProvider::findSupplierById(long long id)
{
    try {
        auto json = httpGet("/Supplier/" + std::to_string(id) + "/");
        auto obj  = getDataObject(json);
        if (obj.empty()) return std::nullopt;
        auto dto = parseSupplier(obj);
        std::vector<SupplierDTO> v{dto};
        enrichSuppliers(v);
        return v[0];
    } catch (...) {
        return std::nullopt;
    }
}

// ── Supplier-Products ────────────────────────────────────────

static SupplierProductDTO parseSupplierProduct(const Wt::Json::Object& item) {
    SupplierProductDTO dto;
    dto.id = jsonId(item);
    const auto& attr = getAttr(item);
    dto.productId     = jsonInt(attr, "product_id");
    dto.supplierId    = jsonInt(attr, "supplier_id");
    dto.unitPrice     = jsonDbl(attr, "unit_price");
    dto.stockQty      = jsonInt(attr, "stock_qty");
    dto.inStock       = jsonBool(attr, "in_stock");
    dto.canBackorder  = jsonBool(attr, "can_backorder");
    dto.minOrderQty   = jsonInt(attr, "min_order_qty");
    dto.bulkDiscount  = jsonDbl(attr, "bulk_discount");
    dto.bulkThreshold = jsonInt(attr, "bulk_threshold");
    dto.lastUpdated   = jsonStr(attr, "last_updated");
    return dto;
}

std::vector<SupplierProductDTO> ApiDataProvider::findSupplierProductsByProductId(long long productId)
{
    auto json = httpGet("/SupplierProduct/?filter[product_id]="
                        + std::to_string(productId) + "&page[limit]=1000");
    auto arr  = getDataArray(json);

    std::vector<SupplierProductDTO> result;
    for (const auto& v : arr)
        result.push_back(parseSupplierProduct(static_cast<const Wt::Json::Object&>(v)));
    enrichSupplierProducts(result);
    return result;
}

std::vector<SupplierProductDTO> ApiDataProvider::findSupplierProductsBySupplierId(long long supplierId)
{
    auto json = httpGet("/SupplierProduct/?filter[supplier_id]="
                        + std::to_string(supplierId) + "&page[limit]=1000");
    auto arr  = getDataArray(json);

    std::vector<SupplierProductDTO> result;
    for (const auto& v : arr)
        result.push_back(parseSupplierProduct(static_cast<const Wt::Json::Object&>(v)));
    enrichSupplierProducts(result);
    return result;
}

// ── Clients ──────────────────────────────────────────────────

static ClientDTO parseClient(const Wt::Json::Object& item) {
    ClientDTO dto;
    dto.id = jsonId(item);
    const auto& attr = getAttr(item);
    dto.name      = jsonStr(attr, "name");
    dto.company   = jsonStr(attr, "company");
    dto.address   = jsonStr(attr, "address");
    dto.city      = jsonStr(attr, "city");
    dto.state     = jsonStr(attr, "state");
    dto.zipCode   = jsonStr(attr, "zip_code");
    dto.phone     = jsonStr(attr, "phone");
    dto.email     = jsonStr(attr, "email");
    dto.latitude  = jsonDbl(attr, "latitude");
    dto.longitude = jsonDbl(attr, "longitude");
    return dto;
}

static Wt::Json::Object clientToAttributes(const ClientDTO& dto) {
    Wt::Json::Object attr;
    attr["name"]      = Wt::Json::Value(Wt::WString::fromUTF8(dto.name));
    attr["company"]   = Wt::Json::Value(Wt::WString::fromUTF8(dto.company));
    attr["address"]   = Wt::Json::Value(Wt::WString::fromUTF8(dto.address));
    attr["city"]      = Wt::Json::Value(Wt::WString::fromUTF8(dto.city));
    attr["state"]     = Wt::Json::Value(Wt::WString::fromUTF8(dto.state));
    attr["zip_code"]  = Wt::Json::Value(Wt::WString::fromUTF8(dto.zipCode));
    attr["phone"]     = Wt::Json::Value(Wt::WString::fromUTF8(dto.phone));
    attr["email"]     = Wt::Json::Value(Wt::WString::fromUTF8(dto.email));
    attr["latitude"]  = Wt::Json::Value(dto.latitude);
    attr["longitude"] = Wt::Json::Value(dto.longitude);
    return attr;
}

std::vector<ClientDTO> ApiDataProvider::findAllClients()
{
    auto json = httpGet("/Client/?page[limit]=1000");
    auto arr  = getDataArray(json);

    std::vector<ClientDTO> result;
    for (const auto& v : arr)
        result.push_back(parseClient(static_cast<const Wt::Json::Object&>(v)));
    enrichClients(result);
    return result;
}

std::optional<ClientDTO> ApiDataProvider::findClientById(long long id)
{
    try {
        auto json = httpGet("/Client/" + std::to_string(id) + "/");
        auto obj  = getDataObject(json);
        if (obj.empty()) return std::nullopt;
        auto dto = parseClient(obj);
        std::vector<ClientDTO> v{dto};
        enrichClients(v);
        return v[0];
    } catch (...) {
        return std::nullopt;
    }
}

ClientDTO ApiDataProvider::createClient(const ClientDTO& dto)
{
    auto body = buildJsonBody("Client", clientToAttributes(dto));
    auto json = httpPost("/Client/", body);
    auto obj  = getDataObject(json);
    auto result = parseClient(obj);
    return result;
}

ClientDTO ApiDataProvider::updateClient(long long id, const ClientDTO& dto)
{
    auto body = buildJsonBody("Client", clientToAttributes(dto), id);
    auto json = httpPatch("/Client/" + std::to_string(id) + "/", body);
    auto obj  = getDataObject(json);
    return parseClient(obj);
}

void ApiDataProvider::deleteClient(long long id)
{
    httpDelete("/Client/" + std::to_string(id) + "/");
}

// ── Quotes ───────────────────────────────────────────────────

static QuoteDTO parseQuote(const Wt::Json::Object& item) {
    QuoteDTO dto;
    dto.id = jsonId(item);
    const auto& attr = getAttr(item);
    dto.clientId    = jsonInt(attr, "client_id");
    dto.title       = jsonStr(attr, "title");
    dto.description = jsonStr(attr, "description");
    dto.createdDate = jsonStr(attr, "created_date");
    dto.expiryDate  = jsonStr(attr, "expiry_date");
    dto.status      = jsonInt(attr, "status");
    dto.taxRate     = jsonDbl(attr, "tax_rate");
    dto.markupRate  = jsonDbl(attr, "markup_rate");
    dto.notes       = jsonStr(attr, "notes");
    return dto;
}

static Wt::Json::Object quoteToAttributes(const QuoteDTO& dto) {
    Wt::Json::Object attr;
    attr["title"]       = Wt::Json::Value(Wt::WString::fromUTF8(dto.title));
    attr["description"] = Wt::Json::Value(Wt::WString::fromUTF8(dto.description));
    attr["status"]      = Wt::Json::Value(dto.status);
    attr["tax_rate"]    = Wt::Json::Value(dto.taxRate);
    attr["markup_rate"] = Wt::Json::Value(dto.markupRate);
    attr["notes"]       = Wt::Json::Value(Wt::WString::fromUTF8(dto.notes));
    if (dto.clientId > 0)
        attr["client_id"] = Wt::Json::Value(static_cast<int>(dto.clientId));
    if (!dto.createdDate.empty())
        attr["created_date"] = Wt::Json::Value(Wt::WString::fromUTF8(dto.createdDate));
    if (!dto.expiryDate.empty())
        attr["expiry_date"]  = Wt::Json::Value(Wt::WString::fromUTF8(dto.expiryDate));
    return attr;
}

// ── Enrichment helpers (raw HTTP to avoid cascading enrichment) ──

void ApiDataProvider::enrichProducts(std::vector<ProductDTO>& products)
{
    if (products.empty()) return;

    auto json = httpGet("/SupplierProduct/?page[limit]=5000");
    auto arr  = getDataArray(json);

    std::map<long long, std::vector<double>> pricesByProduct;
    for (const auto& v : arr) {
        auto sp = parseSupplierProduct(static_cast<const Wt::Json::Object&>(v));
        pricesByProduct[sp.productId].push_back(sp.unitPrice);
    }

    for (auto& p : products) {
        auto it = pricesByProduct.find(p.id);
        if (it != pricesByProduct.end()) {
            p.supplierCount = static_cast<int>(it->second.size());
            p.minPrice = *std::min_element(it->second.begin(), it->second.end());
            p.maxPrice = *std::max_element(it->second.begin(), it->second.end());
        }
    }
}

void ApiDataProvider::enrichSuppliers(std::vector<SupplierDTO>& suppliers)
{
    if (suppliers.empty()) return;

    auto json = httpGet("/SupplierProduct/?page[limit]=5000");
    auto arr  = getDataArray(json);

    std::map<long long, int> countBySupplier;
    for (const auto& v : arr) {
        auto sp = parseSupplierProduct(static_cast<const Wt::Json::Object&>(v));
        countBySupplier[sp.supplierId]++;
    }

    for (auto& s : suppliers) {
        auto it = countBySupplier.find(s.id);
        if (it != countBySupplier.end())
            s.productCount = it->second;
    }
}

void ApiDataProvider::enrichClients(std::vector<ClientDTO>& clients)
{
    if (clients.empty()) return;

    auto json = httpGet("/Quote/?page[limit]=5000");
    auto arr  = getDataArray(json);

    std::map<long long, int> countByClient;
    for (const auto& v : arr) {
        auto q = parseQuote(static_cast<const Wt::Json::Object&>(v));
        if (q.clientId > 0)
            countByClient[q.clientId]++;
    }

    for (auto& c : clients) {
        auto it = countByClient.find(c.id);
        if (it != countByClient.end())
            c.quoteCount = it->second;
    }
}

void ApiDataProvider::enrichSupplierProducts(std::vector<SupplierProductDTO>& sps)
{
    if (sps.empty()) return;

    // Fetch raw products
    auto pJson = httpGet("/Product/?page[limit]=5000");
    auto pArr  = getDataArray(pJson);
    std::map<long long, ProductDTO> prodMap;
    for (const auto& v : pArr) {
        auto p = parseProduct(static_cast<const Wt::Json::Object&>(v));
        prodMap[p.id] = p;
    }

    // Fetch raw suppliers
    auto sJson = httpGet("/Supplier/?page[limit]=5000");
    auto sArr  = getDataArray(sJson);
    std::map<long long, SupplierDTO> suppMap;
    for (const auto& v : sArr) {
        auto s = parseSupplier(static_cast<const Wt::Json::Object&>(v));
        suppMap[s.id] = s;
    }

    for (auto& sp : sps) {
        auto pit = prodMap.find(sp.productId);
        if (pit != prodMap.end()) {
            sp.productName     = pit->second.name;
            sp.productSku      = pit->second.sku;
            sp.productCategory = pit->second.category;
        }
        auto sit = suppMap.find(sp.supplierId);
        if (sit != suppMap.end()) {
            sp.supplierName     = sit->second.name;
            sp.supplierRating   = sit->second.rating;
            sp.supplierLeadDays = sit->second.leadTimeDays;
            sp.supplierLat      = sit->second.latitude;
            sp.supplierLon      = sit->second.longitude;
        }
    }
}

void ApiDataProvider::enrichQuotes(std::vector<QuoteDTO>& quotes)
{
    if (quotes.empty()) return;

    // Batch-resolve client names (raw fetch, no cascade)
    auto cJson = httpGet("/Client/?page[limit]=5000");
    auto cArr  = getDataArray(cJson);
    std::map<long long, std::string> clientNames;
    for (const auto& v : cArr) {
        auto c = parseClient(static_cast<const Wt::Json::Object&>(v));
        clientNames[c.id] = c.name;
    }

    // Fetch ALL line items at once (avoids N+1 per quote)
    auto liJson = httpGet("/QuoteLineItem/?page[limit]=5000");
    auto liArr  = getDataArray(liJson);
    std::map<long long, std::pair<int, double>> quoteLineTotals;
    for (const auto& v : liArr) {
        auto li = parseLineItem(static_cast<const Wt::Json::Object&>(v));
        auto& [count, total] = quoteLineTotals[li.quoteId];
        ++count;
        total += li.lineTotal;
    }

    for (auto& q : quotes) {
        if (q.clientId > 0) {
            auto it = clientNames.find(q.clientId);
            if (it != clientNames.end())
                q.clientName = it->second;
        }
        auto it = quoteLineTotals.find(q.id);
        if (it != quoteLineTotals.end()) {
            q.lineItemCount = it->second.first;
            q.totalAmount   = it->second.second;
        }
    }
}

void ApiDataProvider::enrichLineItems(std::vector<QuoteLineItemDTO>& items)
{
    if (items.empty()) return;

    // Fetch raw products
    auto pJson = httpGet("/Product/?page[limit]=5000");
    auto pArr  = getDataArray(pJson);
    std::map<long long, ProductDTO> prodMap;
    for (const auto& v : pArr) {
        auto p = parseProduct(static_cast<const Wt::Json::Object&>(v));
        prodMap[p.id] = p;
    }

    // Fetch raw suppliers
    auto sJson = httpGet("/Supplier/?page[limit]=5000");
    auto sArr  = getDataArray(sJson);
    std::map<long long, std::string> suppNames;
    for (const auto& v : sArr) {
        auto s = parseSupplier(static_cast<const Wt::Json::Object&>(v));
        suppNames[s.id] = s.name;
    }

    for (auto& li : items) {
        auto pit = prodMap.find(li.productId);
        if (pit != prodMap.end()) {
            li.productName = pit->second.name;
            li.productUnit = pit->second.unit;
        }
        auto sit = suppNames.find(li.supplierId);
        if (sit != suppNames.end())
            li.supplierName = sit->second;
    }
}

std::vector<QuoteDTO> ApiDataProvider::findAllQuotes()
{
    auto json = httpGet("/Quote/?page[limit]=1000&sort=-created_date");
    auto arr  = getDataArray(json);

    std::vector<QuoteDTO> result;
    for (const auto& v : arr)
        result.push_back(parseQuote(static_cast<const Wt::Json::Object&>(v)));
    enrichQuotes(result);
    return result;
}

std::optional<QuoteDTO> ApiDataProvider::findQuoteById(long long id)
{
    try {
        auto json = httpGet("/Quote/" + std::to_string(id) + "/");
        auto obj  = getDataObject(json);
        if (obj.empty()) return std::nullopt;
        auto dto = parseQuote(obj);

        // Enrich single quote
        std::vector<QuoteDTO> v{dto};
        enrichQuotes(v);
        return v[0];
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<QuoteDTO> ApiDataProvider::findRecentQuotes(int limit)
{
    auto json = httpGet("/Quote/?page[limit]=" + std::to_string(limit) +
                        "&sort=-created_date");
    auto arr  = getDataArray(json);

    std::vector<QuoteDTO> result;
    for (const auto& v : arr)
        result.push_back(parseQuote(static_cast<const Wt::Json::Object&>(v)));
    enrichQuotes(result);
    return result;
}

QuoteDTO ApiDataProvider::createQuote(const QuoteDTO& dto)
{
    QuoteDTO toCreate = dto;
    if (toCreate.createdDate.empty()) {
        // Ensure created_date is set for new quotes
        auto now = std::chrono::system_clock::now();
        auto tt  = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_r(&tt, &tm);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
        toCreate.createdDate = buf;
    }
    auto body = buildJsonBody("Quote", quoteToAttributes(toCreate));
    auto json = httpPost("/Quote/", body);
    auto obj  = getDataObject(json);
    return parseQuote(obj);
}

QuoteDTO ApiDataProvider::updateQuote(long long id, const QuoteDTO& dto)
{
    auto body = buildJsonBody("Quote", quoteToAttributes(dto), id);
    auto json = httpPatch("/Quote/" + std::to_string(id) + "/", body);
    auto obj  = getDataObject(json);
    return parseQuote(obj);
}

void ApiDataProvider::deleteQuote(long long id)
{
    // Delete line items first
    auto items = findLineItemsByQuoteId(id);
    for (auto& li : items)
        deleteLineItem(li.id);

    httpDelete("/Quote/" + std::to_string(id) + "/");
}

// ── Quote Line Items ─────────────────────────────────────────

static QuoteLineItemDTO parseLineItem(const Wt::Json::Object& item) {
    QuoteLineItemDTO dto;
    dto.id = jsonId(item);
    const auto& attr = getAttr(item);
    dto.quoteId    = jsonInt(attr, "quote_id");
    dto.productId  = jsonInt(attr, "product_id");
    dto.supplierId = jsonInt(attr, "supplier_id");
    dto.quantity   = jsonInt(attr, "quantity");
    dto.unitPrice  = jsonDbl(attr, "unit_price");
    dto.markup     = jsonDbl(attr, "markup");
    dto.lineTotal  = jsonDbl(attr, "line_total");
    dto.notes      = jsonStr(attr, "notes");
    return dto;
}

static Wt::Json::Object lineItemToAttributes(const QuoteLineItemDTO& dto) {
    Wt::Json::Object attr;
    attr["quote_id"]   = Wt::Json::Value(static_cast<int>(dto.quoteId));
    attr["quantity"]    = Wt::Json::Value(dto.quantity);
    attr["unit_price"]  = Wt::Json::Value(dto.unitPrice);
    attr["markup"]      = Wt::Json::Value(dto.markup);
    attr["line_total"]  = Wt::Json::Value(dto.lineTotal);
    attr["notes"]       = Wt::Json::Value(Wt::WString::fromUTF8(dto.notes));
    if (dto.productId > 0)
        attr["product_id"]  = Wt::Json::Value(static_cast<int>(dto.productId));
    if (dto.supplierId > 0)
        attr["supplier_id"] = Wt::Json::Value(static_cast<int>(dto.supplierId));
    return attr;
}

std::vector<QuoteLineItemDTO> ApiDataProvider::findLineItemsByQuoteId(long long quoteId)
{
    auto json = httpGet("/QuoteLineItem/?filter[quote_id]="
                        + std::to_string(quoteId) + "&page[limit]=1000");
    auto arr  = getDataArray(json);

    std::vector<QuoteLineItemDTO> result;
    for (const auto& v : arr)
        result.push_back(parseLineItem(static_cast<const Wt::Json::Object&>(v)));
    enrichLineItems(result);
    return result;
}

std::optional<QuoteLineItemDTO> ApiDataProvider::findLineItemById(long long id)
{
    try {
        auto json = httpGet("/QuoteLineItem/" + std::to_string(id) + "/");
        auto obj  = getDataObject(json);
        if (obj.empty()) return std::nullopt;
        auto dto = parseLineItem(obj);
        std::vector<QuoteLineItemDTO> v{dto};
        enrichLineItems(v);
        return v[0];
    } catch (...) {
        return std::nullopt;
    }
}

QuoteLineItemDTO ApiDataProvider::createLineItem(const QuoteLineItemDTO& dto)
{
    // Compute line total before sending
    QuoteLineItemDTO out = dto;
    out.lineTotal = dto.quantity * dto.unitPrice * (1.0 + dto.markup / 100.0);

    auto body = buildJsonBody("QuoteLineItem", lineItemToAttributes(out));
    auto json = httpPost("/QuoteLineItem/", body);
    auto obj  = getDataObject(json);
    return parseLineItem(obj);
}

QuoteLineItemDTO ApiDataProvider::updateLineItem(long long id, const QuoteLineItemDTO& dto)
{
    QuoteLineItemDTO out = dto;
    out.lineTotal = dto.quantity * dto.unitPrice * (1.0 + dto.markup / 100.0);

    auto body = buildJsonBody("QuoteLineItem", lineItemToAttributes(out), id);
    auto json = httpPatch("/QuoteLineItem/" + std::to_string(id) + "/", body);
    auto obj  = getDataObject(json);
    return parseLineItem(obj);
}

void ApiDataProvider::deleteLineItem(long long id)
{
    httpDelete("/QuoteLineItem/" + std::to_string(id) + "/");
}

// ── Aggregates ───────────────────────────────────────────────

int ApiDataProvider::getProductCount()
{
    auto json = httpGet("/Product/?page[limit]=0");
    return getTotalCount(json);
}

int ApiDataProvider::getSupplierCount()
{
    auto json = httpGet("/Supplier/?page[limit]=0");
    return getTotalCount(json);
}

int ApiDataProvider::getClientCount()
{
    auto json = httpGet("/Client/?page[limit]=0");
    return getTotalCount(json);
}

int ApiDataProvider::getQuoteCount()
{
    auto json = httpGet("/Quote/?page[limit]=0");
    return getTotalCount(json);
}

std::vector<CategoryStatDTO> ApiDataProvider::getCategoryStats()
{
    // No aggregate endpoint; compute client-side from products
    auto products = findAllProducts();
    std::map<std::string, int> counts;
    for (auto& p : products)
        counts[p.category]++;

    std::vector<CategoryStatDTO> result;
    for (auto& [cat, cnt] : counts) {
        CategoryStatDTO s;
        s.category     = cat;
        s.productCount = cnt;
        result.push_back(std::move(s));
    }
    return result;
}

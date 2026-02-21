#include "data/LocalDataProvider.h"
#include "models/Session.h"
#include "models/Product.h"
#include "models/Supplier.h"
#include "models/SupplierProduct.h"
#include "models/Client.h"
#include "models/Quote.h"
#include "models/QuoteLineItem.h"
#include <Wt/Dbo/Transaction.h>
#include <Wt/WDateTime.h>

LocalDataProvider::LocalDataProvider(Session& session)
    : session_(session)
{
}

// ── Products ─────────────────────────────────────────────────

std::vector<ProductDTO> LocalDataProvider::findAllProducts()
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<Product>().orderBy("category, name").resultList();

    std::vector<ProductDTO> result;
    for (auto& p : rows) {
        ProductDTO dto;
        dto.id             = p.id();
        dto.name           = p->name;
        dto.category       = p->category;
        dto.unit           = p->unit;
        dto.specifications = p->specifications;
        dto.sku            = p->sku;

        double mn = 1e18, mx = 0;
        int cnt = 0;
        for (const auto& sp : p->supplierProducts) {
            ++cnt;
            if (sp->unitPrice < mn) mn = sp->unitPrice;
            if (sp->unitPrice > mx) mx = sp->unitPrice;
        }
        dto.supplierCount = cnt;
        dto.minPrice = (cnt > 0) ? mn : 0;
        dto.maxPrice = mx;

        result.push_back(std::move(dto));
    }
    t.commit();
    return result;
}

std::optional<ProductDTO> LocalDataProvider::findProductById(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto p = session_.dbo().find<Product>().where("id = ?").bind(id).resultValue();
    if (!p) return std::nullopt;

    ProductDTO dto;
    dto.id             = p.id();
    dto.name           = p->name;
    dto.category       = p->category;
    dto.unit           = p->unit;
    dto.specifications = p->specifications;
    dto.sku            = p->sku;
    t.commit();
    return dto;
}

std::vector<std::string> LocalDataProvider::getDistinctCategories()
{
    Wt::Dbo::Transaction t(session_.dbo());
    using CatRow = std::tuple<std::string>;
    auto rows = session_.dbo().query<CatRow>(
        "select distinct category from product order by category").resultList();

    std::vector<std::string> result;
    for (auto& r : rows)
        result.push_back(std::get<0>(r));
    t.commit();
    return result;
}

// ── Suppliers ────────────────────────────────────────────────

std::vector<SupplierDTO> LocalDataProvider::findAllSuppliers()
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<Supplier>().orderBy("name").resultList();

    std::vector<SupplierDTO> result;
    for (auto& s : rows) {
        SupplierDTO dto;
        dto.id           = s.id();
        dto.name         = s->name;
        dto.address      = s->address;
        dto.city         = s->city;
        dto.state        = s->state;
        dto.zipCode      = s->zipCode;
        dto.phone        = s->phone;
        dto.email        = s->email;
        dto.website      = s->website;
        dto.latitude     = s->latitude;
        dto.longitude    = s->longitude;
        dto.rating       = s->rating;
        dto.leadTimeDays = s->leadTimeDays;
        dto.productCount = static_cast<int>(s->supplierProducts.size());
        result.push_back(std::move(dto));
    }
    t.commit();
    return result;
}

std::optional<SupplierDTO> LocalDataProvider::findSupplierById(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto s = session_.dbo().find<Supplier>().where("id = ?").bind(id).resultValue();
    if (!s) return std::nullopt;

    SupplierDTO dto;
    dto.id           = s.id();
    dto.name         = s->name;
    dto.address      = s->address;
    dto.city         = s->city;
    dto.state        = s->state;
    dto.zipCode      = s->zipCode;
    dto.phone        = s->phone;
    dto.email        = s->email;
    dto.website      = s->website;
    dto.latitude     = s->latitude;
    dto.longitude    = s->longitude;
    dto.rating       = s->rating;
    dto.leadTimeDays = s->leadTimeDays;
    dto.productCount = static_cast<int>(s->supplierProducts.size());
    t.commit();
    return dto;
}

// ── Supplier-Products ────────────────────────────────────────

std::vector<SupplierProductDTO> LocalDataProvider::findSupplierProductsByProductId(long long productId)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<SupplierProduct>()
        .where("product_id = ?").bind(productId).resultList();

    std::vector<SupplierProductDTO> result;
    for (auto& sp : rows) {
        SupplierProductDTO dto;
        dto.id             = sp.id();
        dto.productId      = sp->product.id();
        dto.supplierId     = sp->supplier.id();
        dto.unitPrice      = sp->unitPrice;
        dto.stockQty       = sp->stockQty;
        dto.inStock        = sp->inStock;
        dto.canBackorder   = sp->canBackorder;
        dto.minOrderQty    = sp->minOrderQty;
        dto.bulkDiscount   = sp->bulkDiscount;
        dto.bulkThreshold  = sp->bulkThreshold;
        if (sp->lastUpdated.isValid())
            dto.lastUpdated = sp->lastUpdated.toString("yyyy-MM-ddTHH:mm:ss").toUTF8();

        dto.productName     = sp->product->name;
        dto.productSku      = sp->product->sku;
        dto.productCategory = sp->product->category;
        dto.supplierName    = sp->supplier->name;
        dto.supplierRating  = sp->supplier->rating;
        dto.supplierLeadDays= sp->supplier->leadTimeDays;
        dto.supplierLat     = sp->supplier->latitude;
        dto.supplierLon     = sp->supplier->longitude;
        result.push_back(std::move(dto));
    }
    t.commit();
    return result;
}

std::vector<SupplierProductDTO> LocalDataProvider::findSupplierProductsBySupplierId(long long supplierId)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<SupplierProduct>()
        .where("supplier_id = ?").bind(supplierId).resultList();

    std::vector<SupplierProductDTO> result;
    for (auto& sp : rows) {
        SupplierProductDTO dto;
        dto.id             = sp.id();
        dto.productId      = sp->product.id();
        dto.supplierId     = sp->supplier.id();
        dto.unitPrice      = sp->unitPrice;
        dto.stockQty       = sp->stockQty;
        dto.inStock        = sp->inStock;
        dto.canBackorder   = sp->canBackorder;
        dto.minOrderQty    = sp->minOrderQty;
        dto.bulkDiscount   = sp->bulkDiscount;
        dto.bulkThreshold  = sp->bulkThreshold;
        if (sp->lastUpdated.isValid())
            dto.lastUpdated = sp->lastUpdated.toString("yyyy-MM-ddTHH:mm:ss").toUTF8();

        dto.productName     = sp->product->name;
        dto.productSku      = sp->product->sku;
        dto.productCategory = sp->product->category;
        dto.supplierName    = sp->supplier->name;
        dto.supplierRating  = sp->supplier->rating;
        dto.supplierLeadDays= sp->supplier->leadTimeDays;
        dto.supplierLat     = sp->supplier->latitude;
        dto.supplierLon     = sp->supplier->longitude;
        result.push_back(std::move(dto));
    }
    t.commit();
    return result;
}

// ── Clients ──────────────────────────────────────────────────

std::vector<ClientDTO> LocalDataProvider::findAllClients()
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<Client>().orderBy("name").resultList();

    std::vector<ClientDTO> result;
    for (auto& c : rows) {
        ClientDTO dto;
        dto.id         = c.id();
        dto.name       = c->name;
        dto.company    = c->company;
        dto.address    = c->address;
        dto.city       = c->city;
        dto.state      = c->state;
        dto.zipCode    = c->zipCode;
        dto.phone      = c->phone;
        dto.email      = c->email;
        dto.latitude   = c->latitude;
        dto.longitude  = c->longitude;
        dto.quoteCount = static_cast<int>(c->quotes.size());
        result.push_back(std::move(dto));
    }
    t.commit();
    return result;
}

std::optional<ClientDTO> LocalDataProvider::findClientById(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto c = session_.dbo().find<Client>().where("id = ?").bind(id).resultValue();
    if (!c) return std::nullopt;

    ClientDTO dto;
    dto.id        = c.id();
    dto.name      = c->name;
    dto.company   = c->company;
    dto.address   = c->address;
    dto.city      = c->city;
    dto.state     = c->state;
    dto.zipCode   = c->zipCode;
    dto.phone     = c->phone;
    dto.email     = c->email;
    dto.latitude  = c->latitude;
    dto.longitude = c->longitude;
    t.commit();
    return dto;
}

ClientDTO LocalDataProvider::createClient(const ClientDTO& dto)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto c = session_.dbo().addNew<Client>();
    c.modify()->name      = dto.name;
    c.modify()->company   = dto.company;
    c.modify()->address   = dto.address;
    c.modify()->city      = dto.city;
    c.modify()->state     = dto.state;
    c.modify()->zipCode   = dto.zipCode;
    c.modify()->phone     = dto.phone;
    c.modify()->email     = dto.email;
    c.modify()->latitude  = dto.latitude;
    c.modify()->longitude = dto.longitude;
    t.commit();

    ClientDTO out = dto;
    out.id = c.id();
    return out;
}

ClientDTO LocalDataProvider::updateClient(long long id, const ClientDTO& dto)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto c = session_.dbo().find<Client>().where("id = ?").bind(id).resultValue();
    c.modify()->name      = dto.name;
    c.modify()->company   = dto.company;
    c.modify()->address   = dto.address;
    c.modify()->city      = dto.city;
    c.modify()->state     = dto.state;
    c.modify()->zipCode   = dto.zipCode;
    c.modify()->phone     = dto.phone;
    c.modify()->email     = dto.email;
    c.modify()->latitude  = dto.latitude;
    c.modify()->longitude = dto.longitude;
    t.commit();

    ClientDTO out = dto;
    out.id = id;
    return out;
}

void LocalDataProvider::deleteClient(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    session_.dbo().execute("DELETE FROM quote_line_item WHERE quote_id IN "
                           "(SELECT id FROM quote WHERE client_id = ?)").bind(id);
    session_.dbo().execute("DELETE FROM quote WHERE client_id = ?").bind(id);
    session_.dbo().execute("DELETE FROM client WHERE id = ?").bind(id);
    t.commit();
}

// ── Quotes ───────────────────────────────────────────────────

static QuoteDTO quoteToDTO(const Wt::Dbo::ptr<Quote>& q) {
    QuoteDTO dto;
    dto.id          = q.id();
    dto.title       = q->title;
    dto.description = q->description;
    dto.status      = static_cast<int>(q->status);
    dto.taxRate     = q->taxRate;
    dto.markupRate  = q->markupRate;
    dto.notes       = q->notes;

    if (q->createdDate.isValid())
        dto.createdDate = q->createdDate.toString("yyyy-MM-ddTHH:mm:ss").toUTF8();
    if (q->expiryDate.isValid())
        dto.expiryDate = q->expiryDate.toString("yyyy-MM-ddTHH:mm:ss").toUTF8();

    if (q->client) {
        dto.clientId   = q->client.id();
        dto.clientName = q->client->name;
    }

    double total = 0;
    int count = 0;
    for (const auto& li : q->lineItems) {
        total += li->lineTotal;
        ++count;
    }
    dto.totalAmount   = total;
    dto.lineItemCount = count;
    return dto;
}

std::vector<QuoteDTO> LocalDataProvider::findAllQuotes()
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<Quote>().orderBy("created_date desc").resultList();

    std::vector<QuoteDTO> result;
    for (auto& q : rows)
        result.push_back(quoteToDTO(q));
    t.commit();
    return result;
}

std::optional<QuoteDTO> LocalDataProvider::findQuoteById(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto q = session_.dbo().find<Quote>().where("id = ?").bind(id).resultValue();
    if (!q) return std::nullopt;
    auto dto = quoteToDTO(q);
    t.commit();
    return dto;
}

std::vector<QuoteDTO> LocalDataProvider::findRecentQuotes(int limit)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<Quote>()
        .orderBy("created_date desc").limit(limit).resultList();

    std::vector<QuoteDTO> result;
    for (auto& q : rows)
        result.push_back(quoteToDTO(q));
    t.commit();
    return result;
}

QuoteDTO LocalDataProvider::createQuote(const QuoteDTO& dto)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto q = session_.dbo().addNew<Quote>();
    q.modify()->title       = dto.title;
    q.modify()->description = dto.description;
    q.modify()->status      = static_cast<Quote::Status>(dto.status);
    q.modify()->taxRate     = dto.taxRate;
    q.modify()->markupRate  = dto.markupRate;
    q.modify()->notes       = dto.notes;
    q.modify()->createdDate = Wt::WDateTime::currentDateTime();

    if (dto.clientId > 0) {
        auto client = session_.dbo().find<Client>()
            .where("id = ?").bind(dto.clientId).resultValue();
        if (client)
            q.modify()->client = client;
    }
    t.commit();

    QuoteDTO out = dto;
    out.id = q.id();
    return out;
}

QuoteDTO LocalDataProvider::updateQuote(long long id, const QuoteDTO& dto)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto q = session_.dbo().find<Quote>().where("id = ?").bind(id).resultValue();
    q.modify()->title       = dto.title;
    q.modify()->description = dto.description;
    q.modify()->status      = static_cast<Quote::Status>(dto.status);
    q.modify()->taxRate     = dto.taxRate;
    q.modify()->markupRate  = dto.markupRate;
    q.modify()->notes       = dto.notes;

    if (!dto.expiryDate.empty())
        q.modify()->expiryDate = Wt::WDateTime::fromString(
            Wt::WString::fromUTF8(dto.expiryDate), "yyyy-MM-ddTHH:mm:ss");

    if (dto.clientId > 0) {
        auto client = session_.dbo().find<Client>()
            .where("id = ?").bind(dto.clientId).resultValue();
        if (client)
            q.modify()->client = client;
    }
    t.commit();

    QuoteDTO out = dto;
    out.id = id;
    return out;
}

void LocalDataProvider::deleteQuote(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto items = session_.dbo().find<QuoteLineItem>()
        .where("quote_id = ?").bind(id).resultList();
    for (auto& li : items)
        li.remove();

    auto q = session_.dbo().find<Quote>().where("id = ?").bind(id).resultValue();
    if (q) q.remove();
    t.commit();
}

// ── Quote Line Items ─────────────────────────────────────────

static QuoteLineItemDTO lineItemToDTO(const Wt::Dbo::ptr<QuoteLineItem>& li) {
    QuoteLineItemDTO dto;
    dto.id        = li.id();
    dto.quoteId   = li->quote.id();
    dto.quantity   = li->quantity;
    dto.unitPrice  = li->unitPrice;
    dto.markup     = li->markup;
    dto.lineTotal  = li->lineTotal;
    dto.notes      = li->notes;

    if (li->product) {
        dto.productId   = li->product.id();
        dto.productName = li->product->name;
        dto.productUnit = li->product->unit;
    }
    if (li->supplier) {
        dto.supplierId   = li->supplier.id();
        dto.supplierName = li->supplier->name;
    }
    return dto;
}

std::vector<QuoteLineItemDTO> LocalDataProvider::findLineItemsByQuoteId(long long quoteId)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto rows = session_.dbo().find<QuoteLineItem>()
        .where("quote_id = ?").bind(quoteId).resultList();

    std::vector<QuoteLineItemDTO> result;
    for (auto& li : rows)
        result.push_back(lineItemToDTO(li));
    t.commit();
    return result;
}

std::optional<QuoteLineItemDTO> LocalDataProvider::findLineItemById(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto li = session_.dbo().find<QuoteLineItem>()
        .where("id = ?").bind(id).resultValue();
    if (!li) return std::nullopt;
    auto dto = lineItemToDTO(li);
    t.commit();
    return dto;
}

QuoteLineItemDTO LocalDataProvider::createLineItem(const QuoteLineItemDTO& dto)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto li = session_.dbo().addNew<QuoteLineItem>();

    auto quote = session_.dbo().find<Quote>()
        .where("id = ?").bind(dto.quoteId).resultValue();
    if (quote)
        li.modify()->quote = quote;

    if (dto.productId > 0) {
        auto product = session_.dbo().find<Product>()
            .where("id = ?").bind(dto.productId).resultValue();
        if (product)
            li.modify()->product = product;
    }
    if (dto.supplierId > 0) {
        auto supplier = session_.dbo().find<Supplier>()
            .where("id = ?").bind(dto.supplierId).resultValue();
        if (supplier)
            li.modify()->supplier = supplier;
    }

    li.modify()->quantity  = dto.quantity;
    li.modify()->unitPrice = dto.unitPrice;
    li.modify()->markup    = dto.markup;
    li.modify()->computeTotal();
    t.commit();

    QuoteLineItemDTO out = dto;
    out.id        = li.id();
    out.lineTotal = li->lineTotal;
    return out;
}

QuoteLineItemDTO LocalDataProvider::updateLineItem(long long id, const QuoteLineItemDTO& dto)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto li = session_.dbo().find<QuoteLineItem>()
        .where("id = ?").bind(id).resultValue();

    li.modify()->quantity  = dto.quantity;
    li.modify()->unitPrice = dto.unitPrice;
    li.modify()->markup    = dto.markup;
    li.modify()->notes     = dto.notes;

    if (dto.supplierId > 0) {
        auto supplier = session_.dbo().find<Supplier>()
            .where("id = ?").bind(dto.supplierId).resultValue();
        if (supplier)
            li.modify()->supplier = supplier;
    }
    li.modify()->computeTotal();
    t.commit();

    QuoteLineItemDTO out = dto;
    out.id        = id;
    out.lineTotal = li->lineTotal;
    return out;
}

void LocalDataProvider::deleteLineItem(long long id)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto li = session_.dbo().find<QuoteLineItem>()
        .where("id = ?").bind(id).resultValue();
    if (li) li.remove();
    t.commit();
}

// ── Aggregates ───────────────────────────────────────────────

int LocalDataProvider::getProductCount()
{
    Wt::Dbo::Transaction t(session_.dbo());
    int n = session_.dbo().query<int>("select count(1) from product");
    t.commit();
    return n;
}

int LocalDataProvider::getSupplierCount()
{
    Wt::Dbo::Transaction t(session_.dbo());
    int n = session_.dbo().query<int>("select count(1) from supplier");
    t.commit();
    return n;
}

int LocalDataProvider::getClientCount()
{
    Wt::Dbo::Transaction t(session_.dbo());
    int n = session_.dbo().query<int>("select count(1) from client");
    t.commit();
    return n;
}

int LocalDataProvider::getQuoteCount()
{
    Wt::Dbo::Transaction t(session_.dbo());
    int n = session_.dbo().query<int>("select count(1) from quote");
    t.commit();
    return n;
}

std::vector<CategoryStatDTO> LocalDataProvider::getCategoryStats()
{
    Wt::Dbo::Transaction t(session_.dbo());

    // Product count per category
    using CatRow = std::tuple<std::string, int>;
    auto rows = session_.dbo().query<CatRow>(
        "select category, count(*) from product group by category order by category"
    ).resultList();

    std::vector<CategoryStatDTO> result;
    for (auto& r : rows) {
        CategoryStatDTO s;
        s.category     = std::get<0>(r);
        s.productCount = std::get<1>(r);
        result.push_back(std::move(s));
    }

    // Distinct supplier count per category via supplier_product join
    using SupCatRow = std::tuple<std::string, int>;
    auto supRows = session_.dbo().query<SupCatRow>(
        "select p.category, count(distinct sp.supplier_id) "
        "from product p "
        "join supplier_product sp on sp.product_id = p.id "
        "group by p.category"
    ).resultList();

    std::map<std::string, int> supCounts;
    for (auto& r : supRows)
        supCounts[std::get<0>(r)] = std::get<1>(r);

    for (auto& s : result) {
        auto it = supCounts.find(s.category);
        if (it != supCounts.end())
            s.supplierCount = it->second;
    }

    t.commit();
    return result;
}

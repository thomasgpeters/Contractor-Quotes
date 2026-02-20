#include "models/Session.h"
#include "models/Product.h"
#include "models/Supplier.h"
#include "models/SupplierProduct.h"
#include "models/Client.h"
#include "models/Quote.h"
#include "models/QuoteLineItem.h"
#include <Wt/WDateTime.h>
#include <iostream>

Session::Session(const std::string& dbPath)
{
    connection_ = std::make_unique<Wt::Dbo::backend::Sqlite3>(dbPath);
    connection_->setProperty("show-queries", "false");
    session_.setConnection(std::move(connection_));

    session_.mapClass<Product>("product");
    session_.mapClass<Supplier>("supplier");
    session_.mapClass<SupplierProduct>("supplier_product");
    session_.mapClass<Client>("client");
    session_.mapClass<Quote>("quote");
    session_.mapClass<QuoteLineItem>("quote_line_item");

    try {
        session_.createTables();
        std::cout << "Database tables created.\n";
    } catch (Wt::Dbo::Exception& e) {
        // Tables already exist
        std::cout << "Tables already exist: " << e.what() << "\n";
    }
}

void Session::seedIfEmpty()
{
    Wt::Dbo::Transaction t(session_);
    if (session_.query<int>("select count(1) from product") == 0) {
        std::cout << "Seeding database with sample data...\n";
        seedProducts();
        seedSuppliers();
        seedSupplierProducts();
        seedClients();
    }
    t.commit();
}

void Session::seedProducts()
{
    auto add = [&](const std::string& name, const std::string& category,
                   const std::string& unit, const std::string& specs,
                   const std::string& sku) {
        auto p = session_.addNew<Product>();
        p.modify()->name           = name;
        p.modify()->category       = category;
        p.modify()->unit           = unit;
        p.modify()->specifications = specs;
        p.modify()->sku            = sku;
    };

    // Lumber
    add("2x4x8 Stud Grade SPF",       "Lumber", "piece",     "Spruce-Pine-Fir, #2 & Better, KD", "LBR-2408");
    add("2x6x10 #2 Doug Fir",         "Lumber", "piece",     "Douglas Fir, #2 Grade, S4S",        "LBR-2610");
    add("4x4x8 Treated Post",         "Lumber", "piece",     "Pressure Treated, Ground Contact",  "LBR-4408T");
    add("3/4\" CDX Plywood 4x8",      "Lumber", "sheet",     "CDX Grade, Exposure 1",             "PLY-3448");
    add("7/16\" OSB Sheathing 4x8",   "Lumber", "sheet",     "Structural, APA Rated",             "PLY-0748");

    // Concrete
    add("80lb Concrete Mix",          "Concrete", "bag",      "4000 PSI, Normal Set",              "CON-80NM");
    add("60lb Quikrete Fast-Set",     "Concrete", "bag",      "4000 PSI, Fast Setting",            "CON-60FS");
    add("#4 Rebar 20ft",              "Concrete", "piece",    "Grade 60, 1/2\" diameter",          "REB-0420");
    add("Wire Mesh 6x6 W1.4",        "Concrete", "sheet",    "Welded, 5ft x 150ft roll",          "MSH-6614");

    // Roofing
    add("Architectural Shingles",     "Roofing",  "bundle",   "30-Year, Algae Resistant",          "ROF-ARCH");
    add("30# Felt Underlayment",      "Roofing",  "roll",     "ASTM D226, 36\" x 144\'",          "ROF-30FL");
    add("Ice & Water Shield",         "Roofing",  "roll",     "Self-Adhesive, 36\" x 75\'",        "ROF-IAWS");
    add("Drip Edge 10ft",             "Roofing",  "piece",    "Aluminum, 2\" x 2\"",               "ROF-DRIP");

    // Electrical
    add("12/2 NM-B Romex 250ft",     "Electrical", "roll",   "With Ground, UL Listed",            "ELE-1225");
    add("14/2 NM-B Romex 250ft",     "Electrical", "roll",   "With Ground, UL Listed",            "ELE-1425");
    add("200A Main Panel Box",       "Electrical", "piece",   "42-Space, Indoor, NEMA 1",          "ELE-200P");
    add("20A GFCI Outlet",           "Electrical", "piece",   "Tamper-Resistant, Self-Test",        "ELE-GFCI");

    // Plumbing
    add("1/2\" Copper Type L 10ft",  "Plumbing", "piece",    "Type L, Hard Drawn",                 "PLB-05CL");
    add("3/4\" PEX Tubing 100ft",    "Plumbing", "roll",     "Oxygen Barrier, ASTM F876",          "PLB-075P");
    add("40-Gal Water Heater",       "Plumbing", "piece",    "Gas, 40000 BTU, Energy Star",        "PLB-40WH");
    add("3\" PVC DWV Pipe 10ft",     "Plumbing", "piece",    "Schedule 40, ASTM D2665",            "PLB-3PVC");

    // Insulation
    add("R-19 Fiberglass Batt 6.25\"", "Insulation", "bag",  "Kraft Faced, 15\" wide",             "INS-R19F");
    add("R-30 Fiberglass Batt 10\"",   "Insulation", "bag",  "Kraft Faced, 15\" wide",             "INS-R30F");
    add("2\" Rigid Foam XPS 4x8",      "Insulation", "sheet","R-10, Moisture Resistant",           "INS-2XPS");

    // Drywall
    add("1/2\" Drywall 4x8",          "Drywall", "sheet",    "Standard, UL Fire Rated",            "DRY-1248");
    add("5/8\" Fire-Rated Drywall",    "Drywall", "sheet",    "Type X, 4x8",                       "DRY-5848");
    add("Joint Compound 5-Gal",        "Drywall", "bucket",   "All-Purpose, Ready Mixed",          "DRY-JC5G");
}

void Session::seedSuppliers()
{
    auto add = [&](const std::string& name, const std::string& addr,
                   const std::string& city, const std::string& state,
                   const std::string& zip, const std::string& phone,
                   const std::string& email, double lat, double lon,
                   double rating, int lead) {
        auto s = session_.addNew<Supplier>();
        s.modify()->name         = name;
        s.modify()->address      = addr;
        s.modify()->city         = city;
        s.modify()->state        = state;
        s.modify()->zipCode      = zip;
        s.modify()->phone        = phone;
        s.modify()->email        = email;
        s.modify()->latitude     = lat;
        s.modify()->longitude    = lon;
        s.modify()->rating       = rating;
        s.modify()->leadTimeDays = lead;
    };

    add("BuildRight Supply Co.",    "1200 Industrial Blvd", "Austin",      "TX", "78701",
        "(512) 555-0101", "sales@buildright.example.com",    30.267, -97.743, 4.5, 2);
    add("Metro Building Materials", "450 Commerce St",      "Dallas",      "TX", "75201",
        "(214) 555-0202", "orders@metrobm.example.com",      32.783, -96.797, 4.2, 3);
    add("Lone Star Lumber",        "890 Timber Rd",        "Houston",     "TX", "77001",
        "(713) 555-0303", "info@lonestarlbr.example.com",    29.760, -95.370, 4.8, 1);
    add("SunBelt Wholesale",       "2100 Supply Dr",       "San Antonio", "TX", "78201",
        "(210) 555-0404", "wholesale@sunbelt.example.com",   29.424, -98.494, 3.9, 4);
    add("ProTrade Distributors",   "560 Warehouse Ave",    "Fort Worth",  "TX", "76102",
        "(817) 555-0505", "sales@protrade.example.com",      32.755, -97.331, 4.6, 2);
    add("Capital City Supply",     "700 Main Plaza",       "Austin",      "TX", "78702",
        "(512) 555-0606", "orders@capitalcity.example.com",  30.260, -97.738, 4.0, 3);
    add("Gulf Coast Materials",    "1500 Port Blvd",       "Corpus Christi","TX","78401",
        "(361) 555-0707", "sales@gulfcoastmat.example.com",  27.801, -97.396, 3.7, 5);
    add("Hill Country Hardware",   "310 Ranch Rd",         "Fredericksburg","TX","78624",
        "(830) 555-0808", "info@hillcountry.example.com",    30.275, -98.872, 4.3, 3);
}

void Session::seedSupplierProducts()
{
    auto products  = session_.find<Product>().resultList();
    auto suppliers = session_.find<Supplier>().resultList();

    // Deterministic price/stock variation per supplier-product pair
    int idx = 0;
    for (auto& prod : products) {
        for (auto& sup : suppliers) {
            // Not every supplier stocks every product
            if ((idx + sup.id()) % 3 == 0) {
                ++idx;
                continue;  // skip ~1/3 of combinations
            }

            auto sp = session_.addNew<SupplierProduct>();
            sp.modify()->product     = prod;
            sp.modify()->supplier    = sup;

            // Base price varies by supplier rating (better suppliers charge slightly more)
            double basePriceMultiplier = 0.85 + (sup->rating / 5.0) * 0.30;
            // Category-based pricing
            double basePrice = 5.0;
            if (prod->category == "Lumber")      basePrice = 4.50 + (idx % 7) * 1.2;
            if (prod->category == "Concrete")    basePrice = 5.00 + (idx % 5) * 0.8;
            if (prod->category == "Roofing")     basePrice = 25.0 + (idx % 10) * 3.0;
            if (prod->category == "Electrical")  basePrice = 15.0 + (idx % 8) * 10.0;
            if (prod->category == "Plumbing")    basePrice = 8.0  + (idx % 9) * 5.0;
            if (prod->category == "Insulation")  basePrice = 18.0 + (idx % 6) * 2.5;
            if (prod->category == "Drywall")     basePrice = 10.0 + (idx % 5) * 1.5;

            sp.modify()->unitPrice = basePrice * basePriceMultiplier;
            sp.modify()->stockQty  = 20 + (idx * 17 + sup.id() * 31) % 500;
            sp.modify()->inStock   = sp->stockQty > 0;
            sp.modify()->canBackorder = (sup.id() % 2 == 0);
            sp.modify()->minOrderQty  = 1 + (idx % 3);
            sp.modify()->bulkDiscount  = (idx % 4 == 0) ? 5.0 : ((idx % 3 == 0) ? 3.0 : 0.0);
            sp.modify()->bulkThreshold = (sp->bulkDiscount > 0) ? 50 + (idx % 50) : 0;
            sp.modify()->lastUpdated   = Wt::WDateTime::currentDateTime();

            ++idx;
        }
    }
}

void Session::seedClients()
{
    auto add = [&](const std::string& name, const std::string& company,
                   const std::string& addr, const std::string& city,
                   const std::string& state, const std::string& zip,
                   const std::string& phone, const std::string& email,
                   double lat, double lon) {
        auto c = session_.addNew<Client>();
        c.modify()->name      = name;
        c.modify()->company   = company;
        c.modify()->address   = addr;
        c.modify()->city      = city;
        c.modify()->state     = state;
        c.modify()->zipCode   = zip;
        c.modify()->phone     = phone;
        c.modify()->email     = email;
        c.modify()->latitude  = lat;
        c.modify()->longitude = lon;
    };

    add("John Martinez",   "Martinez Construction",    "500 Oak Hill Dr",     "Austin",       "TX", "78745",
        "(512) 555-1001", "john@martinezconst.example.com",   30.210, -97.790);
    add("Sarah Chen",      "Chen Renovations LLC",     "1820 Elm St",         "Dallas",       "TX", "75202",
        "(214) 555-1002", "sarah@chenreno.example.com",       32.780, -96.800);
    add("Robert Williams", "Williams Custom Homes",    "4050 Cypress Creek",  "Houston",      "TX", "77014",
        "(713) 555-1003", "rob@williamscustom.example.com",   29.800, -95.400);
    add("Emily Johnson",   "Johnson Property Mgmt",    "230 River Walk",      "San Antonio",  "TX", "78205",
        "(210) 555-1004", "emily@johnsonpm.example.com",      29.420, -98.490);
    add("Mike Thompson",   "Thompson Builders Inc",    "910 Prairie View Ln", "Fort Worth",   "TX", "76104",
        "(817) 555-1005", "mike@thompsonbld.example.com",     32.730, -97.340);
}

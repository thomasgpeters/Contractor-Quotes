#pragma once

#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>

class Supplier;
class SupplierProduct;
class QuoteLineItem;

/// Represents a building material product in the catalog.
class Product {
public:
    std::string name;
    std::string category;       // e.g. "Lumber", "Concrete", "Electrical", "Plumbing", "Roofing"
    std::string unit;           // e.g. "piece", "sqft", "linear ft", "bag", "box"
    std::string specifications; // material grade, dimensions, certifications
    std::string sku;

    Wt::Dbo::collection<Wt::Dbo::ptr<SupplierProduct>> supplierProducts;
    Wt::Dbo::collection<Wt::Dbo::ptr<QuoteLineItem>> lineItems;

    template <class Action>
    void persist(Action& a)
    {
        Wt::Dbo::field(a, name,           "name");
        Wt::Dbo::field(a, category,       "category");
        Wt::Dbo::field(a, unit,           "unit");
        Wt::Dbo::field(a, specifications, "specifications");
        Wt::Dbo::field(a, sku,            "sku");

        Wt::Dbo::hasMany(a, supplierProducts, Wt::Dbo::ManyToOne, "product");
        Wt::Dbo::hasMany(a, lineItems,        Wt::Dbo::ManyToOne, "product");
    }
};

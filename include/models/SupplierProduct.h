#pragma once

#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>
#include <Wt/WDateTime.h>

class Product;
class Supplier;

/// Maps a product to a supplier with pricing, stock, and availability info.
class SupplierProduct {
public:
    double      unitPrice    = 0.0;
    int         stockQty     = 0;
    bool        inStock      = false;
    bool        canBackorder = false;
    int         minOrderQty  = 1;
    double      bulkDiscount = 0.0;   // percentage discount for large orders
    int         bulkThreshold = 0;    // quantity threshold for bulk discount
    Wt::WDateTime lastUpdated;

    Wt::Dbo::ptr<Product>  product;
    Wt::Dbo::ptr<Supplier> supplier;

    template <class Action>
    void persist(Action& a)
    {
        Wt::Dbo::field(a, unitPrice,      "unit_price");
        Wt::Dbo::field(a, stockQty,       "stock_qty");
        Wt::Dbo::field(a, inStock,        "in_stock");
        Wt::Dbo::field(a, canBackorder,   "can_backorder");
        Wt::Dbo::field(a, minOrderQty,    "min_order_qty");
        Wt::Dbo::field(a, bulkDiscount,   "bulk_discount");
        Wt::Dbo::field(a, bulkThreshold,  "bulk_threshold");
        Wt::Dbo::field(a, lastUpdated,    "last_updated");

        Wt::Dbo::belongsTo(a, product,  "product");
        Wt::Dbo::belongsTo(a, supplier, "supplier");
    }
};

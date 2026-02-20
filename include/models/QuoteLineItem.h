#pragma once

#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>

class Quote;
class Product;
class Supplier;

/// A single line item in a quote, linking a product to a chosen supplier.
class QuoteLineItem {
public:
    int    quantity   = 0;
    double unitPrice  = 0.0;  // sourced price from supplier
    double markup     = 0.0;  // line-level markup percentage
    double lineTotal  = 0.0;  // computed: quantity * unitPrice * (1 + markup/100)
    std::string notes;

    Wt::Dbo::ptr<Quote>    quote;
    Wt::Dbo::ptr<Product>  product;
    Wt::Dbo::ptr<Supplier> supplier;  // chosen best supplier for this item

    void computeTotal() {
        lineTotal = quantity * unitPrice * (1.0 + markup / 100.0);
    }

    template <class Action>
    void persist(Action& a)
    {
        Wt::Dbo::field(a, quantity,  "quantity");
        Wt::Dbo::field(a, unitPrice, "unit_price");
        Wt::Dbo::field(a, markup,    "markup");
        Wt::Dbo::field(a, lineTotal, "line_total");
        Wt::Dbo::field(a, notes,     "notes");

        Wt::Dbo::belongsTo(a, quote,    "quote");
        Wt::Dbo::belongsTo(a, product,  "product");
        Wt::Dbo::belongsTo(a, supplier, "supplier");
    }
};

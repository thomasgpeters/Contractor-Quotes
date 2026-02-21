#pragma once

#include <string>
#include <vector>
#include <optional>

/// Plain data-transfer objects decoupled from Wt::Dbo.
/// Used by DataProvider and all views.

struct ProductDTO {
    long long   id = 0;
    std::string name;
    std::string category;
    std::string unit;
    std::string specifications;
    std::string sku;

    // Populated on demand (supplier comparison)
    int    supplierCount = 0;
    double minPrice      = 0.0;
    double maxPrice      = 0.0;
};

struct SupplierDTO {
    long long   id = 0;
    std::string name;
    std::string address;
    std::string city;
    std::string state;
    std::string zipCode;
    std::string phone;
    std::string email;
    std::string website;
    double      latitude      = 0.0;
    double      longitude     = 0.0;
    double      rating        = 0.0;
    int         leadTimeDays  = 0;

    int         productCount  = 0; // populated on demand
};

struct SupplierProductDTO {
    long long   id = 0;
    long long   productId   = 0;
    long long   supplierId  = 0;
    double      unitPrice     = 0.0;
    int         stockQty      = 0;
    bool        inStock       = false;
    bool        canBackorder  = false;
    int         minOrderQty   = 1;
    double      bulkDiscount  = 0.0;
    int         bulkThreshold = 0;
    std::string lastUpdated;

    // Denormalized fields for convenience (joined data)
    std::string productName;
    std::string productSku;
    std::string productCategory;
    std::string supplierName;
    double      supplierRating   = 0.0;
    int         supplierLeadDays = 0;
    double      supplierLat      = 0.0;
    double      supplierLon      = 0.0;
};

struct ClientDTO {
    long long   id = 0;
    std::string name;
    std::string company;
    std::string address;
    std::string city;
    std::string state;
    std::string zipCode;
    std::string phone;
    std::string email;
    double      latitude  = 0.0;
    double      longitude = 0.0;

    int         quoteCount = 0; // populated on demand
};

struct QuoteDTO {
    long long   id = 0;
    long long   clientId = 0;
    std::string title;
    std::string description;
    std::string createdDate;
    std::string expiryDate;
    int         status     = 0; // 0=Draft,1=Sent,2=Accepted,3=Rejected,4=Expired
    double      taxRate    = 0.0;
    double      markupRate = 0.0;
    std::string notes;

    // Denormalized
    std::string clientName;
    double      totalAmount = 0.0; // sum of line totals
    int         lineItemCount = 0;
};

struct QuoteLineItemDTO {
    long long   id = 0;
    long long   quoteId    = 0;
    long long   productId  = 0;
    long long   supplierId = 0;
    int         quantity   = 0;
    double      unitPrice  = 0.0;
    double      markup     = 0.0;
    double      lineTotal  = 0.0;
    std::string notes;

    // Denormalized
    std::string productName;
    std::string productUnit;
    std::string supplierName;
};

struct CategoryStatDTO {
    std::string category;
    int         productCount = 0;
    int         supplierCount = 0; // distinct suppliers sourcing products in this category
};

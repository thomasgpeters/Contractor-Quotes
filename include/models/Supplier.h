#pragma once

#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>

class SupplierProduct;

/// Represents a building materials supplier/vendor.
class Supplier {
public:
    std::string name;
    std::string address;
    std::string city;
    std::string state;
    std::string zipCode;
    std::string phone;
    std::string email;
    std::string website;
    double      latitude  = 0.0;
    double      longitude = 0.0;
    double      rating    = 0.0;  // 0-5 quality rating
    int         leadTimeDays = 0; // typical delivery lead time

    Wt::Dbo::collection<Wt::Dbo::ptr<SupplierProduct>> supplierProducts;

    template <class Action>
    void persist(Action& a)
    {
        Wt::Dbo::field(a, name,         "name");
        Wt::Dbo::field(a, address,      "address");
        Wt::Dbo::field(a, city,         "city");
        Wt::Dbo::field(a, state,        "state");
        Wt::Dbo::field(a, zipCode,      "zip_code");
        Wt::Dbo::field(a, phone,        "phone");
        Wt::Dbo::field(a, email,        "email");
        Wt::Dbo::field(a, website,      "website");
        Wt::Dbo::field(a, latitude,     "latitude");
        Wt::Dbo::field(a, longitude,    "longitude");
        Wt::Dbo::field(a, rating,       "rating");
        Wt::Dbo::field(a, leadTimeDays, "lead_time_days");

        Wt::Dbo::hasMany(a, supplierProducts, Wt::Dbo::ManyToOne, "supplier");
    }
};

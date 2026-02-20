#pragma once

#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>

class Quote;

/// Represents a contractor's client for whom quotes are prepared.
class Client {
public:
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

    Wt::Dbo::collection<Wt::Dbo::ptr<Quote>> quotes;

    template <class Action>
    void persist(Action& a)
    {
        Wt::Dbo::field(a, name,      "name");
        Wt::Dbo::field(a, company,   "company");
        Wt::Dbo::field(a, address,   "address");
        Wt::Dbo::field(a, city,      "city");
        Wt::Dbo::field(a, state,     "state");
        Wt::Dbo::field(a, zipCode,   "zip_code");
        Wt::Dbo::field(a, phone,     "phone");
        Wt::Dbo::field(a, email,     "email");
        Wt::Dbo::field(a, latitude,  "latitude");
        Wt::Dbo::field(a, longitude, "longitude");

        Wt::Dbo::hasMany(a, quotes, Wt::Dbo::ManyToOne, "client");
    }
};

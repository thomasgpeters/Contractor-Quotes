#pragma once

#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>
#include <Wt/WDateTime.h>

class Client;
class QuoteLineItem;

/// Represents a contractor quote prepared for a client.
class Quote {
public:
    enum class Status { Draft, Sent, Accepted, Rejected, Expired };

    std::string   title;
    std::string   description;
    Wt::WDateTime createdDate;
    Wt::WDateTime expiryDate;
    Status        status = Status::Draft;
    double        taxRate = 0.0;       // percentage
    double        markupRate = 0.0;    // contractor markup percentage
    std::string   notes;

    Wt::Dbo::ptr<Client> client;
    Wt::Dbo::collection<Wt::Dbo::ptr<QuoteLineItem>> lineItems;

    template <class Action>
    void persist(Action& a)
    {
        Wt::Dbo::field(a, title,       "title");
        Wt::Dbo::field(a, description, "description");
        Wt::Dbo::field(a, createdDate, "created_date");
        Wt::Dbo::field(a, expiryDate,  "expiry_date");
        Wt::Dbo::field(a, status,      "status");
        Wt::Dbo::field(a, taxRate,     "tax_rate");
        Wt::Dbo::field(a, markupRate,  "markup_rate");
        Wt::Dbo::field(a, notes,       "notes");

        Wt::Dbo::belongsTo(a, client, "client");
        Wt::Dbo::hasMany(a, lineItems, Wt::Dbo::ManyToOne, "quote");
    }
};

// Allow Dbo to store the enum as an int
namespace Wt {
namespace Dbo {
template<>
struct sql_value_traits<Quote::Status, void>
    : public sql_value_traits<int, void>
{
    static const char* type(SqlConnection* conn, int size) {
        return sql_value_traits<int, void>::type(conn, size);
    }
    static void bind(Quote::Status v, SqlStatement* statement, int column, int size) {
        sql_value_traits<int, void>::bind(static_cast<int>(v), statement, column, size);
    }
    static bool read(Quote::Status& v, SqlStatement* statement, int column, int size) {
        int intVal = 0;
        bool result = sql_value_traits<int, void>::read(intVal, statement, column, size);
        v = static_cast<Quote::Status>(intVal);
        return result;
    }
};
}
}

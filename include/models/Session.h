#pragma once

#include <Wt/Dbo/Session.h>
#include <Wt/Dbo/backend/Sqlite3.h>
#include <memory>
#include <string>

class Product;
class Supplier;
class SupplierProduct;
class Client;
class Quote;
class QuoteLineItem;

/// Database session manager – owns the Dbo::Session and configures mappings.
class Session {
public:
    explicit Session(const std::string& dbPath);

    Wt::Dbo::Session& dbo() { return session_; }

    /// Populate the database with sample data if empty.
    void seedIfEmpty();

private:
    std::unique_ptr<Wt::Dbo::backend::Sqlite3> connection_;
    Wt::Dbo::Session session_;

    void seedProducts();
    void seedSuppliers();
    void seedSupplierProducts();
    void seedClients();
};

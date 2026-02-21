#pragma once

#include "data/DataTypes.h"
#include <memory>
#include <vector>
#include <optional>

/// Abstract interface for all persistent-data operations.
/// Two concrete implementations:
///   LocalDataProvider  – direct Wt::Dbo / SQLite / PostgreSQL
///   ApiDataProvider    – REST calls to ApiLogicServer

class DataProvider {
public:
    virtual ~DataProvider() = default;

    // ── Products ─────────────────────────────────────────
    virtual std::vector<ProductDTO>       findAllProducts()                        = 0;
    virtual std::optional<ProductDTO>     findProductById(long long id)            = 0;
    virtual std::vector<std::string>      getDistinctCategories()                  = 0;

    // ── Suppliers ────────────────────────────────────────
    virtual std::vector<SupplierDTO>      findAllSuppliers()                       = 0;
    virtual std::optional<SupplierDTO>    findSupplierById(long long id)           = 0;

    // ── Supplier-Products ────────────────────────────────
    virtual std::vector<SupplierProductDTO> findSupplierProductsByProductId(long long productId) = 0;
    virtual std::vector<SupplierProductDTO> findSupplierProductsBySupplierId(long long supplierId) = 0;

    // ── Clients ──────────────────────────────────────────
    virtual std::vector<ClientDTO>        findAllClients()                         = 0;
    virtual std::optional<ClientDTO>      findClientById(long long id)             = 0;
    virtual ClientDTO                     createClient(const ClientDTO& dto)       = 0;
    virtual ClientDTO                     updateClient(long long id, const ClientDTO& dto) = 0;
    virtual void                          deleteClient(long long id)               = 0;

    // ── Quotes ───────────────────────────────────────────
    virtual std::vector<QuoteDTO>         findAllQuotes()                          = 0;
    virtual std::optional<QuoteDTO>       findQuoteById(long long id)              = 0;
    virtual std::vector<QuoteDTO>         findRecentQuotes(int limit)              = 0;
    virtual QuoteDTO                      createQuote(const QuoteDTO& dto)         = 0;
    virtual QuoteDTO                      updateQuote(long long id, const QuoteDTO& dto) = 0;
    virtual void                          deleteQuote(long long id)                = 0;

    // ── Quote Line Items ─────────────────────────────────
    virtual std::vector<QuoteLineItemDTO> findLineItemsByQuoteId(long long quoteId) = 0;
    virtual std::optional<QuoteLineItemDTO> findLineItemById(long long id)         = 0;
    virtual QuoteLineItemDTO              createLineItem(const QuoteLineItemDTO& dto) = 0;
    virtual QuoteLineItemDTO              updateLineItem(long long id, const QuoteLineItemDTO& dto) = 0;
    virtual void                          deleteLineItem(long long id)             = 0;

    // ── Aggregates (Dashboard) ───────────────────────────
    virtual int getProductCount()  = 0;
    virtual int getSupplierCount() = 0;
    virtual int getClientCount()   = 0;
    virtual int getQuoteCount()    = 0;
    virtual std::vector<CategoryStatDTO> getCategoryStats() = 0;
};

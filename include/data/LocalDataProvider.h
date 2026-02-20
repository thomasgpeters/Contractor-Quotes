#pragma once

#include "data/DataProvider.h"
#include <Wt/Dbo/Session.h>

class Session;

/// DataProvider backed by a direct Wt::Dbo database session (SQLite or PostgreSQL).
class LocalDataProvider : public DataProvider {
public:
    explicit LocalDataProvider(Session& session);

    // Products
    std::vector<ProductDTO>       findAllProducts() override;
    std::optional<ProductDTO>     findProductById(long long id) override;
    std::vector<std::string>      getDistinctCategories() override;

    // Suppliers
    std::vector<SupplierDTO>      findAllSuppliers() override;
    std::optional<SupplierDTO>    findSupplierById(long long id) override;

    // Supplier-Products
    std::vector<SupplierProductDTO> findSupplierProductsByProductId(long long productId) override;
    std::vector<SupplierProductDTO> findSupplierProductsBySupplierId(long long supplierId) override;

    // Clients
    std::vector<ClientDTO>        findAllClients() override;
    std::optional<ClientDTO>      findClientById(long long id) override;
    ClientDTO                     createClient(const ClientDTO& dto) override;
    ClientDTO                     updateClient(long long id, const ClientDTO& dto) override;
    void                          deleteClient(long long id) override;

    // Quotes
    std::vector<QuoteDTO>         findAllQuotes() override;
    std::optional<QuoteDTO>       findQuoteById(long long id) override;
    std::vector<QuoteDTO>         findRecentQuotes(int limit) override;
    QuoteDTO                      createQuote(const QuoteDTO& dto) override;
    QuoteDTO                      updateQuote(long long id, const QuoteDTO& dto) override;
    void                          deleteQuote(long long id) override;

    // Quote Line Items
    std::vector<QuoteLineItemDTO> findLineItemsByQuoteId(long long quoteId) override;
    std::optional<QuoteLineItemDTO> findLineItemById(long long id) override;
    QuoteLineItemDTO              createLineItem(const QuoteLineItemDTO& dto) override;
    QuoteLineItemDTO              updateLineItem(long long id, const QuoteLineItemDTO& dto) override;
    void                          deleteLineItem(long long id) override;

    // Aggregates
    int getProductCount() override;
    int getSupplierCount() override;
    int getClientCount() override;
    int getQuoteCount() override;
    std::vector<CategoryStatDTO> getCategoryStats() override;

private:
    Session& session_;
};

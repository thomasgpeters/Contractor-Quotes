#include "views/DashboardView.h"
#include "data/DataProvider.h"
#include <Wt/WText.h>
#include <Wt/WTable.h>
#include <Wt/WBreak.h>
#include <sstream>
#include <iomanip>

static const char* statusLabel(int s) {
    switch (s) {
        case 0: return "Draft";
        case 1: return "Sent";
        case 2: return "Accepted";
        case 3: return "Rejected";
        case 4: return "Expired";
        default: return "?";
    }
}

DashboardView::DashboardView(DataProvider& provider)
    : provider_(provider)
{
    addStyleClass("dashboard-view");
    try {
        buildUI();
    } catch (const std::exception& e) {
        addWidget(std::make_unique<Wt::WText>("<h2>Dashboard</h2>"));
        addWidget(std::make_unique<Wt::WText>(
            "<div class='alert alert-danger'>Unable to load dashboard: " +
            std::string(e.what()) + "</div>"));
    }
}

void DashboardView::buildUI()
{
    int productCount  = provider_.getProductCount();
    int supplierCount = provider_.getSupplierCount();
    int clientCount   = provider_.getClientCount();
    int quoteCount    = provider_.getQuoteCount();

    auto allQuotes = provider_.findAllQuotes();
    double totalQuoteValue = 0.0;
    for (auto& q : allQuotes)
        totalQuoteValue += q.totalAmount;

    // Header
    addWidget(std::make_unique<Wt::WText>("<h2>Dashboard</h2>"));
    addWidget(std::make_unique<Wt::WText>(
        "<p class='dashboard-subtitle'>Contractor Quote &amp; Sourcing Overview</p>"));

    // Stats cards
    auto cardsDiv = addWidget(std::make_unique<Wt::WContainerWidget>());
    cardsDiv->addStyleClass("stats-cards");

    auto addCard = [&](const std::string& label, const std::string& value,
                       const std::string& icon, const std::string& cssClass) {
        auto card = cardsDiv->addWidget(std::make_unique<Wt::WContainerWidget>());
        card->addStyleClass("stat-card " + cssClass);
        card->addWidget(std::make_unique<Wt::WText>(
            "<div class='stat-icon'>" + icon + "</div>"
            "<div class='stat-value'>" + value + "</div>"
            "<div class='stat-label'>" + label + "</div>"
        ));
    };

    addCard("Products in Catalog", std::to_string(productCount),  "&#128230;", "card-products");
    addCard("Active Suppliers",    std::to_string(supplierCount), "&#127970;", "card-suppliers");
    addCard("Clients",             std::to_string(clientCount),   "&#128101;", "card-clients");
    addCard("Quotes Created",      std::to_string(quoteCount),    "&#128196;", "card-quotes");

    std::ostringstream oss;
    oss << "$" << std::fixed << std::setprecision(2) << totalQuoteValue;
    addCard("Total Quote Value", oss.str(), "&#128176;", "card-value");

    // Recent quotes card
    auto quotesCard = addWidget(std::make_unique<Wt::WContainerWidget>());
    quotesCard->addStyleClass("dashboard-card");
    quotesCard->addWidget(std::make_unique<Wt::WText>("<h3>Recent Quotes</h3>"));

    if (quoteCount > 0) {
        auto table = quotesCard->addWidget(std::make_unique<Wt::WTable>());
        table->addStyleClass("table table-striped");
        table->setHeaderCount(1);
        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Title"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Client"));
        table->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Status"));
        table->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Items"));
        table->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Total"));

        auto recentQuotes = provider_.findRecentQuotes(10);
        int row = 1;
        for (auto& q : recentQuotes) {
            table->elementAt(row, 0)->addWidget(
                std::make_unique<Wt::WText>(q.title));
            table->elementAt(row, 1)->addWidget(
                std::make_unique<Wt::WText>(q.clientName.empty() ? "(none)" : q.clientName));
            table->elementAt(row, 2)->addWidget(
                std::make_unique<Wt::WText>(statusLabel(q.status)));

            table->elementAt(row, 3)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(q.lineItemCount)));

            std::ostringstream ts;
            ts << "$" << std::fixed << std::setprecision(2) << q.totalAmount;
            table->elementAt(row, 4)->addWidget(
                std::make_unique<Wt::WText>(ts.str()));
            ++row;
        }
    } else {
        quotesCard->addWidget(std::make_unique<Wt::WText>(
            "<p class='text-muted'>No quotes yet. Go to the Quote Builder to create one.</p>"));
    }

    // Product categories card
    auto catCard = addWidget(std::make_unique<Wt::WContainerWidget>());
    catCard->addStyleClass("dashboard-card");
    catCard->addWidget(std::make_unique<Wt::WText>("<h3>Product Categories</h3>"));
    auto catTable = catCard->addWidget(std::make_unique<Wt::WTable>());
    catTable->addStyleClass("table table-striped");
    catTable->setHeaderCount(1);
    catTable->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Category"));
    catTable->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Products"));
    catTable->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Suppliers"));

    auto categories = provider_.getCategoryStats();
    int crow = 1;
    for (auto& cat : categories) {
        catTable->elementAt(crow, 0)->addWidget(std::make_unique<Wt::WText>(cat.category));
        catTable->elementAt(crow, 1)->addWidget(std::make_unique<Wt::WText>(std::to_string(cat.productCount)));
        catTable->elementAt(crow, 2)->addWidget(std::make_unique<Wt::WText>(std::to_string(cat.supplierCount)));
        ++crow;
    }
}

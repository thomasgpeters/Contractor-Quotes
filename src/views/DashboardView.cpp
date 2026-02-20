#include "views/DashboardView.h"
#include "models/Session.h"
#include "models/Product.h"
#include "models/Supplier.h"
#include "models/Client.h"
#include "models/Quote.h"
#include "models/QuoteLineItem.h"
#include <Wt/WText.h>
#include <Wt/WTable.h>
#include <Wt/WBreak.h>
#include <sstream>
#include <iomanip>

DashboardView::DashboardView(Session& session)
    : session_(session)
{
    addStyleClass("dashboard-view");
    buildUI();
}

void DashboardView::buildUI()
{
    Wt::Dbo::Transaction t(session_.dbo());

    int productCount  = session_.dbo().query<int>("select count(1) from product");
    int supplierCount = session_.dbo().query<int>("select count(1) from supplier");
    int clientCount   = session_.dbo().query<int>("select count(1) from client");
    int quoteCount    = session_.dbo().query<int>("select count(1) from quote");

    double totalQuoteValue = 0.0;
    auto quotes = session_.dbo().find<Quote>().resultList();
    for (const auto& q : quotes) {
        for (const auto& li : q->lineItems) {
            totalQuoteValue += li->lineTotal;
        }
    }

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

    // Recent quotes table
    addWidget(std::make_unique<Wt::WText>("<h3>Recent Quotes</h3>"));

    if (quoteCount > 0) {
        auto table = addWidget(std::make_unique<Wt::WTable>());
        table->addStyleClass("table table-striped");
        table->setHeaderCount(1);
        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Title"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Client"));
        table->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Status"));
        table->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Items"));
        table->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Total"));

        int row = 1;
        auto recentQuotes = session_.dbo().find<Quote>()
            .orderBy("created_date desc")
            .limit(10)
            .resultList();

        for (const auto& q : recentQuotes) {
            table->elementAt(row, 0)->addWidget(
                std::make_unique<Wt::WText>(q->title));

            std::string clientName = q->client ? q->client->name : "(none)";
            table->elementAt(row, 1)->addWidget(
                std::make_unique<Wt::WText>(clientName));

            std::string statusStr;
            switch (q->status) {
                case Quote::Status::Draft:    statusStr = "Draft"; break;
                case Quote::Status::Sent:     statusStr = "Sent"; break;
                case Quote::Status::Accepted: statusStr = "Accepted"; break;
                case Quote::Status::Rejected: statusStr = "Rejected"; break;
                case Quote::Status::Expired:  statusStr = "Expired"; break;
            }
            table->elementAt(row, 2)->addWidget(
                std::make_unique<Wt::WText>(statusStr));

            int itemCount = 0;
            double total = 0.0;
            for (const auto& li : q->lineItems) {
                ++itemCount;
                total += li->lineTotal;
            }
            table->elementAt(row, 3)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(itemCount)));

            std::ostringstream ts;
            ts << "$" << std::fixed << std::setprecision(2) << total;
            table->elementAt(row, 4)->addWidget(
                std::make_unique<Wt::WText>(ts.str()));

            ++row;
        }
    } else {
        addWidget(std::make_unique<Wt::WText>(
            "<p class='text-muted'>No quotes yet. Go to the Quote Builder to create one.</p>"));
    }

    // Product categories summary
    addWidget(std::make_unique<Wt::WText>("<h3>Product Categories</h3>"));
    auto catTable = addWidget(std::make_unique<Wt::WTable>());
    catTable->addStyleClass("table table-striped");
    catTable->setHeaderCount(1);
    catTable->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Category"));
    catTable->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Products"));
    catTable->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Avg. Sources"));

    using CatRow = std::tuple<std::string, int>;
    auto categories = session_.dbo().query<CatRow>(
        "select category, count(*) from product group by category order by category"
    ).resultList();

    int crow = 1;
    for (auto& [cat, count] : categories) {
        catTable->elementAt(crow, 0)->addWidget(std::make_unique<Wt::WText>(cat));
        catTable->elementAt(crow, 1)->addWidget(std::make_unique<Wt::WText>(std::to_string(count)));

        // Average number of suppliers per product in this category
        auto avgSources = session_.dbo().query<double>(
            "select avg(cnt) from (select count(*) as cnt from supplier_product sp "
            "join product p on sp.product_id = p.id where p.category = ? group by sp.product_id)"
        ).bind(cat).resultValue();
        std::ostringstream as;
        as << std::fixed << std::setprecision(1) << avgSources;
        catTable->elementAt(crow, 2)->addWidget(std::make_unique<Wt::WText>(as.str()));
        ++crow;
    }

    t.commit();
}

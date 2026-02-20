#include "views/ProductCatalogView.h"
#include "models/Session.h"
#include "models/Product.h"
#include "models/Supplier.h"
#include "models/SupplierProduct.h"
#include "engine/SourcingEngine.h"
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
#include <Wt/WBreak.h>
#include <Wt/WDialog.h>
#include <Wt/WLabel.h>
#include <sstream>
#include <iomanip>

ProductCatalogView::ProductCatalogView(Session& session)
    : session_(session)
{
    addStyleClass("product-catalog-view");
    buildUI();
}

void ProductCatalogView::buildUI()
{
    addWidget(std::make_unique<Wt::WText>("<h2>Product Catalog</h2>"));
    addWidget(std::make_unique<Wt::WText>(
        "<p>Browse building materials and compare supplier pricing.</p>"));

    // Filters toolbar
    auto toolbar = addWidget(std::make_unique<Wt::WContainerWidget>());
    toolbar->addStyleClass("toolbar");

    toolbar->addWidget(std::make_unique<Wt::WLabel>("Category: "));
    categoryFilter_ = toolbar->addWidget(std::make_unique<Wt::WComboBox>());
    categoryFilter_->addStyleClass("form-control inline-control");
    categoryFilter_->addItem("All Categories");

    {
        Wt::Dbo::Transaction t(session_.dbo());
        using CatRow = std::tuple<std::string>;
        auto cats = session_.dbo().query<CatRow>(
            "select distinct category from product order by category"
        ).resultList();
        for (auto& [cat] : cats) {
            categoryFilter_->addItem(cat);
        }
        t.commit();
    }

    categoryFilter_->changed().connect(this, &ProductCatalogView::refreshTable);

    toolbar->addWidget(std::make_unique<Wt::WLabel>("  Search: "));
    searchBox_ = toolbar->addWidget(std::make_unique<Wt::WLineEdit>());
    searchBox_->setPlaceholderText("Search products...");
    searchBox_->addStyleClass("form-control inline-control");
    searchBox_->textInput().connect(this, &ProductCatalogView::refreshTable);

    table_ = addWidget(std::make_unique<Wt::WTable>());
    table_->addStyleClass("table table-striped");

    refreshTable();
}

void ProductCatalogView::refreshTable()
{
    table_->clear();
    table_->setHeaderCount(1);

    table_->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("SKU"));
    table_->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Product Name"));
    table_->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Category"));
    table_->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Unit"));
    table_->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Specifications"));
    table_->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Sources"));
    table_->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Price Range"));
    table_->elementAt(0, 7)->addWidget(std::make_unique<Wt::WText>("Actions"));

    Wt::Dbo::Transaction t(session_.dbo());

    std::string catFilter = categoryFilter_->currentText().toUTF8();
    std::string searchText = searchBox_->text().toUTF8();

    auto query = session_.dbo().find<Product>().orderBy("category, name");

    // We'll filter in code since Wt Dbo doesn't support dynamic where clauses elegantly
    auto products = query.resultList();

    int row = 1;
    for (auto& p : products) {
        // Apply category filter
        if (catFilter != "All Categories" && p->category != catFilter)
            continue;

        // Apply search filter
        if (!searchText.empty()) {
            std::string lowerName = p->name;
            std::string lowerSearch = searchText;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
            if (lowerName.find(lowerSearch) == std::string::npos &&
                p->sku.find(searchText) == std::string::npos)
                continue;
        }

        table_->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(p->sku));
        table_->elementAt(row, 1)->addWidget(std::make_unique<Wt::WText>(p->name));
        table_->elementAt(row, 2)->addWidget(std::make_unique<Wt::WText>(p->category));
        table_->elementAt(row, 3)->addWidget(std::make_unique<Wt::WText>(p->unit));
        table_->elementAt(row, 4)->addWidget(std::make_unique<Wt::WText>(p->specifications));

        // Count sources and price range
        int sourceCount = 0;
        double minPrice = 1e9, maxPrice = 0;
        for (const auto& sp : p->supplierProducts) {
            ++sourceCount;
            if (sp->unitPrice < minPrice) minPrice = sp->unitPrice;
            if (sp->unitPrice > maxPrice) maxPrice = sp->unitPrice;
        }

        table_->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(sourceCount)));

        if (sourceCount > 0) {
            std::ostringstream priceRange;
            priceRange << "$" << std::fixed << std::setprecision(2) << minPrice
                       << " - $" << maxPrice;
            table_->elementAt(row, 6)->addWidget(
                std::make_unique<Wt::WText>(priceRange.str()));
        } else {
            table_->elementAt(row, 6)->addWidget(
                std::make_unique<Wt::WText>("N/A"));
        }

        long long pid = p.id();
        auto compareBtn = table_->elementAt(row, 7)->addWidget(
            std::make_unique<Wt::WPushButton>("Compare Sources"));
        compareBtn->addStyleClass("btn btn-sm btn-info");
        compareBtn->clicked().connect([this, pid] { showSupplierComparison(pid); });

        ++row;
    }

    t.commit();
}

void ProductCatalogView::showSupplierComparison(long long productId)
{
    Wt::Dbo::Transaction t(session_.dbo());

    auto product = session_.dbo().find<Product>()
        .where("id = ?").bind(productId).resultValue();
    if (!product) return;

    auto dialog = addChild(std::make_unique<Wt::WDialog>(
        "Supplier Comparison: " + product->name));
    dialog->setModal(true);
    dialog->setClosable(true);
    dialog->rejectWhenEscapePressed();
    dialog->setWidth(Wt::WLength(900));
    dialog->contents()->addStyleClass("dialog-content");

    dialog->contents()->addWidget(std::make_unique<Wt::WText>(
        "<p><strong>Category:</strong> " + product->category +
        " | <strong>Unit:</strong> " + product->unit +
        " | <strong>Specs:</strong> " + product->specifications + "</p>"));

    // Run sourcing engine with Austin, TX as default job site
    SourcingEngine engine(session_.dbo());
    auto results = engine.findBestSources(productId, 10, 30.267, -97.743);

    if (results.empty()) {
        dialog->contents()->addWidget(std::make_unique<Wt::WText>(
            "<p>No suppliers found for this product.</p>"));
    } else {
        auto table = dialog->contents()->addWidget(std::make_unique<Wt::WTable>());
        table->addStyleClass("table table-striped");
        table->setHeaderCount(1);

        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Rank"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Supplier"));
        table->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Unit Price"));
        table->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Eff. Price"));
        table->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("In Stock"));
        table->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Available"));
        table->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Distance"));
        table->elementAt(0, 7)->addWidget(std::make_unique<Wt::WText>("Rating"));
        table->elementAt(0, 8)->addWidget(std::make_unique<Wt::WText>("Lead Time"));
        table->elementAt(0, 9)->addWidget(std::make_unique<Wt::WText>("Score"));

        int row = 1;
        for (auto& r : results) {
            table->elementAt(row, 0)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(row)));
            table->elementAt(row, 1)->addWidget(
                std::make_unique<Wt::WText>(r.supplierName));

            std::ostringstream up, ep, dist, score;
            up << "$" << std::fixed << std::setprecision(2) << r.unitPrice;
            ep << "$" << std::fixed << std::setprecision(2) << r.effectivePrice;
            dist << std::fixed << std::setprecision(1) << r.distanceMiles << " mi";
            score << std::fixed << std::setprecision(3) << r.compositeScore;

            table->elementAt(row, 2)->addWidget(std::make_unique<Wt::WText>(up.str()));
            table->elementAt(row, 3)->addWidget(std::make_unique<Wt::WText>(ep.str()));
            table->elementAt(row, 4)->addWidget(
                std::make_unique<Wt::WText>(r.inStock ? "Yes" : "No"));
            table->elementAt(row, 5)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(r.availableQty)));
            table->elementAt(row, 6)->addWidget(std::make_unique<Wt::WText>(dist.str()));

            std::ostringstream rat;
            rat << std::fixed << std::setprecision(1) << r.supplierRating << "/5";
            table->elementAt(row, 7)->addWidget(std::make_unique<Wt::WText>(rat.str()));
            table->elementAt(row, 8)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(r.leadTimeDays) + " days"));
            table->elementAt(row, 9)->addWidget(std::make_unique<Wt::WText>(score.str()));

            // Highlight best source
            if (row == 1) {
                for (int col = 0; col < 10; ++col) {
                    table->elementAt(row, col)->addStyleClass("best-source");
                }
            }

            ++row;
        }
    }

    auto closeBtn = dialog->footer()->addWidget(
        std::make_unique<Wt::WPushButton>("Close"));
    closeBtn->addStyleClass("btn btn-secondary");
    closeBtn->clicked().connect(dialog, &Wt::WDialog::reject);

    t.commit();
    dialog->show();
}

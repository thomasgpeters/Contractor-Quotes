#include "views/ProductCatalogView.h"
#include "data/DataProvider.h"
#include "engine/SourcingEngine.h"
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
#include <Wt/WBreak.h>
#include <Wt/WDialog.h>
#include <Wt/WLabel.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

ProductCatalogView::ProductCatalogView(DataProvider& provider)
    : provider_(provider)
{
    addStyleClass("product-catalog-view");
    buildUI();
}

void ProductCatalogView::buildUI()
{
    addWidget(std::make_unique<Wt::WText>("<h2>Product Catalog</h2>"));
    addWidget(std::make_unique<Wt::WText>(
        "<p>Browse building materials and compare supplier pricing.</p>"));

    auto toolbar = addWidget(std::make_unique<Wt::WContainerWidget>());
    toolbar->addStyleClass("toolbar");

    toolbar->addWidget(std::make_unique<Wt::WLabel>("Category: "));
    categoryFilter_ = toolbar->addWidget(std::make_unique<Wt::WComboBox>());
    categoryFilter_->addStyleClass("form-control inline-control");
    categoryFilter_->addItem("All Categories");

    auto cats = provider_.getDistinctCategories();
    for (auto& cat : cats)
        categoryFilter_->addItem(cat);

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

    std::string catFilter = categoryFilter_->currentText().toUTF8();
    std::string searchText = searchBox_->text().toUTF8();

    auto products = provider_.findAllProducts();

    int row = 1;
    for (auto& p : products) {
        if (catFilter != "All Categories" && p.category != catFilter)
            continue;

        if (!searchText.empty()) {
            std::string lowerName = p.name;
            std::string lowerSearch = searchText;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
            if (lowerName.find(lowerSearch) == std::string::npos &&
                p.sku.find(searchText) == std::string::npos)
                continue;
        }

        table_->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(p.sku));
        table_->elementAt(row, 1)->addWidget(std::make_unique<Wt::WText>(p.name));
        table_->elementAt(row, 2)->addWidget(std::make_unique<Wt::WText>(p.category));
        table_->elementAt(row, 3)->addWidget(std::make_unique<Wt::WText>(p.unit));
        table_->elementAt(row, 4)->addWidget(std::make_unique<Wt::WText>(p.specifications));

        table_->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(p.supplierCount)));

        if (p.supplierCount > 0) {
            std::ostringstream priceRange;
            priceRange << "$" << std::fixed << std::setprecision(2) << p.minPrice
                       << " - $" << p.maxPrice;
            table_->elementAt(row, 6)->addWidget(
                std::make_unique<Wt::WText>(priceRange.str()));
        } else {
            table_->elementAt(row, 6)->addWidget(
                std::make_unique<Wt::WText>("N/A"));
        }

        long long pid = p.id;
        auto compareBtn = table_->elementAt(row, 7)->addWidget(
            std::make_unique<Wt::WPushButton>("Compare Sources"));
        compareBtn->addStyleClass("btn btn-sm btn-info");
        compareBtn->clicked().connect([this, pid] { showSupplierComparison(pid); });

        ++row;
    }
}

void ProductCatalogView::showSupplierComparison(long long productId)
{
    auto product = provider_.findProductById(productId);
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

    SourcingEngine engine(provider_);
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

        int srow = 1;
        for (auto& r : results) {
            table->elementAt(srow, 0)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(srow)));
            table->elementAt(srow, 1)->addWidget(
                std::make_unique<Wt::WText>(r.supplierName));

            std::ostringstream up, ep, dist, score;
            up << "$" << std::fixed << std::setprecision(2) << r.unitPrice;
            ep << "$" << std::fixed << std::setprecision(2) << r.effectivePrice;
            dist << std::fixed << std::setprecision(1) << r.distanceMiles << " mi";
            score << std::fixed << std::setprecision(3) << r.compositeScore;

            table->elementAt(srow, 2)->addWidget(std::make_unique<Wt::WText>(up.str()));
            table->elementAt(srow, 3)->addWidget(std::make_unique<Wt::WText>(ep.str()));
            table->elementAt(srow, 4)->addWidget(
                std::make_unique<Wt::WText>(r.inStock ? "Yes" : "No"));
            table->elementAt(srow, 5)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(r.availableQty)));
            table->elementAt(srow, 6)->addWidget(std::make_unique<Wt::WText>(dist.str()));

            std::ostringstream rat;
            rat << std::fixed << std::setprecision(1) << r.supplierRating << "/5";
            table->elementAt(srow, 7)->addWidget(std::make_unique<Wt::WText>(rat.str()));
            table->elementAt(srow, 8)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(r.leadTimeDays) + " days"));
            table->elementAt(srow, 9)->addWidget(std::make_unique<Wt::WText>(score.str()));

            if (srow == 1) {
                for (int col = 0; col < 10; ++col)
                    table->elementAt(srow, col)->addStyleClass("best-source");
            }
            ++srow;
        }
    }

    auto closeBtn = dialog->footer()->addWidget(
        std::make_unique<Wt::WPushButton>("Close"));
    closeBtn->addStyleClass("btn btn-secondary");
    closeBtn->clicked().connect(dialog, &Wt::WDialog::reject);

    dialog->show();
}

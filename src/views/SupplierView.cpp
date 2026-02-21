#include "views/SupplierView.h"
#include "data/DataProvider.h"
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
#include <Wt/WDialog.h>
#include <Wt/WBreak.h>
#include <sstream>
#include <iomanip>

SupplierView::SupplierView(DataProvider& provider)
    : provider_(provider)
{
    addStyleClass("supplier-view");
    buildUI();
}

void SupplierView::buildUI()
{
    addWidget(std::make_unique<Wt::WText>("<h2>Supplier Directory</h2>"));
    addWidget(std::make_unique<Wt::WText>(
        "<p>View all registered building material suppliers and their inventory.</p>"));

    table_ = addWidget(std::make_unique<Wt::WTable>());
    table_->addStyleClass("table table-striped");

    refreshTable();
}

void SupplierView::refreshTable()
{
    table_->clear();
    table_->setHeaderCount(1);

    table_->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Supplier"));
    table_->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Location"));
    table_->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Phone"));
    table_->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Rating"));
    table_->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Lead Time"));
    table_->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Products"));
    table_->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Actions"));

    auto suppliers = provider_.findAllSuppliers();

    int row = 1;
    for (auto& s : suppliers) {
        table_->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(s.name));

        std::string location = s.city + ", " + s.state + " " + s.zipCode;
        table_->elementAt(row, 1)->addWidget(std::make_unique<Wt::WText>(location));
        table_->elementAt(row, 2)->addWidget(std::make_unique<Wt::WText>(s.phone));

        std::ostringstream rat;
        rat << std::fixed << std::setprecision(1) << s.rating << "/5.0";
        table_->elementAt(row, 3)->addWidget(std::make_unique<Wt::WText>(rat.str()));
        table_->elementAt(row, 4)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(s.leadTimeDays) + " days"));
        table_->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(s.productCount)));

        long long sid = s.id;
        auto detailBtn = table_->elementAt(row, 6)->addWidget(
            std::make_unique<Wt::WPushButton>("View Inventory"));
        detailBtn->addStyleClass("btn btn-sm btn-info");
        detailBtn->clicked().connect([this, sid] { showSupplierDetail(sid); });

        ++row;
    }
}

void SupplierView::showSupplierDetail(long long supplierId)
{
    auto supplier = provider_.findSupplierById(supplierId);
    if (!supplier) return;

    auto dialog = addChild(std::make_unique<Wt::WDialog>(supplier->name + " - Inventory"));
    dialog->setModal(true);
    dialog->setClosable(true);
    dialog->rejectWhenEscapePressed();
    dialog->setWidth(Wt::WLength(850));
    dialog->contents()->addStyleClass("dialog-content");

    std::ostringstream info;
    info << "<div class='supplier-info'>"
         << "<p><strong>Address:</strong> " << supplier->address << ", "
         << supplier->city << ", " << supplier->state << " " << supplier->zipCode << "</p>"
         << "<p><strong>Phone:</strong> " << supplier->phone
         << " | <strong>Email:</strong> " << supplier->email << "</p>"
         << "<p><strong>Rating:</strong> " << std::fixed << std::setprecision(1) << supplier->rating << "/5.0"
         << " | <strong>Lead Time:</strong> " << supplier->leadTimeDays << " days</p>"
         << "</div>";
    dialog->contents()->addWidget(std::make_unique<Wt::WText>(info.str()));

    auto spList = provider_.findSupplierProductsBySupplierId(supplierId);

    auto invTable = dialog->contents()->addWidget(std::make_unique<Wt::WTable>());
    invTable->addStyleClass("table table-striped");
    invTable->setHeaderCount(1);

    invTable->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("SKU"));
    invTable->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Product"));
    invTable->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Category"));
    invTable->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Unit Price"));
    invTable->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Stock"));
    invTable->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("In Stock"));
    invTable->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Bulk Disc."));
    invTable->elementAt(0, 7)->addWidget(std::make_unique<Wt::WText>("Min Order"));

    int row = 1;
    for (auto& sp : spList) {
        invTable->elementAt(row, 0)->addWidget(
            std::make_unique<Wt::WText>(sp.productSku));
        invTable->elementAt(row, 1)->addWidget(
            std::make_unique<Wt::WText>(sp.productName));
        invTable->elementAt(row, 2)->addWidget(
            std::make_unique<Wt::WText>(sp.productCategory));

        std::ostringstream price;
        price << "$" << std::fixed << std::setprecision(2) << sp.unitPrice;
        invTable->elementAt(row, 3)->addWidget(std::make_unique<Wt::WText>(price.str()));

        invTable->elementAt(row, 4)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(sp.stockQty)));
        invTable->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(sp.inStock ? "Yes" : "No"));

        if (sp.bulkDiscount > 0) {
            std::ostringstream disc;
            disc << std::fixed << std::setprecision(0) << sp.bulkDiscount << "% (>="
                 << sp.bulkThreshold << ")";
            invTable->elementAt(row, 6)->addWidget(std::make_unique<Wt::WText>(disc.str()));
        } else {
            invTable->elementAt(row, 6)->addWidget(std::make_unique<Wt::WText>("-"));
        }

        invTable->elementAt(row, 7)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(sp.minOrderQty)));

        if (!sp.inStock) {
            for (int col = 0; col < 8; ++col)
                invTable->elementAt(row, col)->addStyleClass("out-of-stock");
        }

        ++row;
    }

    auto closeBtn = dialog->footer()->addWidget(
        std::make_unique<Wt::WPushButton>("Close"));
    closeBtn->addStyleClass("btn btn-secondary");
    closeBtn->clicked().connect(dialog, &Wt::WDialog::reject);

    dialog->show();
}

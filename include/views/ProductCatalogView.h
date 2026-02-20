#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WComboBox.h>
#include <Wt/WLineEdit.h>

class DataProvider;

/// Product catalog view – browse products, filter by category, view suppliers.
class ProductCatalogView : public Wt::WContainerWidget {
public:
    ProductCatalogView(DataProvider& provider);

private:
    DataProvider& provider_;

    Wt::WComboBox* categoryFilter_ = nullptr;
    Wt::WLineEdit* searchBox_      = nullptr;
    Wt::WTable*    table_          = nullptr;

    void buildUI();
    void refreshTable();
    void showSupplierComparison(long long productId);
};

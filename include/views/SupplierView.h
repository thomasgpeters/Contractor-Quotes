#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>

class DataProvider;

/// Supplier directory view – list all suppliers and their details.
class SupplierView : public Wt::WContainerWidget {
public:
    SupplierView(DataProvider& provider);

private:
    DataProvider& provider_;

    Wt::WTable* table_ = nullptr;

    void buildUI();
    void refreshTable();
    void showSupplierDetail(long long supplierId);
};

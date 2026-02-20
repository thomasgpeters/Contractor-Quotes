#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>

class Session;

/// Supplier directory view – list all suppliers and their details.
class SupplierView : public Wt::WContainerWidget {
public:
    SupplierView(Session& session);

private:
    Session& session_;

    Wt::WTable* table_ = nullptr;

    void buildUI();
    void refreshTable();
    void showSupplierDetail(long long supplierId);
};

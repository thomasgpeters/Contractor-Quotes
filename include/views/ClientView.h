#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WDialog.h>

class DataProvider;

/// Client management view – list, add, edit, and remove clients.
class ClientView : public Wt::WContainerWidget {
public:
    ClientView(DataProvider& provider);

private:
    DataProvider& provider_;

    Wt::WTable* table_ = nullptr;

    void buildUI();
    void refreshTable();
    void showAddDialog();
    void showEditDialog(long long clientId);
    void deleteClient(long long clientId);
};

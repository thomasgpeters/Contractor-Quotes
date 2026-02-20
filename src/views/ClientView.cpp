#include "views/ClientView.h"
#include "data/DataProvider.h"
#include <Wt/WText.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WDialog.h>
#include <Wt/WBreak.h>
#include <Wt/WTemplate.h>
#include <Wt/WMessageBox.h>
#include <Wt/WLabel.h>

ClientView::ClientView(DataProvider& provider)
    : provider_(provider)
{
    addStyleClass("client-view");
    buildUI();
}

void ClientView::buildUI()
{
    addWidget(std::make_unique<Wt::WText>("<h2>Client Management</h2>"));

    auto toolbar = addWidget(std::make_unique<Wt::WContainerWidget>());
    toolbar->addStyleClass("toolbar");

    auto addBtn = toolbar->addWidget(std::make_unique<Wt::WPushButton>("+ Add Client"));
    addBtn->addStyleClass("btn btn-primary");
    addBtn->clicked().connect(this, &ClientView::showAddDialog);

    table_ = addWidget(std::make_unique<Wt::WTable>());
    table_->addStyleClass("table table-striped");

    refreshTable();
}

void ClientView::refreshTable()
{
    table_->clear();
    table_->setHeaderCount(1);

    table_->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Name"));
    table_->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Company"));
    table_->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("City"));
    table_->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("State"));
    table_->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Phone"));
    table_->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Email"));
    table_->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Quotes"));
    table_->elementAt(0, 7)->addWidget(std::make_unique<Wt::WText>("Actions"));

    auto clients = provider_.findAllClients();

    int row = 1;
    for (auto& c : clients) {
        table_->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(c.name));
        table_->elementAt(row, 1)->addWidget(std::make_unique<Wt::WText>(c.company));
        table_->elementAt(row, 2)->addWidget(std::make_unique<Wt::WText>(c.city));
        table_->elementAt(row, 3)->addWidget(std::make_unique<Wt::WText>(c.state));
        table_->elementAt(row, 4)->addWidget(std::make_unique<Wt::WText>(c.phone));
        table_->elementAt(row, 5)->addWidget(std::make_unique<Wt::WText>(c.email));
        table_->elementAt(row, 6)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(c.quoteCount)));

        auto actionsDiv = table_->elementAt(row, 7)->addWidget(
            std::make_unique<Wt::WContainerWidget>());
        actionsDiv->addStyleClass("action-buttons");

        long long cid = c.id;

        auto editBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Edit"));
        editBtn->addStyleClass("btn btn-sm btn-secondary");
        editBtn->clicked().connect([this, cid] { showEditDialog(cid); });

        auto delBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        delBtn->addStyleClass("btn btn-sm btn-danger");
        delBtn->clicked().connect([this, cid] { deleteClient(cid); });

        ++row;
    }
}

void ClientView::showAddDialog()
{
    auto dialog = addChild(std::make_unique<Wt::WDialog>("Add New Client"));
    dialog->setModal(true);
    dialog->setClosable(true);
    dialog->rejectWhenEscapePressed();
    dialog->contents()->addStyleClass("dialog-content");

    auto addField = [&](const std::string& label) -> Wt::WLineEdit* {
        dialog->contents()->addWidget(std::make_unique<Wt::WLabel>(label));
        auto edit = dialog->contents()->addWidget(std::make_unique<Wt::WLineEdit>());
        edit->addStyleClass("form-control");
        dialog->contents()->addWidget(std::make_unique<Wt::WBreak>());
        return edit;
    };

    auto nameEdit    = addField("Name");
    auto companyEdit = addField("Company");
    auto addressEdit = addField("Address");
    auto cityEdit    = addField("City");
    auto stateEdit   = addField("State");
    auto zipEdit     = addField("Zip Code");
    auto phoneEdit   = addField("Phone");
    auto emailEdit   = addField("Email");

    auto footer = dialog->footer();
    auto saveBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Save"));
    saveBtn->addStyleClass("btn btn-primary");
    auto cancelBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Cancel"));
    cancelBtn->addStyleClass("btn btn-secondary");

    saveBtn->clicked().connect([=] {
        ClientDTO dto;
        dto.name    = nameEdit->text().toUTF8();
        dto.company = companyEdit->text().toUTF8();
        dto.address = addressEdit->text().toUTF8();
        dto.city    = cityEdit->text().toUTF8();
        dto.state   = stateEdit->text().toUTF8();
        dto.zipCode = zipEdit->text().toUTF8();
        dto.phone   = phoneEdit->text().toUTF8();
        dto.email   = emailEdit->text().toUTF8();
        provider_.createClient(dto);

        dialog->accept();
        refreshTable();
    });

    cancelBtn->clicked().connect(dialog, &Wt::WDialog::reject);
    dialog->show();
}

void ClientView::showEditDialog(long long clientId)
{
    auto client = provider_.findClientById(clientId);
    if (!client) return;

    auto dialog = addChild(std::make_unique<Wt::WDialog>("Edit Client"));
    dialog->setModal(true);
    dialog->setClosable(true);
    dialog->rejectWhenEscapePressed();
    dialog->contents()->addStyleClass("dialog-content");

    auto addField = [&](const std::string& label, const std::string& value) -> Wt::WLineEdit* {
        dialog->contents()->addWidget(std::make_unique<Wt::WLabel>(label));
        auto edit = dialog->contents()->addWidget(std::make_unique<Wt::WLineEdit>());
        edit->setText(value);
        edit->addStyleClass("form-control");
        dialog->contents()->addWidget(std::make_unique<Wt::WBreak>());
        return edit;
    };

    auto nameEdit    = addField("Name",     client->name);
    auto companyEdit = addField("Company",  client->company);
    auto addressEdit = addField("Address",  client->address);
    auto cityEdit    = addField("City",     client->city);
    auto stateEdit   = addField("State",    client->state);
    auto zipEdit     = addField("Zip Code", client->zipCode);
    auto phoneEdit   = addField("Phone",    client->phone);
    auto emailEdit   = addField("Email",    client->email);

    auto footer = dialog->footer();
    auto saveBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Save"));
    saveBtn->addStyleClass("btn btn-primary");
    auto cancelBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Cancel"));
    cancelBtn->addStyleClass("btn btn-secondary");

    saveBtn->clicked().connect([=] {
        ClientDTO dto;
        dto.name    = nameEdit->text().toUTF8();
        dto.company = companyEdit->text().toUTF8();
        dto.address = addressEdit->text().toUTF8();
        dto.city    = cityEdit->text().toUTF8();
        dto.state   = stateEdit->text().toUTF8();
        dto.zipCode = zipEdit->text().toUTF8();
        dto.phone   = phoneEdit->text().toUTF8();
        dto.email   = emailEdit->text().toUTF8();
        provider_.updateClient(clientId, dto);

        dialog->accept();
        refreshTable();
    });

    cancelBtn->clicked().connect(dialog, &Wt::WDialog::reject);
    dialog->show();
}

void ClientView::deleteClient(long long clientId)
{
    auto msgBox = addChild(std::make_unique<Wt::WMessageBox>(
        "Confirm Delete",
        "Are you sure you want to delete this client? "
        "All associated quotes will also be removed.",
        Wt::Icon::Warning,
        Wt::StandardButton::Yes | Wt::StandardButton::No));

    msgBox->buttonClicked().connect([=] {
        if (msgBox->buttonResult() == Wt::StandardButton::Yes) {
            provider_.deleteClient(clientId);
            refreshTable();
        }
        removeChild(msgBox);
    });

    msgBox->show();
}

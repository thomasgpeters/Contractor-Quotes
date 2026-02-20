#include "views/ClientView.h"
#include "models/Session.h"
#include "models/Client.h"
#include "models/Quote.h"
#include "models/QuoteLineItem.h"
#include <Wt/WText.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WDialog.h>
#include <Wt/WBreak.h>
#include <Wt/WTemplate.h>
#include <Wt/WMessageBox.h>
#include <Wt/WLabel.h>

ClientView::ClientView(Session& session)
    : session_(session)
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

    Wt::Dbo::Transaction t(session_.dbo());
    auto clients = session_.dbo().find<Client>().orderBy("name").resultList();

    int row = 1;
    for (auto& c : clients) {
        table_->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(c->name));
        table_->elementAt(row, 1)->addWidget(std::make_unique<Wt::WText>(c->company));
        table_->elementAt(row, 2)->addWidget(std::make_unique<Wt::WText>(c->city));
        table_->elementAt(row, 3)->addWidget(std::make_unique<Wt::WText>(c->state));
        table_->elementAt(row, 4)->addWidget(std::make_unique<Wt::WText>(c->phone));
        table_->elementAt(row, 5)->addWidget(std::make_unique<Wt::WText>(c->email));

        int quoteCount = static_cast<int>(c->quotes.size());
        table_->elementAt(row, 6)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(quoteCount)));

        auto actionsDiv = table_->elementAt(row, 7)->addWidget(
            std::make_unique<Wt::WContainerWidget>());
        actionsDiv->addStyleClass("action-buttons");

        long long cid = c.id();

        auto editBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Edit"));
        editBtn->addStyleClass("btn btn-sm btn-secondary");
        editBtn->clicked().connect([this, cid] { showEditDialog(cid); });

        auto delBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        delBtn->addStyleClass("btn btn-sm btn-danger");
        delBtn->clicked().connect([this, cid] { deleteClient(cid); });

        ++row;
    }
    t.commit();
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
        Wt::Dbo::Transaction t(session_.dbo());
        auto c = session_.dbo().addNew<Client>();
        c.modify()->name    = nameEdit->text().toUTF8();
        c.modify()->company = companyEdit->text().toUTF8();
        c.modify()->address = addressEdit->text().toUTF8();
        c.modify()->city    = cityEdit->text().toUTF8();
        c.modify()->state   = stateEdit->text().toUTF8();
        c.modify()->zipCode = zipEdit->text().toUTF8();
        c.modify()->phone   = phoneEdit->text().toUTF8();
        c.modify()->email   = emailEdit->text().toUTF8();
        t.commit();

        dialog->accept();
        refreshTable();
    });

    cancelBtn->clicked().connect(dialog, &Wt::WDialog::reject);
    dialog->show();
}

void ClientView::showEditDialog(long long clientId)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto client = session_.dbo().find<Client>().where("id = ?").bind(clientId).resultValue();
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

    auto nameEdit    = addField("Name",    client->name);
    auto companyEdit = addField("Company", client->company);
    auto addressEdit = addField("Address", client->address);
    auto cityEdit    = addField("City",    client->city);
    auto stateEdit   = addField("State",   client->state);
    auto zipEdit     = addField("Zip Code",client->zipCode);
    auto phoneEdit   = addField("Phone",   client->phone);
    auto emailEdit   = addField("Email",   client->email);
    t.commit();

    auto footer = dialog->footer();
    auto saveBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Save"));
    saveBtn->addStyleClass("btn btn-primary");
    auto cancelBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Cancel"));
    cancelBtn->addStyleClass("btn btn-secondary");

    saveBtn->clicked().connect([=] {
        Wt::Dbo::Transaction t2(session_.dbo());
        auto c = session_.dbo().find<Client>().where("id = ?").bind(clientId).resultValue();
        c.modify()->name    = nameEdit->text().toUTF8();
        c.modify()->company = companyEdit->text().toUTF8();
        c.modify()->address = addressEdit->text().toUTF8();
        c.modify()->city    = cityEdit->text().toUTF8();
        c.modify()->state   = stateEdit->text().toUTF8();
        c.modify()->zipCode = zipEdit->text().toUTF8();
        c.modify()->phone   = phoneEdit->text().toUTF8();
        c.modify()->email   = emailEdit->text().toUTF8();
        t2.commit();

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
            Wt::Dbo::Transaction t(session_.dbo());
            // Delete line items for all quotes belonging to this client
            session_.dbo().execute(
                "DELETE FROM quote_line_item WHERE quote_id IN "
                "(SELECT id FROM quote WHERE client_id = ?)"
            ).bind(clientId);
            // Delete quotes belonging to this client
            session_.dbo().execute(
                "DELETE FROM quote WHERE client_id = ?"
            ).bind(clientId);
            // Delete client
            session_.dbo().execute(
                "DELETE FROM client WHERE id = ?"
            ).bind(clientId);
            t.commit();
            refreshTable();
        }
        removeChild(msgBox);
    });

    msgBox->show();
}

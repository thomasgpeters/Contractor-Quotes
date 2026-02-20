#include "views/QuoteBuilderView.h"
#include "models/Session.h"
#include "models/Product.h"
#include "models/Supplier.h"
#include "models/SupplierProduct.h"
#include "models/Client.h"
#include "models/Quote.h"
#include "models/QuoteLineItem.h"
#include "engine/SourcingEngine.h"
#include <Wt/WText.h>
#include <Wt/WBreak.h>
#include <Wt/WDialog.h>
#include <Wt/WLabel.h>
#include <Wt/WMessageBox.h>
#include <Wt/WGroupBox.h>
#include <sstream>
#include <iomanip>

QuoteBuilderView::QuoteBuilderView(Session& session)
    : session_(session)
{
    addStyleClass("quote-builder-view");
    buildUI();
}

void QuoteBuilderView::buildUI()
{
    addWidget(std::make_unique<Wt::WText>("<h2>Quote Builder</h2>"));
    addWidget(std::make_unique<Wt::WText>(
        "<p>Create quotes for clients with automatic best-source selection for each material.</p>"));

    // Top area: quote list + new quote button
    auto toolbar = addWidget(std::make_unique<Wt::WContainerWidget>());
    toolbar->addStyleClass("toolbar");

    auto newBtn = toolbar->addWidget(std::make_unique<Wt::WPushButton>("+ New Quote"));
    newBtn->addStyleClass("btn btn-primary");
    newBtn->clicked().connect(this, &QuoteBuilderView::createNewQuote);

    quoteListTable_ = addWidget(std::make_unique<Wt::WTable>());
    quoteListTable_->addStyleClass("table table-striped");

    // Editor panel (hidden until a quote is selected)
    editorPanel_ = addWidget(std::make_unique<Wt::WContainerWidget>());
    editorPanel_->addStyleClass("quote-editor");
    editorPanel_->hide();

    // Quote details form
    auto detailsBox = editorPanel_->addWidget(std::make_unique<Wt::WGroupBox>("Quote Details"));
    detailsBox->addStyleClass("form-group-box");

    auto addFormRow = [&](Wt::WContainerWidget* parent, const std::string& label) {
        auto row = parent->addWidget(std::make_unique<Wt::WContainerWidget>());
        row->addStyleClass("form-row");
        row->addWidget(std::make_unique<Wt::WLabel>(label));
        return row;
    };

    auto row1 = addFormRow(detailsBox, "Title:");
    titleEdit_ = row1->addWidget(std::make_unique<Wt::WLineEdit>());
    titleEdit_->addStyleClass("form-control");

    auto row2 = addFormRow(detailsBox, "Description:");
    descEdit_ = row2->addWidget(std::make_unique<Wt::WTextArea>());
    descEdit_->addStyleClass("form-control");
    descEdit_->setRows(2);

    auto row3 = addFormRow(detailsBox, "Client:");
    clientCombo_ = row3->addWidget(std::make_unique<Wt::WComboBox>());
    clientCombo_->addStyleClass("form-control");

    auto row4 = addFormRow(detailsBox, "Status:");
    statusCombo_ = row4->addWidget(std::make_unique<Wt::WComboBox>());
    statusCombo_->addStyleClass("form-control");
    statusCombo_->addItem("Draft");
    statusCombo_->addItem("Sent");
    statusCombo_->addItem("Accepted");
    statusCombo_->addItem("Rejected");
    statusCombo_->addItem("Expired");

    auto row5 = addFormRow(detailsBox, "Tax Rate (%):");
    taxRateSpin_ = row5->addWidget(std::make_unique<Wt::WDoubleSpinBox>());
    taxRateSpin_->setRange(0, 25);
    taxRateSpin_->setValue(8.25);
    taxRateSpin_->setSingleStep(0.25);
    taxRateSpin_->addStyleClass("form-control spin-control");

    auto row6 = addFormRow(detailsBox, "Markup (%):");
    markupRateSpin_ = row6->addWidget(std::make_unique<Wt::WDoubleSpinBox>());
    markupRateSpin_->setRange(0, 100);
    markupRateSpin_->setValue(15.0);
    markupRateSpin_->setSingleStep(1);
    markupRateSpin_->addStyleClass("form-control spin-control");

    auto row7 = addFormRow(detailsBox, "Notes:");
    notesEdit_ = row7->addWidget(std::make_unique<Wt::WTextArea>());
    notesEdit_->addStyleClass("form-control");
    notesEdit_->setRows(2);

    auto saveBar = detailsBox->addWidget(std::make_unique<Wt::WContainerWidget>());
    saveBar->addStyleClass("toolbar");
    auto saveBtn = saveBar->addWidget(std::make_unique<Wt::WPushButton>("Save Quote"));
    saveBtn->addStyleClass("btn btn-primary");
    saveBtn->clicked().connect(this, &QuoteBuilderView::saveQuote);

    // Line items section
    auto lineBox = editorPanel_->addWidget(std::make_unique<Wt::WGroupBox>("Line Items"));
    lineBox->addStyleClass("form-group-box");

    auto lineToolbar = lineBox->addWidget(std::make_unique<Wt::WContainerWidget>());
    lineToolbar->addStyleClass("toolbar");

    auto addLineBtn = lineToolbar->addWidget(std::make_unique<Wt::WPushButton>("+ Add Item"));
    addLineBtn->addStyleClass("btn btn-primary");
    addLineBtn->clicked().connect(this, &QuoteBuilderView::addLineItem);

    auto sourceAllBtn = lineToolbar->addWidget(std::make_unique<Wt::WPushButton>("Find Best Sources for All"));
    sourceAllBtn->addStyleClass("btn btn-success");
    sourceAllBtn->clicked().connect(this, &QuoteBuilderView::runSourcingForAll);

    lineItemTable_ = lineBox->addWidget(std::make_unique<Wt::WTable>());
    lineItemTable_->addStyleClass("table table-striped");

    // Totals section
    auto totalsBox = editorPanel_->addWidget(std::make_unique<Wt::WGroupBox>("Quote Totals"));
    totalsBox->addStyleClass("form-group-box totals-box");

    subtotalText_ = totalsBox->addWidget(std::make_unique<Wt::WText>("Subtotal: $0.00"));
    subtotalText_->addStyleClass("total-line");
    totalsBox->addWidget(std::make_unique<Wt::WBreak>());
    taxText_ = totalsBox->addWidget(std::make_unique<Wt::WText>("Tax: $0.00"));
    taxText_->addStyleClass("total-line");
    totalsBox->addWidget(std::make_unique<Wt::WBreak>());
    totalText_ = totalsBox->addWidget(std::make_unique<Wt::WText>("Total: $0.00"));
    totalText_->addStyleClass("total-line grand-total");

    refreshQuoteList();
}

void QuoteBuilderView::populateClientCombo()
{
    clientCombo_->clear();
    clientCombo_->addItem("-- Select Client --");

    Wt::Dbo::Transaction t(session_.dbo());
    auto clients = session_.dbo().find<Client>().orderBy("name").resultList();
    for (auto& c : clients) {
        clientCombo_->addItem(c->name + " (" + c->company + ")");
    }
    t.commit();
}

void QuoteBuilderView::refreshQuoteList()
{
    quoteListTable_->clear();
    quoteListTable_->setHeaderCount(1);

    quoteListTable_->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Title"));
    quoteListTable_->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Client"));
    quoteListTable_->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Status"));
    quoteListTable_->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Items"));
    quoteListTable_->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Total"));
    quoteListTable_->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Created"));
    quoteListTable_->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Actions"));

    Wt::Dbo::Transaction t(session_.dbo());
    auto quotes = session_.dbo().find<Quote>()
        .orderBy("created_date desc").resultList();

    int row = 1;
    for (auto& q : quotes) {
        quoteListTable_->elementAt(row, 0)->addWidget(
            std::make_unique<Wt::WText>(q->title));

        std::string clientName = q->client ? q->client->name : "(none)";
        quoteListTable_->elementAt(row, 1)->addWidget(
            std::make_unique<Wt::WText>(clientName));

        std::string statusStr;
        switch (q->status) {
            case Quote::Status::Draft:    statusStr = "Draft"; break;
            case Quote::Status::Sent:     statusStr = "Sent"; break;
            case Quote::Status::Accepted: statusStr = "Accepted"; break;
            case Quote::Status::Rejected: statusStr = "Rejected"; break;
            case Quote::Status::Expired:  statusStr = "Expired"; break;
        }
        quoteListTable_->elementAt(row, 2)->addWidget(
            std::make_unique<Wt::WText>(statusStr));

        int itemCount = 0;
        double total = 0;
        for (const auto& li : q->lineItems) {
            ++itemCount;
            total += li->lineTotal;
        }

        quoteListTable_->elementAt(row, 3)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(itemCount)));

        std::ostringstream ts;
        ts << "$" << std::fixed << std::setprecision(2) << total;
        quoteListTable_->elementAt(row, 4)->addWidget(
            std::make_unique<Wt::WText>(ts.str()));

        std::string dateStr = q->createdDate.isValid()
            ? q->createdDate.toString("yyyy-MM-dd").toUTF8() : "-";
        quoteListTable_->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(dateStr));

        auto actionsDiv = quoteListTable_->elementAt(row, 6)->addWidget(
            std::make_unique<Wt::WContainerWidget>());
        actionsDiv->addStyleClass("action-buttons");

        long long qid = q.id();

        auto openBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Open"));
        openBtn->addStyleClass("btn btn-sm btn-primary");
        openBtn->clicked().connect([this, qid] { openQuote(qid); });

        auto delBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        delBtn->addStyleClass("btn btn-sm btn-danger");
        delBtn->clicked().connect([this, qid] { deleteQuote(qid); });

        ++row;
    }

    t.commit();
}

void QuoteBuilderView::createNewQuote()
{
    Wt::Dbo::Transaction t(session_.dbo());

    auto q = session_.dbo().addNew<Quote>();
    q.modify()->title       = "New Quote";
    q.modify()->createdDate = Wt::WDateTime::currentDateTime();
    q.modify()->status      = Quote::Status::Draft;
    q.modify()->taxRate     = 8.25;
    q.modify()->markupRate  = 15.0;

    long long newId = q.id();
    t.commit();

    refreshQuoteList();
    openQuote(newId);
}

void QuoteBuilderView::openQuote(long long quoteId)
{
    currentQuoteId_ = quoteId;
    editorPanel_->show();

    populateClientCombo();

    Wt::Dbo::Transaction t(session_.dbo());
    auto q = session_.dbo().find<Quote>()
        .where("id = ?").bind(quoteId).resultValue();
    if (!q) return;

    titleEdit_->setText(q->title);
    descEdit_->setText(q->description);
    statusCombo_->setCurrentIndex(static_cast<int>(q->status));
    taxRateSpin_->setValue(q->taxRate);
    markupRateSpin_->setValue(q->markupRate);
    notesEdit_->setText(q->notes);

    // Select client in combo
    if (q->client) {
        std::string clientLabel = q->client->name + " (" + q->client->company + ")";
        for (int i = 0; i < clientCombo_->count(); ++i) {
            if (clientCombo_->itemText(i).toUTF8() == clientLabel) {
                clientCombo_->setCurrentIndex(i);
                break;
            }
        }
    } else {
        clientCombo_->setCurrentIndex(0);
    }

    t.commit();

    refreshLineItems();
    updateTotals();
}

void QuoteBuilderView::saveQuote()
{
    if (currentQuoteId_ < 0) return;

    Wt::Dbo::Transaction t(session_.dbo());
    auto q = session_.dbo().find<Quote>()
        .where("id = ?").bind(currentQuoteId_).resultValue();
    if (!q) return;

    q.modify()->title       = titleEdit_->text().toUTF8();
    q.modify()->description = descEdit_->text().toUTF8();
    q.modify()->status      = static_cast<Quote::Status>(statusCombo_->currentIndex());
    q.modify()->taxRate     = taxRateSpin_->value();
    q.modify()->markupRate  = markupRateSpin_->value();
    q.modify()->notes       = notesEdit_->text().toUTF8();

    // Resolve client from combo
    if (clientCombo_->currentIndex() > 0) {
        auto clients = session_.dbo().find<Client>().orderBy("name").resultList();
        int idx = 1;
        for (auto& c : clients) {
            if (idx == clientCombo_->currentIndex()) {
                q.modify()->client = c;
                break;
            }
            ++idx;
        }
    }

    t.commit();
    refreshQuoteList();
}

void QuoteBuilderView::deleteQuote(long long quoteId)
{
    auto msgBox = addChild(std::make_unique<Wt::WMessageBox>(
        "Confirm Delete",
        "Delete this quote and all its line items?",
        Wt::Icon::Warning,
        Wt::StandardButton::Yes | Wt::StandardButton::No));

    msgBox->buttonClicked().connect([=] {
        if (msgBox->buttonResult() == Wt::StandardButton::Yes) {
            Wt::Dbo::Transaction t(session_.dbo());
            auto lineItems = session_.dbo().find<QuoteLineItem>()
                .where("quote_id = ?").bind(quoteId).resultList();
            for (auto& li : lineItems) {
                li.remove();
            }
            auto q = session_.dbo().find<Quote>()
                .where("id = ?").bind(quoteId).resultValue();
            if (q) q.remove();
            t.commit();

            if (currentQuoteId_ == quoteId) {
                currentQuoteId_ = -1;
                editorPanel_->hide();
            }
            refreshQuoteList();
        }
        removeChild(msgBox);
    });

    msgBox->show();
}

void QuoteBuilderView::refreshLineItems()
{
    lineItemTable_->clear();
    lineItemTable_->setHeaderCount(1);

    lineItemTable_->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("#"));
    lineItemTable_->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Product"));
    lineItemTable_->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Qty"));
    lineItemTable_->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Best Supplier"));
    lineItemTable_->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Unit Price"));
    lineItemTable_->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Markup %"));
    lineItemTable_->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Line Total"));
    lineItemTable_->elementAt(0, 7)->addWidget(std::make_unique<Wt::WText>("Actions"));

    if (currentQuoteId_ < 0) return;

    Wt::Dbo::Transaction t(session_.dbo());
    auto lineItems = session_.dbo().find<QuoteLineItem>()
        .where("quote_id = ?").bind(currentQuoteId_)
        .resultList();

    int row = 1;
    for (auto& li : lineItems) {
        lineItemTable_->elementAt(row, 0)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(row)));

        std::string prodName = li->product ? li->product->name : "(select product)";
        lineItemTable_->elementAt(row, 1)->addWidget(
            std::make_unique<Wt::WText>(prodName));
        lineItemTable_->elementAt(row, 2)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(li->quantity)));

        std::string supplierName = li->supplier ? li->supplier->name : "(not sourced)";
        lineItemTable_->elementAt(row, 3)->addWidget(
            std::make_unique<Wt::WText>(supplierName));

        std::ostringstream upStr, ltStr;
        upStr << "$" << std::fixed << std::setprecision(2) << li->unitPrice;
        ltStr << "$" << std::fixed << std::setprecision(2) << li->lineTotal;

        lineItemTable_->elementAt(row, 4)->addWidget(
            std::make_unique<Wt::WText>(upStr.str()));

        std::ostringstream mkStr;
        mkStr << std::fixed << std::setprecision(1) << li->markup << "%";
        lineItemTable_->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(mkStr.str()));

        lineItemTable_->elementAt(row, 6)->addWidget(
            std::make_unique<Wt::WText>(ltStr.str()));

        auto actionsDiv = lineItemTable_->elementAt(row, 7)->addWidget(
            std::make_unique<Wt::WContainerWidget>());
        actionsDiv->addStyleClass("action-buttons");

        long long liId = li.id();

        auto sourceBtn = actionsDiv->addWidget(
            std::make_unique<Wt::WPushButton>("Find Source"));
        sourceBtn->addStyleClass("btn btn-sm btn-info");
        sourceBtn->clicked().connect([this, liId] { runSourcing(liId); });

        auto removeBtn = actionsDiv->addWidget(
            std::make_unique<Wt::WPushButton>("Remove"));
        removeBtn->addStyleClass("btn btn-sm btn-danger");
        removeBtn->clicked().connect([this, liId] { removeLineItem(liId); });

        ++row;
    }

    t.commit();
}

void QuoteBuilderView::addLineItem()
{
    if (currentQuoteId_ < 0) return;

    // Dialog to select product and quantity
    auto dialog = addChild(std::make_unique<Wt::WDialog>("Add Line Item"));
    dialog->setModal(true);
    dialog->setClosable(true);
    dialog->rejectWhenEscapePressed();
    dialog->setWidth(Wt::WLength(500));
    dialog->contents()->addStyleClass("dialog-content");

    dialog->contents()->addWidget(std::make_unique<Wt::WLabel>("Product:"));
    auto prodCombo = dialog->contents()->addWidget(std::make_unique<Wt::WComboBox>());
    prodCombo->addStyleClass("form-control");

    std::vector<long long> productIds;
    {
        Wt::Dbo::Transaction t(session_.dbo());
        auto products = session_.dbo().find<Product>().orderBy("category, name").resultList();
        for (auto& p : products) {
            prodCombo->addItem("[" + p->category + "] " + p->name + " (" + p->sku + ")");
            productIds.push_back(p.id());
        }
        t.commit();
    }

    dialog->contents()->addWidget(std::make_unique<Wt::WBreak>());
    dialog->contents()->addWidget(std::make_unique<Wt::WLabel>("Quantity:"));
    auto qtySpin = dialog->contents()->addWidget(std::make_unique<Wt::WSpinBox>());
    qtySpin->setRange(1, 10000);
    qtySpin->setValue(1);
    qtySpin->addStyleClass("form-control");

    dialog->contents()->addWidget(std::make_unique<Wt::WBreak>());
    dialog->contents()->addWidget(std::make_unique<Wt::WLabel>("Markup (%):"));
    auto markupSpin = dialog->contents()->addWidget(std::make_unique<Wt::WDoubleSpinBox>());
    markupSpin->setRange(0, 100);
    markupSpin->setValue(markupRateSpin_->value());
    markupSpin->addStyleClass("form-control");

    auto footer = dialog->footer();
    auto addBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Add"));
    addBtn->addStyleClass("btn btn-primary");
    auto cancelBtn = footer->addWidget(std::make_unique<Wt::WPushButton>("Cancel"));
    cancelBtn->addStyleClass("btn btn-secondary");

    addBtn->clicked().connect([=] {
        if (prodCombo->currentIndex() < 0 ||
            static_cast<size_t>(prodCombo->currentIndex()) >= productIds.size())
            return;

        long long selectedProductId = productIds[prodCombo->currentIndex()];
        int qty = qtySpin->value();
        double markup = markupSpin->value();

        Wt::Dbo::Transaction t(session_.dbo());

        auto quote = session_.dbo().find<Quote>()
            .where("id = ?").bind(currentQuoteId_).resultValue();
        auto product = session_.dbo().find<Product>()
            .where("id = ?").bind(selectedProductId).resultValue();

        auto li = session_.dbo().addNew<QuoteLineItem>();
        li.modify()->quote    = quote;
        li.modify()->product  = product;
        li.modify()->quantity = qty;
        li.modify()->markup   = markup;
        // Price and supplier will be set when sourcing is run

        t.commit();

        dialog->accept();
        refreshLineItems();
        updateTotals();
    });

    cancelBtn->clicked().connect(dialog, &Wt::WDialog::reject);
    dialog->show();
}

void QuoteBuilderView::removeLineItem(long long lineItemId)
{
    Wt::Dbo::Transaction t(session_.dbo());
    auto li = session_.dbo().find<QuoteLineItem>()
        .where("id = ?").bind(lineItemId).resultValue();
    if (li) li.remove();
    t.commit();

    refreshLineItems();
    updateTotals();
}

void QuoteBuilderView::runSourcing(long long lineItemId)
{
    Wt::Dbo::Transaction t(session_.dbo());

    auto li = session_.dbo().find<QuoteLineItem>()
        .where("id = ?").bind(lineItemId).resultValue();
    if (!li || !li->product) return;

    // Determine job site location from the quote's client, or default
    double jobLat = 30.267, jobLon = -97.743; // Austin TX default
    auto quote = li->quote;
    if (quote && quote->client) {
        if (quote->client->latitude != 0.0) {
            jobLat = quote->client->latitude;
            jobLon = quote->client->longitude;
        }
    }

    SourcingEngine engine(session_.dbo());
    auto results = engine.findBestSources(
        li->product.id(), li->quantity, jobLat, jobLon);

    if (!results.empty()) {
        auto& best = results[0];
        li.modify()->unitPrice = best.effectivePrice;
        li.modify()->supplier  = best.supplierProduct->supplier;
        li.modify()->computeTotal();

        // Show sourcing results dialog
        auto dialog = addChild(std::make_unique<Wt::WDialog>(
            "Sourcing Results: " + li->product->name));
        dialog->setModal(true);
        dialog->setClosable(true);
        dialog->rejectWhenEscapePressed();
        dialog->setWidth(Wt::WLength(800));
        dialog->contents()->addStyleClass("dialog-content");

        std::ostringstream header;
        header << "<p><strong>Product:</strong> " << li->product->name
               << " | <strong>Quantity:</strong> " << li->quantity
               << " | <strong>Best Source:</strong> " << best.supplierName << "</p>";
        dialog->contents()->addWidget(std::make_unique<Wt::WText>(header.str()));

        auto table = dialog->contents()->addWidget(std::make_unique<Wt::WTable>());
        table->addStyleClass("table table-striped");
        table->setHeaderCount(1);

        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Rank"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Supplier"));
        table->elementAt(0, 2)->addWidget(std::make_unique<Wt::WText>("Price"));
        table->elementAt(0, 3)->addWidget(std::make_unique<Wt::WText>("Stock"));
        table->elementAt(0, 4)->addWidget(std::make_unique<Wt::WText>("Distance"));
        table->elementAt(0, 5)->addWidget(std::make_unique<Wt::WText>("Rating"));
        table->elementAt(0, 6)->addWidget(std::make_unique<Wt::WText>("Score"));
        table->elementAt(0, 7)->addWidget(std::make_unique<Wt::WText>("Select"));

        int row = 1;
        for (auto& r : results) {
            table->elementAt(row, 0)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(row)));
            table->elementAt(row, 1)->addWidget(
                std::make_unique<Wt::WText>(r.supplierName));

            std::ostringstream ep, dist, sc;
            ep << "$" << std::fixed << std::setprecision(2) << r.effectivePrice;
            dist << std::fixed << std::setprecision(1) << r.distanceMiles << " mi";
            sc << std::fixed << std::setprecision(3) << r.compositeScore;

            table->elementAt(row, 2)->addWidget(std::make_unique<Wt::WText>(ep.str()));
            table->elementAt(row, 3)->addWidget(
                std::make_unique<Wt::WText>(
                    std::to_string(r.availableQty) + (r.meetsQty ? "" : " (short)")));
            table->elementAt(row, 4)->addWidget(std::make_unique<Wt::WText>(dist.str()));

            std::ostringstream rat;
            rat << std::fixed << std::setprecision(1) << r.supplierRating;
            table->elementAt(row, 5)->addWidget(std::make_unique<Wt::WText>(rat.str()));
            table->elementAt(row, 6)->addWidget(std::make_unique<Wt::WText>(sc.str()));

            // Allow manual override
            auto selectBtn = table->elementAt(row, 7)->addWidget(
                std::make_unique<Wt::WPushButton>("Use This"));
            selectBtn->addStyleClass("btn btn-sm btn-success");

            auto supplierProduct = r.supplierProduct;
            double effPrice = r.effectivePrice;
            selectBtn->clicked().connect([=] {
                Wt::Dbo::Transaction t2(session_.dbo());
                auto item = session_.dbo().find<QuoteLineItem>()
                    .where("id = ?").bind(lineItemId).resultValue();
                if (item) {
                    item.modify()->unitPrice = effPrice;
                    item.modify()->supplier  = supplierProduct->supplier;
                    item.modify()->computeTotal();
                }
                t2.commit();

                dialog->accept();
                refreshLineItems();
                updateTotals();
            });

            if (row == 1) {
                for (int col = 0; col < 8; ++col)
                    table->elementAt(row, col)->addStyleClass("best-source");
            }

            ++row;
        }

        auto closeBtn = dialog->footer()->addWidget(
            std::make_unique<Wt::WPushButton>("Keep Best & Close"));
        closeBtn->addStyleClass("btn btn-primary");
        closeBtn->clicked().connect(dialog, &Wt::WDialog::accept);

        t.commit();
        dialog->show();
    } else {
        t.commit();
    }

    refreshLineItems();
    updateTotals();
}

void QuoteBuilderView::runSourcingForAll()
{
    if (currentQuoteId_ < 0) return;

    Wt::Dbo::Transaction t(session_.dbo());

    auto quote = session_.dbo().find<Quote>()
        .where("id = ?").bind(currentQuoteId_).resultValue();
    if (!quote) return;

    // Determine job site location
    double jobLat = 30.267, jobLon = -97.743;
    if (quote->client && quote->client->latitude != 0.0) {
        jobLat = quote->client->latitude;
        jobLon = quote->client->longitude;
    }

    SourcingEngine engine(session_.dbo());
    int sourced = 0;

    auto lineItems = session_.dbo().find<QuoteLineItem>()
        .where("quote_id = ?").bind(currentQuoteId_).resultList();

    for (auto& li : lineItems) {
        if (!li->product) continue;

        auto results = engine.findBestSources(
            li->product.id(), li->quantity, jobLat, jobLon);

        if (!results.empty()) {
            auto& best = results[0];
            li.modify()->unitPrice = best.effectivePrice;
            li.modify()->supplier  = best.supplierProduct->supplier;
            li.modify()->computeTotal();
            ++sourced;
        }
    }

    t.commit();

    refreshLineItems();
    updateTotals();

    auto msgBox = addChild(std::make_unique<Wt::WMessageBox>(
        "Sourcing Complete",
        "Found best sources for " + std::to_string(sourced) + " line item(s). "
        "Prices and suppliers have been updated.",
        Wt::Icon::Information,
        Wt::StandardButton::Ok));
    msgBox->buttonClicked().connect([=] { removeChild(msgBox); });
    msgBox->show();
}

void QuoteBuilderView::updateTotals()
{
    if (currentQuoteId_ < 0) return;

    Wt::Dbo::Transaction t(session_.dbo());

    double subtotal = 0;
    auto lineItems = session_.dbo().find<QuoteLineItem>()
        .where("quote_id = ?").bind(currentQuoteId_).resultList();
    for (auto& li : lineItems) {
        subtotal += li->lineTotal;
    }

    double taxRate = taxRateSpin_->value();
    double tax = subtotal * taxRate / 100.0;
    double total = subtotal + tax;

    std::ostringstream ss, ts, gs;
    ss << "Subtotal: $" << std::fixed << std::setprecision(2) << subtotal;
    ts << "Tax (" << std::fixed << std::setprecision(2) << taxRate << "%): $"
       << std::fixed << std::setprecision(2) << tax;
    gs << "Total: $" << std::fixed << std::setprecision(2) << total;

    subtotalText_->setText(ss.str());
    taxText_->setText(ts.str());
    totalText_->setText(gs.str());

    t.commit();
}

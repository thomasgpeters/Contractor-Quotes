#include "views/QuoteBuilderView.h"
#include "data/DataProvider.h"
#include "engine/SourcingEngine.h"
#include <Wt/WText.h>
#include <Wt/WBreak.h>
#include <Wt/WDialog.h>
#include <Wt/WLabel.h>
#include <Wt/WMessageBox.h>
#include <Wt/WGroupBox.h>
#include <sstream>
#include <iomanip>

static const char* statusLabel(int s) {
    switch (s) {
        case 0: return "Draft";
        case 1: return "Sent";
        case 2: return "Accepted";
        case 3: return "Rejected";
        case 4: return "Expired";
        default: return "?";
    }
}

QuoteBuilderView::QuoteBuilderView(DataProvider& provider)
    : provider_(provider)
{
    addStyleClass("quote-builder-view");
    buildUI();
}

void QuoteBuilderView::buildUI()
{
    listPanel_ = addWidget(std::make_unique<Wt::WContainerWidget>());

    listPanel_->addWidget(std::make_unique<Wt::WText>("<h2>Quote Builder</h2>"));
    listPanel_->addWidget(std::make_unique<Wt::WText>(
        "<p>Create quotes for clients with automatic best-source selection for each material.</p>"));

    auto toolbar = listPanel_->addWidget(std::make_unique<Wt::WContainerWidget>());
    toolbar->addStyleClass("toolbar");

    auto newBtn = toolbar->addWidget(std::make_unique<Wt::WPushButton>("+ New Quote"));
    newBtn->addStyleClass("btn btn-primary");
    newBtn->clicked().connect(this, &QuoteBuilderView::createNewQuote);

    quoteListTable_ = listPanel_->addWidget(std::make_unique<Wt::WTable>());
    quoteListTable_->addStyleClass("table table-striped");

    editorPanel_ = addWidget(std::make_unique<Wt::WContainerWidget>());
    editorPanel_->addStyleClass("quote-editor");
    editorPanel_->hide();

    auto editorToolbar = editorPanel_->addWidget(std::make_unique<Wt::WContainerWidget>());
    editorToolbar->addStyleClass("editor-nav");

    auto backBtn = editorToolbar->addWidget(std::make_unique<Wt::WPushButton>());
    backBtn->setTextFormat(Wt::TextFormat::XHTML);
    backBtn->setText("&#8592;");
    backBtn->addStyleClass("btn btn-link back-arrow");
    backBtn->setToolTip("Back to Quotes");
    backBtn->clicked().connect([this] {
        editorPanel_->hide();
        listPanel_->show();
        currentQuoteId_ = -1;
    });

    editorQuoteId_ = editorToolbar->addWidget(std::make_unique<Wt::WText>("Quote #0"));
    editorQuoteId_->addStyleClass("editor-quote-id");

    auto detailsBox = editorPanel_->addWidget(std::make_unique<Wt::WGroupBox>("Quote Details"));
    detailsBox->addStyleClass("form-group-box");

    auto addField = [&](Wt::WContainerWidget* parent, const std::string& label) {
        auto cell = parent->addWidget(std::make_unique<Wt::WContainerWidget>());
        cell->addStyleClass("form-cell");
        cell->addWidget(std::make_unique<Wt::WLabel>(label));
        return cell;
    };

    // 3-column grid for short fields
    auto grid3 = detailsBox->addWidget(std::make_unique<Wt::WContainerWidget>());
    grid3->addStyleClass("form-grid-3");

    auto cell1 = addField(grid3, "Title:");
    titleEdit_ = cell1->addWidget(std::make_unique<Wt::WLineEdit>());
    titleEdit_->addStyleClass("form-control");

    auto cell2 = addField(grid3, "Client:");
    clientCombo_ = cell2->addWidget(std::make_unique<Wt::WComboBox>());
    clientCombo_->addStyleClass("form-control");

    auto cell3 = addField(grid3, "Status:");
    statusCombo_ = cell3->addWidget(std::make_unique<Wt::WComboBox>());
    statusCombo_->addStyleClass("form-control");
    statusCombo_->addItem("Draft");
    statusCombo_->addItem("Sent");
    statusCombo_->addItem("Accepted");
    statusCombo_->addItem("Rejected");
    statusCombo_->addItem("Expired");

    auto cell4 = addField(grid3, "Tax Rate (%):");
    taxRateSpin_ = cell4->addWidget(std::make_unique<Wt::WDoubleSpinBox>());
    taxRateSpin_->setRange(0, 25);
    taxRateSpin_->setValue(8.25);
    taxRateSpin_->setSingleStep(0.25);
    taxRateSpin_->addStyleClass("form-control");

    auto cell5 = addField(grid3, "Markup (%):");
    markupRateSpin_ = cell5->addWidget(std::make_unique<Wt::WDoubleSpinBox>());
    markupRateSpin_->setRange(0, 100);
    markupRateSpin_->setValue(15.0);
    markupRateSpin_->setSingleStep(1);
    markupRateSpin_->addStyleClass("form-control");

    // 2-column grid for Description & Notes
    auto grid2 = detailsBox->addWidget(std::make_unique<Wt::WContainerWidget>());
    grid2->addStyleClass("form-grid-2");

    auto cellDesc = addField(grid2, "Description:");
    descEdit_ = cellDesc->addWidget(std::make_unique<Wt::WTextArea>());
    descEdit_->addStyleClass("form-control");
    descEdit_->setRows(3);

    auto cellNotes = addField(grid2, "Notes:");
    notesEdit_ = cellNotes->addWidget(std::make_unique<Wt::WTextArea>());
    notesEdit_->addStyleClass("form-control");
    notesEdit_->setRows(3);

    auto saveBar = detailsBox->addWidget(std::make_unique<Wt::WContainerWidget>());
    saveBar->addStyleClass("toolbar");
    auto saveBtn = saveBar->addWidget(std::make_unique<Wt::WPushButton>("Save Quote"));
    saveBtn->addStyleClass("btn btn-primary");
    saveBtn->clicked().connect(this, &QuoteBuilderView::saveQuote);

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

    auto clients = provider_.findAllClients();
    for (auto& c : clients)
        clientCombo_->addItem(c.name + " (" + c.company + ")");
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

    auto quotes = provider_.findAllQuotes();

    int row = 1;
    for (auto& q : quotes) {
        quoteListTable_->elementAt(row, 0)->addWidget(
            std::make_unique<Wt::WText>(q.title));
        quoteListTable_->elementAt(row, 1)->addWidget(
            std::make_unique<Wt::WText>(q.clientName.empty() ? "(none)" : q.clientName));
        quoteListTable_->elementAt(row, 2)->addWidget(
            std::make_unique<Wt::WText>(statusLabel(q.status)));
        quoteListTable_->elementAt(row, 3)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(q.lineItemCount)));

        std::ostringstream ts;
        ts << "$" << std::fixed << std::setprecision(2) << q.totalAmount;
        quoteListTable_->elementAt(row, 4)->addWidget(
            std::make_unique<Wt::WText>(ts.str()));

        std::string dateStr = q.createdDate.empty() ? "-" : q.createdDate.substr(0, 10);
        quoteListTable_->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(dateStr));

        auto actionsDiv = quoteListTable_->elementAt(row, 6)->addWidget(
            std::make_unique<Wt::WContainerWidget>());
        actionsDiv->addStyleClass("action-buttons");

        long long qid = q.id;

        auto openBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Open"));
        openBtn->addStyleClass("btn btn-sm btn-primary");
        openBtn->clicked().connect([this, qid] { openQuote(qid); });

        auto delBtn = actionsDiv->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        delBtn->addStyleClass("btn btn-sm btn-danger");
        delBtn->clicked().connect([this, qid] { deleteQuote(qid); });

        ++row;
    }
}

void QuoteBuilderView::createNewQuote()
{
    QuoteDTO dto;
    dto.title      = "New Quote";
    dto.status     = 0; // Draft
    dto.taxRate    = 8.25;
    dto.markupRate = 15.0;

    auto created = provider_.createQuote(dto);

    refreshQuoteList();
    openQuote(created.id);
}

void QuoteBuilderView::openQuote(long long quoteId)
{
    currentQuoteId_ = quoteId;
    listPanel_->hide();
    editorPanel_->show();
    editorQuoteId_->setText("Quote #" + std::to_string(quoteId));

    populateClientCombo();

    auto q = provider_.findQuoteById(quoteId);
    if (!q) return;

    titleEdit_->setText(q->title);
    descEdit_->setText(q->description);
    statusCombo_->setCurrentIndex(q->status);
    taxRateSpin_->setValue(q->taxRate);
    markupRateSpin_->setValue(q->markupRate);
    notesEdit_->setText(q->notes);

    // Select client in combo
    if (q->clientId > 0) {
        auto clients = provider_.findAllClients();
        int idx = 1;
        for (auto& c : clients) {
            std::string label = c.name + " (" + c.company + ")";
            if (c.id == q->clientId) {
                clientCombo_->setCurrentIndex(idx);
                break;
            }
            ++idx;
        }
    } else {
        clientCombo_->setCurrentIndex(0);
    }

    refreshLineItems();
    updateTotals();
}

void QuoteBuilderView::saveQuote()
{
    if (currentQuoteId_ < 0) return;

    QuoteDTO dto;
    dto.title       = titleEdit_->text().toUTF8();
    dto.description = descEdit_->text().toUTF8();
    dto.status      = statusCombo_->currentIndex();
    dto.taxRate     = taxRateSpin_->value();
    dto.markupRate  = markupRateSpin_->value();
    dto.notes       = notesEdit_->text().toUTF8();

    // Resolve client from combo index
    if (clientCombo_->currentIndex() > 0) {
        auto clients = provider_.findAllClients();
        int idx = 1;
        for (auto& c : clients) {
            if (idx == clientCombo_->currentIndex()) {
                dto.clientId = c.id;
                break;
            }
            ++idx;
        }
    }

    provider_.updateQuote(currentQuoteId_, dto);

    currentQuoteId_ = -1;
    editorPanel_->hide();
    listPanel_->show();
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
            provider_.deleteQuote(quoteId);

            if (currentQuoteId_ == quoteId) {
                currentQuoteId_ = -1;
                editorPanel_->hide();
                listPanel_->show();
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

    auto lineItems = provider_.findLineItemsByQuoteId(currentQuoteId_);

    int row = 1;
    for (auto& li : lineItems) {
        lineItemTable_->elementAt(row, 0)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(row)));
        lineItemTable_->elementAt(row, 1)->addWidget(
            std::make_unique<Wt::WText>(li.productName.empty() ? "(select product)" : li.productName));
        lineItemTable_->elementAt(row, 2)->addWidget(
            std::make_unique<Wt::WText>(std::to_string(li.quantity)));
        lineItemTable_->elementAt(row, 3)->addWidget(
            std::make_unique<Wt::WText>(li.supplierName.empty() ? "(not sourced)" : li.supplierName));

        std::ostringstream upStr, ltStr;
        upStr << "$" << std::fixed << std::setprecision(2) << li.unitPrice;
        ltStr << "$" << std::fixed << std::setprecision(2) << li.lineTotal;

        lineItemTable_->elementAt(row, 4)->addWidget(
            std::make_unique<Wt::WText>(upStr.str()));

        std::ostringstream mkStr;
        mkStr << std::fixed << std::setprecision(1) << li.markup << "%";
        lineItemTable_->elementAt(row, 5)->addWidget(
            std::make_unique<Wt::WText>(mkStr.str()));
        lineItemTable_->elementAt(row, 6)->addWidget(
            std::make_unique<Wt::WText>(ltStr.str()));

        auto actionsDiv = lineItemTable_->elementAt(row, 7)->addWidget(
            std::make_unique<Wt::WContainerWidget>());
        actionsDiv->addStyleClass("action-buttons");

        long long liId = li.id;

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
}

void QuoteBuilderView::addLineItem()
{
    if (currentQuoteId_ < 0) return;

    auto dialog = addChild(std::make_unique<Wt::WDialog>("Add Line Item"));
    dialog->setModal(true);
    dialog->setClosable(true);
    dialog->rejectWhenEscapePressed();
    dialog->setWidth(Wt::WLength(500));
    dialog->contents()->addStyleClass("dialog-content");

    dialog->contents()->addWidget(std::make_unique<Wt::WLabel>("Product:"));
    auto prodCombo = dialog->contents()->addWidget(std::make_unique<Wt::WComboBox>());
    prodCombo->addStyleClass("form-control");

    auto products = provider_.findAllProducts();
    std::vector<long long> productIds;
    for (auto& p : products) {
        prodCombo->addItem("[" + p.category + "] " + p.name + " (" + p.sku + ")");
        productIds.push_back(p.id);
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

        QuoteLineItemDTO dto;
        dto.quoteId   = currentQuoteId_;
        dto.productId = productIds[prodCombo->currentIndex()];
        dto.quantity  = qtySpin->value();
        dto.markup    = markupSpin->value();

        provider_.createLineItem(dto);

        dialog->accept();
        refreshLineItems();
        updateTotals();
    });

    cancelBtn->clicked().connect(dialog, &Wt::WDialog::reject);
    dialog->show();
}

void QuoteBuilderView::removeLineItem(long long lineItemId)
{
    provider_.deleteLineItem(lineItemId);
    refreshLineItems();
    updateTotals();
}

void QuoteBuilderView::runSourcing(long long lineItemId)
{
    auto li = provider_.findLineItemById(lineItemId);
    if (!li || li->productId <= 0) return;

    // Determine job site from client
    double jobLat = 30.267, jobLon = -97.743;
    auto quote = provider_.findQuoteById(li->quoteId);
    if (quote && quote->clientId > 0) {
        auto client = provider_.findClientById(quote->clientId);
        if (client && client->latitude != 0.0) {
            jobLat = client->latitude;
            jobLon = client->longitude;
        }
    }

    SourcingEngine engine(provider_);
    auto results = engine.findBestSources(li->productId, li->quantity, jobLat, jobLon);

    if (!results.empty()) {
        auto& best = results[0];

        // Update the line item with best source
        QuoteLineItemDTO update = *li;
        update.unitPrice  = best.effectivePrice;
        update.supplierId = best.supplierId;
        update.lineTotal  = li->quantity * best.effectivePrice * (1.0 + li->markup / 100.0);
        provider_.updateLineItem(lineItemId, update);

        // Show results dialog
        auto dialog = addChild(std::make_unique<Wt::WDialog>(
            "Sourcing Results: " + li->productName));
        dialog->setModal(true);
        dialog->setClosable(true);
        dialog->rejectWhenEscapePressed();
        dialog->setWidth(Wt::WLength(800));
        dialog->contents()->addStyleClass("dialog-content");

        std::ostringstream header;
        header << "<p><strong>Product:</strong> " << li->productName
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

        int srow = 1;
        for (auto& r : results) {
            table->elementAt(srow, 0)->addWidget(
                std::make_unique<Wt::WText>(std::to_string(srow)));
            table->elementAt(srow, 1)->addWidget(
                std::make_unique<Wt::WText>(r.supplierName));

            std::ostringstream ep, dist, sc;
            ep << "$" << std::fixed << std::setprecision(2) << r.effectivePrice;
            dist << std::fixed << std::setprecision(1) << r.distanceMiles << " mi";
            sc << std::fixed << std::setprecision(3) << r.compositeScore;

            table->elementAt(srow, 2)->addWidget(std::make_unique<Wt::WText>(ep.str()));
            table->elementAt(srow, 3)->addWidget(
                std::make_unique<Wt::WText>(
                    std::to_string(r.availableQty) + (r.meetsQty ? "" : " (short)")));
            table->elementAt(srow, 4)->addWidget(std::make_unique<Wt::WText>(dist.str()));

            std::ostringstream rat;
            rat << std::fixed << std::setprecision(1) << r.supplierRating;
            table->elementAt(srow, 5)->addWidget(std::make_unique<Wt::WText>(rat.str()));
            table->elementAt(srow, 6)->addWidget(std::make_unique<Wt::WText>(sc.str()));

            // Allow manual override
            long long rSupplierId = r.supplierId;
            double rPrice = r.effectivePrice;
            auto selectBtn = table->elementAt(srow, 7)->addWidget(
                std::make_unique<Wt::WPushButton>("Use This"));
            selectBtn->addStyleClass("btn btn-sm btn-success");
            selectBtn->clicked().connect([=] {
                QuoteLineItemDTO upd;
                auto current = provider_.findLineItemById(lineItemId);
                if (current) {
                    upd = *current;
                    upd.unitPrice  = rPrice;
                    upd.supplierId = rSupplierId;
                    upd.lineTotal  = upd.quantity * rPrice * (1.0 + upd.markup / 100.0);
                    provider_.updateLineItem(lineItemId, upd);
                }
                dialog->accept();
                refreshLineItems();
                updateTotals();
            });

            if (srow == 1) {
                for (int col = 0; col < 8; ++col)
                    table->elementAt(srow, col)->addStyleClass("best-source");
            }
            ++srow;
        }

        auto closeBtn = dialog->footer()->addWidget(
            std::make_unique<Wt::WPushButton>("Keep Best & Close"));
        closeBtn->addStyleClass("btn btn-primary");
        closeBtn->clicked().connect(dialog, &Wt::WDialog::accept);

        dialog->show();
    }

    refreshLineItems();
    updateTotals();
}

void QuoteBuilderView::runSourcingForAll()
{
    if (currentQuoteId_ < 0) return;

    // Determine job site location
    double jobLat = 30.267, jobLon = -97.743;
    auto quote = provider_.findQuoteById(currentQuoteId_);
    if (quote && quote->clientId > 0) {
        auto client = provider_.findClientById(quote->clientId);
        if (client && client->latitude != 0.0) {
            jobLat = client->latitude;
            jobLon = client->longitude;
        }
    }

    SourcingEngine engine(provider_);
    int sourced = 0;

    auto lineItems = provider_.findLineItemsByQuoteId(currentQuoteId_);

    for (auto& li : lineItems) {
        if (li.productId <= 0) continue;

        auto results = engine.findBestSources(li.productId, li.quantity, jobLat, jobLon);

        if (!results.empty()) {
            auto& best = results[0];
            QuoteLineItemDTO update = li;
            update.unitPrice  = best.effectivePrice;
            update.supplierId = best.supplierId;
            update.lineTotal  = li.quantity * best.effectivePrice * (1.0 + li.markup / 100.0);
            provider_.updateLineItem(li.id, update);
            ++sourced;
        }
    }

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

    auto lineItems = provider_.findLineItemsByQuoteId(currentQuoteId_);

    double subtotal = 0;
    for (auto& li : lineItems)
        subtotal += li.lineTotal;

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
}

#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WComboBox.h>
#include <Wt/WLineEdit.h>
#include <Wt/WDoubleSpinBox.h>
#include <Wt/WSpinBox.h>
#include <Wt/WTextArea.h>
#include <Wt/WText.h>
#include <Wt/WPushButton.h>
class DataProvider;

/// Quote builder view – create/edit quotes, add line items, run sourcing
/// engine to find best suppliers, and compute totals.
class QuoteBuilderView : public Wt::WContainerWidget {
public:
    QuoteBuilderView(DataProvider& provider);

private:
    DataProvider& provider_;

    // Quote list
    Wt::WTable* quoteListTable_ = nullptr;

    // Quote editor widgets
    Wt::WContainerWidget* editorPanel_     = nullptr;
    Wt::WLineEdit*        titleEdit_       = nullptr;
    Wt::WTextArea*        descEdit_        = nullptr;
    Wt::WComboBox*        clientCombo_     = nullptr;
    Wt::WComboBox*        statusCombo_     = nullptr;
    Wt::WDoubleSpinBox*   taxRateSpin_     = nullptr;
    Wt::WDoubleSpinBox*   markupRateSpin_  = nullptr;
    Wt::WTextArea*        notesEdit_       = nullptr;
    Wt::WTable*           lineItemTable_   = nullptr;
    Wt::WText*            subtotalText_    = nullptr;
    Wt::WText*            taxText_         = nullptr;
    Wt::WText*            totalText_       = nullptr;

    long long currentQuoteId_ = -1;

    void buildUI();
    void refreshQuoteList();
    void createNewQuote();
    void openQuote(long long quoteId);
    void saveQuote();
    void deleteQuote(long long quoteId);
    void refreshLineItems();
    void addLineItem();
    void removeLineItem(long long lineItemId);
    void runSourcing(long long lineItemId);
    void runSourcingForAll();
    void updateTotals();

    void populateClientCombo();
};

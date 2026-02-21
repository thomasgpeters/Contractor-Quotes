#include "views/ContractorApp.h"
#include "views/DashboardView.h"
#include "views/ClientView.h"
#include "views/ProductCatalogView.h"
#include "views/SupplierView.h"
#include "views/QuoteBuilderView.h"
#include "data/DataProvider.h"
#include <Wt/WBootstrap5Theme.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WMenu.h>
#include <Wt/WText.h>
#include <Wt/WLink.h>
#include <Wt/WCssStyleSheet.h>
#include <Wt/WImage.h>
#include <Wt/WPushButton.h>
#include <Wt/WDialog.h>

ContractorApp::ContractorApp(const Wt::WEnvironment& env, DataProvider& provider)
    : Wt::WApplication(env),
      provider_(provider)
{
    setTitle("Contractor Quotes & Sourcing");

    auto theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);

    useStyleSheet("css/app.css");

    auto root = this->root();
    root->addStyleClass("app-root");

    setupNavigation(root);
}

void ContractorApp::setupNavigation(Wt::WContainerWidget* root)
{
    auto navBar = root->addWidget(std::make_unique<Wt::WNavigationBar>());
    navBar->setTitle("Contractor Quotes", Wt::WLink(Wt::LinkType::InternalPath, "/"));
    navBar->setResponsive(true);
    navBar->addStyleClass("main-navbar");

    auto aboutBtn = std::make_unique<Wt::WPushButton>();
    aboutBtn->setText(Wt::WString::fromUTF8("\xe2\x93\x98"));
    aboutBtn->addStyleClass("btn btn-link about-btn");
    aboutBtn->setToolTip("About");
    aboutBtn->clicked().connect([this] { showAboutDialog(); });
    navBar->addWidget(std::move(aboutBtn), Wt::AlignmentFlag::Right);

    stack_ = root->addWidget(std::make_unique<Wt::WStackedWidget>());
    stack_->addStyleClass("page-content");

    menu_ = navBar->addMenu(std::make_unique<Wt::WMenu>(stack_));
    menu_->addStyleClass("main-menu");

    auto dashItem = menu_->addItem("Dashboard",
        std::make_unique<DashboardView>(provider_));
    dashItem->setPathComponent("dashboard");

    auto clientItem = menu_->addItem("Clients",
        std::make_unique<ClientView>(provider_));
    clientItem->setPathComponent("clients");

    auto productItem = menu_->addItem("Products",
        std::make_unique<ProductCatalogView>(provider_));
    productItem->setPathComponent("products");

    auto supplierItem = menu_->addItem("Suppliers",
        std::make_unique<SupplierView>(provider_));
    supplierItem->setPathComponent("suppliers");

    auto quoteItem = menu_->addItem("Quote Builder",
        std::make_unique<QuoteBuilderView>(provider_));
    quoteItem->setPathComponent("quotes");

    if (internalPath().empty() || internalPath() == "/") {
        setInternalPath("/dashboard", true);
    }
}

void ContractorApp::showAboutDialog()
{
    auto dialog = addChild(std::make_unique<Wt::WDialog>("About Contractor Quotes"));
    dialog->setModal(true);
    dialog->setClosable(true);
    dialog->rejectWhenEscapePressed();
    dialog->setWidth(Wt::WLength(450));
    dialog->contents()->addStyleClass("about-dialog");

    dialog->contents()->addWidget(std::make_unique<Wt::WText>(
        "<div class='about-logo'>"
        "<img src='images/imagery_logo.png' alt='Imagery Business Systems' />"
        "</div>"
        "<h3>Contractor Quotes &amp; Sourcing</h3>"
        "<p>A comprehensive quoting and material sourcing platform for contractors. "
        "Build accurate quotes with automatic best-source supplier selection, "
        "real-time pricing, and intelligent inventory-aware recommendations.</p>"
        "<p class='about-copy'>&copy; 2026 Imagery Business Systems. All rights reserved.</p>"
        "<p><a href='https://imagery-business-systems.com' target='_blank'>"
        "imagery-business-systems.com</a></p>"
    ));

    auto closeBtn = dialog->footer()->addWidget(
        std::make_unique<Wt::WPushButton>("Close"));
    closeBtn->addStyleClass("btn btn-primary");
    closeBtn->clicked().connect(dialog, &Wt::WDialog::accept);

    dialog->finished().connect([this, dialog] {
        removeChild(dialog);
    });

    dialog->show();
}

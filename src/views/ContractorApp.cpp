#include "views/ContractorApp.h"
#include "views/DashboardView.h"
#include "views/ClientView.h"
#include "views/ProductCatalogView.h"
#include "views/SupplierView.h"
#include "views/QuoteBuilderView.h"
#include "models/Session.h"
#include <Wt/WBootstrap5Theme.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WMenu.h>
#include <Wt/WText.h>
#include <Wt/WLink.h>
#include <Wt/WCssStyleSheet.h>

ContractorApp::ContractorApp(const Wt::WEnvironment& env, Session& session)
    : Wt::WApplication(env),
      session_(session)
{
    setTitle("Contractor Quotes & Sourcing");

    // Apply Bootstrap 5 theme
    auto theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);

    // Load custom CSS
    useStyleSheet("css/app.css");

    auto root = this->root();
    root->addStyleClass("app-root");

    setupNavigation(root);
}

void ContractorApp::setupNavigation(Wt::WContainerWidget* root)
{
    // Navigation bar
    auto navBar = root->addWidget(std::make_unique<Wt::WNavigationBar>());
    navBar->setTitle("ContractorQuotes", Wt::WLink(Wt::LinkType::InternalPath, "/"));
    navBar->setResponsive(true);
    navBar->addStyleClass("main-navbar");

    // Stacked widget for page content
    stack_ = root->addWidget(std::make_unique<Wt::WStackedWidget>());
    stack_->addStyleClass("page-content");

    // Navigation menu
    menu_ = navBar->addMenu(std::make_unique<Wt::WMenu>(stack_));
    menu_->addStyleClass("main-menu");

    // Add pages
    auto dashItem = menu_->addItem("Dashboard",
        std::make_unique<DashboardView>(session_));
    dashItem->setPathComponent("dashboard");

    auto clientItem = menu_->addItem("Clients",
        std::make_unique<ClientView>(session_));
    clientItem->setPathComponent("clients");

    auto productItem = menu_->addItem("Products",
        std::make_unique<ProductCatalogView>(session_));
    productItem->setPathComponent("products");

    auto supplierItem = menu_->addItem("Suppliers",
        std::make_unique<SupplierView>(session_));
    supplierItem->setPathComponent("suppliers");

    auto quoteItem = menu_->addItem("Quote Builder",
        std::make_unique<QuoteBuilderView>(session_));
    quoteItem->setPathComponent("quotes");

    // Default page
    if (internalPath().empty() || internalPath() == "/") {
        setInternalPath("/dashboard", true);
    }
}

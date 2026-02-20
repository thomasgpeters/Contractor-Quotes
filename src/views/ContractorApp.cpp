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
    navBar->setTitle("ContractorQuotes", Wt::WLink(Wt::LinkType::InternalPath, "/"));
    navBar->setResponsive(true);
    navBar->addStyleClass("main-navbar");

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

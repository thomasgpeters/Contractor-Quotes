#pragma once

#include <Wt/WApplication.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WMenu.h>
#include <Wt/WContainerWidget.h>
#include <memory>

class DataProvider;

/// Top-level Wt application – sets up navigation and manages page views.
class ContractorApp : public Wt::WApplication {
public:
    ContractorApp(const Wt::WEnvironment& env, DataProvider& provider);

private:
    DataProvider& provider_;

    Wt::WMenu*           menu_;
    Wt::WStackedWidget*  stack_;

    void setupNavigation(Wt::WContainerWidget* root);
};

#pragma once

#include <Wt/WContainerWidget.h>

class DataProvider;

/// Dashboard showing summary statistics and recent activity.
class DashboardView : public Wt::WContainerWidget {
public:
    DashboardView(DataProvider& provider);

private:
    DataProvider& provider_;

    void buildUI();
};

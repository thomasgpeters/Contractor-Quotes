#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/Dbo/Session.h>

class Session;

/// Dashboard showing summary statistics and recent activity.
class DashboardView : public Wt::WContainerWidget {
public:
    DashboardView(Session& session);

private:
    Session& session_;

    void buildUI();
};

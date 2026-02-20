#include "views/ContractorApp.h"
#include "models/Session.h"
#include <Wt/WServer.h>
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    try {
        Wt::WServer server(argc, argv);

        // Initialize database session (shared across app instances)
        Session session("contractor_quotes.db");
        session.seedIfEmpty();

        server.addEntryPoint(
            Wt::EntryPointType::Application,
            [&session](const Wt::WEnvironment& env) {
                return std::make_unique<ContractorApp>(env, session);
            }
        );

        // Start serving
        if (server.start()) {
            int sig = Wt::WServer::waitForShutdown();
            std::cout << "Shutdown (signal = " << sig << ")" << std::endl;
            server.stop();
        }
    } catch (Wt::WServer::Exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

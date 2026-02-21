#include "views/ContractorApp.h"
#include "config/AppConfig.h"
#include "data/DataProvider.h"
#include "data/LocalDataProvider.h"
#include "data/ApiDataProvider.h"
#include "models/Session.h"
#include <Wt/WServer.h>
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    try {
        Wt::WServer server(argc, argv);

        // Load configuration from model/app_config.yaml
        AppConfig config = AppConfig::loadFromFile("model/app_config.yaml");

        // Create the appropriate DataProvider based on architecture setting
        std::unique_ptr<Session> session;
        std::unique_ptr<DataProvider> provider;

        if (config.architecture == AppConfig::Architecture::Local) {
            std::cout << "Architecture: local (" << config.sqlitePath << ")" << std::endl;
            session = std::make_unique<Session>(config.sqlitePath);
            session->seedIfEmpty();
            provider = std::make_unique<LocalDataProvider>(*session);
        } else {
            std::cout << "Architecture: api (" << config.apiBaseUrl << ")" << std::endl;
            provider = std::make_unique<ApiDataProvider>(config);
        }

        DataProvider& providerRef = *provider;

        server.addEntryPoint(
            Wt::EntryPointType::Application,
            [&providerRef](const Wt::WEnvironment& env) {
                return std::make_unique<ContractorApp>(env, providerRef);
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

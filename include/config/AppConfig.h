#pragma once

#include <string>

/// Application configuration loaded from model/app_config.yaml.
struct AppConfig {
    enum class Architecture { Local, Api };
    enum class LocalDatabase { Sqlite, Postgres };

    Architecture  architecture = Architecture::Api;

    // Local database settings
    LocalDatabase localDatabase = LocalDatabase::Sqlite;
    std::string   sqlitePath    = "contractor_quotes.db";
    std::string   pgHost        = "localhost";
    int           pgPort        = 5432;
    std::string   pgDatabase    = "contractor_quotes";
    std::string   pgUser        = "postgres";
    std::string   pgPassword;

    // API settings
    std::string   apiBaseUrl       = "http://localhost:5667/api";
    int           requestTimeout   = 30;
    std::string   apiKeyHeader;
    std::string   apiKeyValue;

    /// Load configuration from a YAML file.
    /// Returns default config if the file cannot be read.
    static AppConfig loadFromFile(const std::string& path);
};

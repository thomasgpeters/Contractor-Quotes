#include "config/AppConfig.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// Minimal YAML parser for our flat config format (2 levels max).
// Handles:  key: value  and  key:\n  subkey: value

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
         (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

static int indentLevel(const std::string& line) {
    int n = 0;
    for (char c : line) {
        if (c == ' ') ++n;
        else break;
    }
    return n;
}

AppConfig AppConfig::loadFromFile(const std::string& path) {
    AppConfig cfg;

    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[AppConfig] Cannot open " << path
                  << " – using defaults\n";
        return cfg;
    }

    std::string section;  // current top-level section (e.g. "local", "api_logic_server")
    std::string subsection; // e.g. "postgres" under "local"
    std::string line;

    while (std::getline(f, line)) {
        // Skip comments and blank lines
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        int indent = indentLevel(line);
        auto colonPos = trimmed.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key   = trim(trimmed.substr(0, colonPos));
        std::string value = trim(trimmed.substr(colonPos + 1));

        // Strip inline comments
        auto commentPos = value.find('#');
        if (commentPos != std::string::npos) {
            value = trim(value.substr(0, commentPos));
        }
        value = stripQuotes(value);

        if (indent == 0) {
            // Top-level key
            subsection.clear();
            if (value.empty()) {
                section = key;
            } else {
                section.clear();
                if (key == "architecture") {
                    if (value == "local")
                        cfg.architecture = Architecture::Local;
                    else
                        cfg.architecture = Architecture::Api;
                }
            }
        } else if (indent <= 4 && !section.empty()) {
            if (value.empty()) {
                subsection = key;
            } else if (section == "local") {
                if (subsection == "postgres") {
                    if      (key == "host")     cfg.pgHost     = value;
                    else if (key == "port")     cfg.pgPort     = std::stoi(value);
                    else if (key == "database") cfg.pgDatabase = value;
                    else if (key == "user")     cfg.pgUser     = value;
                    else if (key == "password") cfg.pgPassword = value;
                } else {
                    if (key == "database") {
                        if (value == "postgres")
                            cfg.localDatabase = LocalDatabase::Postgres;
                        else
                            cfg.localDatabase = LocalDatabase::Sqlite;
                    }
                    else if (key == "sqlite_path")
                        cfg.sqlitePath = value;
                }
            } else if (section == "api_logic_server") {
                if      (key == "api_base_url")     cfg.apiBaseUrl     = value;
                else if (key == "request_timeout")  cfg.requestTimeout = std::stoi(value);
                else if (key == "api_key_header")   cfg.apiKeyHeader   = value;
                else if (key == "api_key_value")    cfg.apiKeyValue    = value;
            }
        }
    }

    std::cout << "[AppConfig] architecture="
              << (cfg.architecture == Architecture::Api ? "api" : "local")
              << "\n";
    if (cfg.architecture == Architecture::Api)
        std::cout << "[AppConfig] api_base_url=" << cfg.apiBaseUrl << "\n";
    else
        std::cout << "[AppConfig] local_database="
                  << (cfg.localDatabase == LocalDatabase::Sqlite ? "sqlite" : "postgres")
                  << "\n";

    return cfg;
}

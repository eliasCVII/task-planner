#include "Config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

Config::Config(const std::string& configPath) : configFilePath(configPath) {
    setDefaults();
}

void Config::setDefaults() {
    // Core settings
    settings["data-dir"] = "data";
    settings["default-day-length"] = "7.0";  // hours
    settings["date-format"] = "YYYY-MM-DD";

    // UI/Behavior settings
    settings["auto-save"] = "true";
    settings["show-warnings"] = "true";
    settings["default-start-time"] = "09:00";
    settings["time-format"] = "24h";  // 24h or 12h

    // File settings
    settings["file-extension"] = ".json";
    settings["backup-enabled"] = "false";
    settings["max-backup-files"] = "5";

    // Display settings
    settings["table-width"] = "full";  // full or auto
    settings["status-messages"] = "true";

    // Session state (not saved to config file, managed separately)
    settings["last-opened-file"] = "";
}

bool Config::loadFromFile() {
    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        std::cout << "Config file not found: " << configFilePath << ". Using defaults." << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Find the colon separator
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "Warning: Invalid config line " << lineNumber << ": " << line << std::endl;
            continue;
        }

        // Extract key and value
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (!key.empty()) {
            settings[key] = value;
        }
    }

    file.close();
    return true;
}

bool Config::saveToFile() const {
    std::ofstream file(configFilePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file for writing: " << configFilePath << std::endl;
        return false;
    }

    file << "# Task Manager Configuration File\n";
    file << "# Format: key: value\n";
    file << "# Lines starting with # or ; are comments\n\n";

    file << "# Core Settings\n";
    file << "data-dir: " << settings.at("data-dir") << "\n";
    file << "default-day-length: " << settings.at("default-day-length") << "\n";
    file << "date-format: " << settings.at("date-format") << "\n\n";

    file << "# UI and Behavior\n";
    file << "auto-save: " << settings.at("auto-save") << "\n";
    file << "show-warnings: " << settings.at("show-warnings") << "\n";
    file << "default-start-time: " << settings.at("default-start-time") << "\n";
    file << "time-format: " << settings.at("time-format") << "\n\n";

    file << "# File Settings\n";
    file << "file-extension: " << settings.at("file-extension") << "\n";
    file << "backup-enabled: " << settings.at("backup-enabled") << "\n";
    file << "max-backup-files: " << settings.at("max-backup-files") << "\n\n";

    file << "# Display Settings\n";
    file << "table-width: " << settings.at("table-width") << "\n";
    file << "status-messages: " << settings.at("status-messages") << "\n";

    file.close();
    return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = settings.find(key);
    return (it != settings.end()) ? it->second : defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    auto it = settings.find(key);
    if (it != settings.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            std::cerr << "Warning: Invalid integer value for " << key << ": " << it->second << std::endl;
        }
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    auto it = settings.find(key);
    if (it != settings.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "yes" || value == "1" || value == "on");
    }
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) const {
    auto it = settings.find(key);
    if (it != settings.end()) {
        try {
            return std::stod(it->second);
        } catch (const std::exception&) {
            std::cerr << "Warning: Invalid double value for " << key << ": " << it->second << std::endl;
        }
    }
    return defaultValue;
}

void Config::set(const std::string& key, const std::string& value) {
    settings[key] = value;
}

void Config::set(const std::string& key, int value) {
    settings[key] = std::to_string(value);
}

void Config::set(const std::string& key, bool value) {
    settings[key] = value ? "true" : "false";
}

void Config::set(const std::string& key, double value) {
    settings[key] = std::to_string(value);
}

bool Config::has(const std::string& key) const {
    return settings.find(key) != settings.end();
}

const std::map<std::string, std::string>& Config::getAllSettings() const {
    return settings;
}

bool Config::createDefaultConfig() const {
    if (std::filesystem::exists(configFilePath)) {
        std::cout << "Config file already exists: " << configFilePath << std::endl;
        return false;
    }

    return saveToFile();
}

// Session state management
std::string Config::getLastOpenedFile() const {
    // Try to load from session state file
    std::string sessionFile = ".task_session";
    std::ifstream file(sessionFile);
    if (file.is_open()) {
        std::string lastFile;
        if (std::getline(file, lastFile)) {
            file.close();
            // Trim whitespace
            lastFile.erase(0, lastFile.find_first_not_of(" \t\n\r"));
            lastFile.erase(lastFile.find_last_not_of(" \t\n\r") + 1);
            return lastFile;
        }
        file.close();
    }
    return "";
}

void Config::setLastOpenedFile(const std::string& filename) {
    settings["last-opened-file"] = filename;
}

bool Config::saveSessionState() const {
    try {
        std::string sessionFile = ".task_session";
        std::ofstream file(sessionFile);
        if (!file.is_open()) {
            return false;
        }

        std::string lastFile = getString("last-opened-file", "");
        if (!lastFile.empty()) {
            file << lastFile << std::endl;
        }

        file.close();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

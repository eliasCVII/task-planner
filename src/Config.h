#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>

class Config {
private:
    std::map<std::string, std::string> settings;
    std::string configFilePath;

    // Default values
    void setDefaults();

public:
    Config(const std::string& configPath = "plan.conf");

    // Load configuration from file
    bool loadFromFile();

    // Save configuration to file
    bool saveToFile() const;

    // Get setting value with default fallback
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;

    // Set setting value
    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, int value);
    void set(const std::string& key, bool value);
    void set(const std::string& key, double value);

    // Check if setting exists
    bool has(const std::string& key) const;

    // Get all settings (for debugging/display)
    const std::map<std::string, std::string>& getAllSettings() const;

    // Create default config file
    bool createDefaultConfig() const;

    // Session state management
    std::string getLastOpenedFile() const;
    void setLastOpenedFile(const std::string& filename);
    bool saveSessionState() const;
};

#endif // CONFIG_H

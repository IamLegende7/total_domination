#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <unordered_map>
#include <string>
#include <filesystem>

class Registry {
    private:
        std::unordered_map<std::string, std::unordered_map<std::string, std::filesystem::path>> values;
    public:
        Registry();
        ~Registry();

        bool add(const std::string& category, const std::string& key, const std::filesystem::path& value);
        bool load(const std::string& category, const std::filesystem::path& registry_file);

        std::filesystem::path get(const std::string& category, const std::string& key, const std::filesystem::path& default_value, bool suppress_logs=false);
};

inline Registry* REGISTRY = nullptr;

#endif
#include <string>
#include <filesystem>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "registry.hpp"

#include "utils/json.hpp"
#include "utils/logger.hpp"

#include "settings/locations.hpp"

Registry::Registry() {}

Registry::~Registry() {}



bool Registry::add(const std::string& category, const std::string& key, const std::filesystem::path& value) {
    LOG(LogLevel::Debug, "Registry: \"%s\":\"%s\" = \"%s\"", category.c_str(), key.c_str(), value.u8string().c_str());
    values[category][key] = value;
    return true;
}

bool Registry::load(const std::string& category, const std::filesystem::path& registry_file) {
    rapidjson::Document data = open_json(replace_locations(registry_file));

    if (!data.IsObject()) {
        LOG(LogLevel::Warning, "\"%s\" is not a valid registry.json file: root is not an object!", registry_file.u8string().c_str());
        return false;
    }

    for (auto declaration = data.MemberBegin(); declaration != data.MemberEnd(); ++declaration) {
        if (declaration->value.IsString()) {
            add(category, declaration->name.GetString(), replace_locations(std::filesystem::path(declaration->value.GetString())));
        } else {
            LOG(LogLevel::Warning, "Key \"%s\" in file \"%s\" contains a non-string value.", declaration->name.GetString(), registry_file.u8string().c_str());
        }
    }
    return true;
}



std::filesystem::path Registry::get(const std::string& category, const std::string& key, const std::filesystem::path& default_value, bool suppress_logs) {
    auto category_it = values.find(category);
    if (category_it != values.end()) {
        auto value_it = category_it->second.find(key);
        if (value_it != category_it->second.end()) {
            return value_it->second;
        }
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent registered value \"%s\" in \"%s\"", key.c_str(), category.c_str());
    return default_value;
}

std::unordered_map<std::string, std::filesystem::path>& Registry::get_category(const std::string& category, bool suppress_logs) {
    for (auto& [key, map] : values) {
        if (key == category) {
            return map;
        }
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent registery category \"%s\"", category.c_str());
    static std::unordered_map<std::string, std::filesystem::path> empty_map;
    empty_map.clear();
    return empty_map;
}
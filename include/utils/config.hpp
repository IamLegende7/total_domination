#ifndef CONFIG_HPP
#define CONFIG_HPP


// general
#include <string>
#include <variant>
#include <filesystem>

#include "utils/logger.hpp"

// loading / saving settings
#include <SimpleIni.h>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>

// for storing settings
#include <map>

// Setting class //
class Setting {
    public:
        using SettingType = std::variant<bool, int, std::string, std::filesystem::path>;

        Setting() {}

        template<typename T>
        Setting(T value) : value(value) {}

        template<typename T>
        void set(T new_value) {
            value = new_value;
        };

        // Some functions for convienient calling //
        template<typename T>
        T get() {
            try {
                return std::get<T>(value);
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::Error, "Type mismatch: Expected \"%s\", returning default.", typeid(T).name());
                return T{};
            }
        }

        std::string str() {
            try {
                return std::get<std::string>(value);
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::Error, "Type mismatch: Expected \"std::string\", returning default empty string.");
                return "";
            }
        }

        const char* c_str() {
            try {
                return std::get<std::string>(value).c_str();
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::Error, "Type mismatch: Expected \"const char*\", returning default empty string.");
                return "";
            }
        }

    private:
        SettingType value;
};

template<typename T>
inline T load_setting(std::filesystem::path file, const std::string& section, const std::string& name, const T default_value = T{}) {
    CSimpleIniA ini;
	ini.SetUnicode();

    const std::string file_str = file.u8string();
    SI_Error rc = ini.LoadFile(file_str.c_str());
    if (rc < 0) {
        LOG(LogLevel::Error, "Could not load ini file \"%s\".", file_str.c_str());
        return T();
    }

    std::stringstream default_stream;
    default_stream << default_value;
    std::string default_value_string = default_stream.str();

    const char* loaded_value = ini.GetValue(section.c_str(), name.c_str(), default_value_string.c_str());
    if (!loaded_value) {
        LOG(LogLevel::Warning, "Key %s not found in section %s", name.c_str(), section.c_str());
        return T();
    }

    T result;
    if constexpr(std::is_same<T, std::string>::value) {
        result = std::string(loaded_value);
    } else if constexpr(std::is_same<T, int>::value) {
        result = std::stoi(std::string(loaded_value));
    } else if constexpr(std::is_same<T, bool>::value) {
        result = (std::strcmp(loaded_value, "true") == 0 || std::strcmp(loaded_value, "1") == 0);
    } else if constexpr(std::is_same<T, std::filesystem::path>::value) {
        result = std::filesystem::path(loaded_value);
    } else {
        LOG(LogLevel::Error, "Could not convert value to type \"%s\" for key %s", typeid(T).name(), name.c_str());
        result = T();
    }

    return result;
}

#endif
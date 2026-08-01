#ifndef CONFIG_HPP
#define CONFIG_HPP


// general
#include <string>
#include <variant>

#include "utils/logger.hpp"

// loading / saving settings
#include <SimpleIni.h>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>

// for storing settings
#include <unordered_map>

// TODO: rewrite

// Setting class //
class Setting {
    public:
        using SettingType = std::variant<bool, int, std::string>;

        // based off https://devblogs.microsoft.com/oldnewthing/20191106-00/?p=103066
        operator bool() const {
            try {
                return std::get<bool>(value);
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::ERROR, "Type mismatch: Expected bool, returning default false.");
                return false;
            }
        }

        operator int() const {
            try {
                return std::get<int>(value);
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::ERROR, "Type mismatch: Expected int, returning default 0.");
                return 0;
            } 
        }

        operator std::string() const {
            try {
                return std::get<std::string>(value);
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::ERROR, "Type mismatch: Expected std::string, returning default empty string.");
                return "";
            }
        }

        Setting() {}

        template<typename T>
        Setting(T value) : value(value) {}

        template<typename T>
        void set(T new_value) {
            value = new_value;
        };

        // Some functions for convienient calling //
        std::string get_str() {
            try {
                return std::get<std::string>(value);
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::ERROR, "Type mismatch: Expected std::string, returning default empty string.");
                return "";
            }
        }

        const char* get_c_str() {
            try {
                return std::get<std::string>(value).c_str();
            } catch (const std::bad_variant_access&) {
                LOG(LogLevel::ERROR, "Type mismatch: Expected const char*, returning default empty string.");
                return "";
            }
        }

    private:
        SettingType value;
};

template<typename T>
inline T load_setting(std::string file, std::string section, std::string name, const T default_value = T()) {
    CSimpleIniA ini;
	ini.SetUnicode();

    SI_Error rc = ini.LoadFile(file.c_str());
    if (rc < 0) {
        LOG(LogLevel::ERROR, "Could not load ini File %s", file.c_str());
        return T();
    }

    std::stringstream default_stream;
    default_stream << default_value;
    std::string default_value_string = default_stream.str();

    const char* loaded_value = ini.GetValue(section.c_str(), name.c_str(), default_value_string.c_str());
    if (!loaded_value) {
        LOG(LogLevel::WARNING, "Key %s not found in section %s", name.c_str(), section.c_str());
        return T();
    }

    T result;
    if constexpr(std::is_same<T, std::string>::value) {
        result = std::string(loaded_value);
    } else if constexpr(std::is_same<T, int>::value) {
        result = std::stoi(std::string(loaded_value));
    } else if constexpr(std::is_same<T, bool>::value){
        result = (std::strcmp(loaded_value, "true") == 0 || std::strcmp(loaded_value, "1") == 0);
    } else {
        LOG(LogLevel::ERROR, "Could not convert value to type T for key %s", name.c_str());
        result = T();
    }

    return result;
}

#endif
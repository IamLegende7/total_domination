#ifndef SETTINGS_LOCATIONS_H
#define SETTINGS_LOCATIONS_H

#include "utils/config.hpp"

#include <string>
#include <SDL3/SDL.h>
#include <filesystem>

/* This is where most file locations are stored for later use in the main scripts

*/

inline std::map<std::string, Setting> LOCATIONS;

inline std::filesystem::path replace_locations(const std::filesystem::path& input_path) {
    std::filesystem::path output_path = "";

    for (const auto& piece : input_path) {
        std::string piece_str = piece.string();
        if (piece_str == "")
            continue;
        if ((piece_str.front() == '$') && (piece_str.back() == '$')) {
            for (auto& [key, setting] : LOCATIONS) {
                if ("$"+key+"$" == piece_str) {
                    output_path /= setting.get<std::filesystem::path>();
                    break;
                }
            }
        } else {
            output_path /= piece_str;
        }
    }

    return output_path;
}

inline void init_locations_settings(const std::filesystem::path& config_file) {
    LOCATIONS["base"] =                 Setting(std::filesystem::path(SDL_GetBasePath()));
    LOCATIONS["cwd"] =                  Setting(std::filesystem::path(SDL_GetCurrentDirectory()));
    // Main dirs //
    LOCATIONS["data_dir"] =             Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Main dirs", "data_dir")));
    LOCATIONS["config_dir"] =           Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Main dirs", "config_dir")));
    LOCATIONS["resource_dir"] =         Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Main dirs", "resource_dir")));
    // Maps //
    LOCATIONS["map_dir"] =              Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Maps", "map_dir")));
    // Textures //
    LOCATIONS["texture_dir"] =          Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Textures", "texture_dir")));
    LOCATIONS["texturepack_dir"] =      Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Textures", "texturepack_dir")));
    LOCATIONS["missing_texture"] =      Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Textures", "missing_texture")));
    LOCATIONS["missing_texture"] =      Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Textures", "missing_texture_tile")));
    LOCATIONS["textures_json"] =        Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Textures", "textures_json")));
    // Logging //
    LOCATIONS["log_dir"] =              Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Logging", "log_dir")));
    LOCATIONS["log_file"] =             Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Logging", "log_file")));
    LOCATIONS["log_crash_dir"] =        Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Logging", "log_crash_dir")));
    // Registry //
    LOCATIONS["units_json"] =           Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Registry", "units_json")));
    LOCATIONS["tiles_json"] =           Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Registry", "tiles_json")));
    // Mods //
    LOCATIONS["mod_dir"] =              Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Mods", "mod_dir")));
    LOCATIONS["mod_server_path"] =      Setting(replace_locations(load_setting<std::filesystem::path>(config_file.u8string(), "Mods", "mod_server_path")));
}

#endif
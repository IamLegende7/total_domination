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
    LOCATIONS["data_dir"] =             Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Main dirs", "data_dir")));
    LOCATIONS["config_dir"] =           Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Main dirs", "config_dir")));
    LOCATIONS["resource_dir"] =         Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Main dirs", "resource_dir")));
    // Maps //
    LOCATIONS["map_dir"] =              Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Maps", "map_dir")));
    // Textures //
    LOCATIONS["texture_dir"] =          Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Textures", "texture_dir")));
    LOCATIONS["texturepack_dir"] =      Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Textures", "texturepack_dir")));
    LOCATIONS["missing_texture"] =      Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Textures", "missing_texture")));
    LOCATIONS["missing_texture"] =      Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Textures", "missing_texture_tile")));
    // Logging //
    LOCATIONS["log_dir"] =              Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Logging", "log_dir")));
    LOCATIONS["log_file"] =             Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Logging", "log_file")));
    LOCATIONS["log_crash_dir"] =        Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Logging", "log_crash_dir")));
    // Mods //
    LOCATIONS["shipped_python"] =       Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Mods", "shipped_python")));
    LOCATIONS["mod_dir"] =              Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Mods", "mod_dir")));
    // Actors //
    LOCATIONS["actor_dir"] =            Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Actors", "actor_dir")));
}

#endif
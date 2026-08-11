#ifndef SETTINGS_DEBUG_H
#define SETTINGS_DEBUG_H

#include "utils/config.hpp"
#include <filesystem>

/* Detailed debug mode settings

*/

inline std::map<std::string, Setting> DEBUG;

inline void init_debug_settings(const std::filesystem::path& config_file) {
    // Renderring //
    DEBUG["show_tile_hiding"] =         Setting(load_setting<bool>(config_file.u8string(), "Renderring", "show_tile_hiding", false));
    DEBUG["save_texture_atlases"] =     Setting(load_setting<bool>(config_file.u8string(), "Renderring", "save_texture_atlases", false));
    DEBUG["show_quadtree"] =            Setting(load_setting<bool>(config_file.u8string(), "Renderring", "show_quadtree", false));
    // Movement //
    DEBUG["show_coords"] =              Setting(load_setting<bool>(config_file.u8string(), "Movement", "show_coords", false));
    // HUD //
    DEBUG["show_fps"] =                 Setting(load_setting<bool>(config_file.u8string(), "HUD", "show_fps", false));
    DEBUG["show_tps"] =                 Setting(load_setting<bool>(config_file.u8string(), "HUD", "show_tps", false));
    // Logging // 
    DEBUG["all_debug_logs"] =           Setting(load_setting<bool>(config_file.u8string(), "Logging", "all_debug_logs", false));
    // OverwritingUI //
    DEBUG["force_load_map"] =           Setting(load_setting<std::filesystem::path>(config_file.u8string(), "OverwritingUI", "force_load_map", "none"));
    // Tests //
    DEBUG["test_logger"] =              Setting(load_setting<bool>(config_file.u8string(), "Tests", "test_logger", false));
    DEBUG["print_locations"] =          Setting(load_setting<bool>(config_file.u8string(), "Tests", "print_locations", false));
};

#endif
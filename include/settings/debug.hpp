#ifndef SETTINGS_DEBUG_H
#define SETTINGS_DEBUG_H

#include "utils/config.hpp"

/* Detailed debug mode settings

*/

inline std::unordered_map<std::string, Setting> DEBUG;

inline void init_debug_settings(std::string config_file) {
    // Renderring //
    DEBUG["show_tile_hiding"] =         Setting(load_setting<bool>(config_file, "Renderring", "show_tile_hiding", false));
    DEBUG["save_texture_atlases"] =     Setting(load_setting<bool>(config_file, "Renderring", "save_texture_atlases", false));
    // Movement //
    DEBUG["show_coords"] =              Setting(load_setting<bool>(config_file, "Movement", "show_coords", false));
    // Logging // 
    DEBUG["test_logger"] =              Setting(load_setting<bool>(config_file, "Logging", "test_logger", false));
    DEBUG["all_debug_logs"] =           Setting(load_setting<bool>(config_file, "Logging", "all_debug_logs", false));
    // OverwritingUI //
    DEBUG["force_load_map"] =           Setting(load_setting<std::string>(config_file, "OverwritingUI", "force_load_map", "none"));
    DEBUG["select_render_state"] =      Setting(load_setting<int>(config_file, "OverwritingUI", "select_render_state", 1));
};

#endif
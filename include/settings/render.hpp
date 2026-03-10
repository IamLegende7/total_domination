#ifndef SETTINGS_RENDER_HPP
#define SETTINGS_RENDER_HPP

#include "utils/config.hpp"

inline std::unordered_map<std::string, Setting> RENDER_SETTINGS;

inline void init_render_settings(std::string config_file) {
    // misc //
    RENDER_SETTINGS["gpu_driver"] =          Setting(load_setting<std::string>(config_file, "Misc", "gpu_driver"));
    RENDER_SETTINGS["render_mode"] =         Setting(load_setting<int>(config_file, "Misc", "render_mode", 0));
    RENDER_SETTINGS["texture_atlas_size"] =  Setting(load_setting<int>(config_file, "Misc", "texture_atlas_size", 32));
    // shaders //
    RENDER_SETTINGS["online_shaders"] =      Setting(load_setting<bool>(config_file, "Shaders", "online_shaders", false));
}

#endif
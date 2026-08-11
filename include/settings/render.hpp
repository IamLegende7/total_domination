#ifndef SETTINGS_RENDER_HPP
#define SETTINGS_RENDER_HPP

#include "utils/config.hpp"
#include "settings/locations.hpp"
#include "status.hpp"

#include <filesystem>

inline std::map<std::string, Setting> RENDER_SETTINGS;

inline void init_render_settings(const std::filesystem::path& config_file) {
    // misc //
    RENDER_SETTINGS["gpu_driver"] =          Setting(load_setting<std::string>(config_file, "Misc", "gpu_driver"));
    RENDER_SETTINGS["render_mode"] =         Setting(load_setting<int>(config_file, "Misc", "render_mode", 1));
    if (STATUS_FORCED_RENDERMODE != -1) RENDER_SETTINGS["render_mode"].set<int>(STATUS_FORCED_RENDERMODE);
    RENDER_SETTINGS["texture_atlas_size"] =  Setting(load_setting<int>(config_file, "Misc", "texture_atlas_size", 32));
    RENDER_SETTINGS["vsync"] =               Setting(load_setting<bool>(config_file, "Misc", "vsync", true));
    RENDER_SETTINGS["resolution"] =          Setting(load_setting<int>(config_file, "Misc", "resolution", 1));
    // shaders //
    RENDER_SETTINGS["shader"] =              Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Shaders", "shader")));
    RENDER_SETTINGS["online_shaders"] =      Setting(load_setting<bool>(config_file, "Shaders", "online_shaders", false));
}

#endif
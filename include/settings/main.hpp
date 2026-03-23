#ifndef SETTINGS_MAIN_HPP
#define SETTINGS_MAIN_HPP

#include "utils/config.hpp"

inline std::unordered_map<std::string, Setting> SETTINGS;

inline void init_main_settings(std::string config_file) {
    // Misc //
    SETTINGS["initial_camera_zoom"] = Setting(load_setting<int>(config_file, "Misc", "initial_camera_zoom", 5));
    // Mulithreading //
    SETTINGS["multithreading"] =      Setting(load_setting<bool>(config_file, "Multithreading", "multithreading", false));
    SETTINGS["num_threads"] =         Setting(load_setting<int>(config_file, "Multithreading", "num_threads", 1));
    // Renderring
    SETTINGS["software_renderring"] = Setting(load_setting<bool>(config_file, "Renderring", "software_renderring", true));
    SETTINGS["gpu_driver"] =          Setting(load_setting<std::string>(config_file, "Renderring", "gpu_driver"));
    // Status //
}

#endif
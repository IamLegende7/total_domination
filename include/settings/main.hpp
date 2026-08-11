#ifndef SETTINGS_MAIN_HPP
#define SETTINGS_MAIN_HPP

#include "utils/config.hpp"
#include "settings/locations.hpp"

#include <filesystem>

inline std::map<std::string, Setting> SETTINGS;

inline void init_main_settings(const std::filesystem::path& config_file) {
    // Misc //
    SETTINGS["font"] =                Setting(replace_locations(load_setting<std::filesystem::path>(config_file, "Misc", "font", "$resource_dir$/fonts/RobotoMono-Regular.ttf")));
    SETTINGS["font_size"] =           Setting(load_setting<int>(config_file, "Misc", "font_size", 50));
    SETTINGS["initial_camera_zoom"] = Setting(load_setting<int>(config_file, "Misc", "initial_camera_zoom", 5));
    // Ticking //
    SETTINGS["max_frame_rate"] =      Setting(load_setting<int>(config_file, "Ticking", "max_frame_rate", 60));
    SETTINGS["tick_rate"] =           Setting(load_setting<int>(config_file, "Ticking", "tick_rate", 20));
    SETTINGS["input_tick_rate"] =     Setting(load_setting<int>(config_file, "Ticking", "input_tick_rate", 5));
    // Mulithreading //
    SETTINGS["multithreading"] =      Setting(load_setting<bool>(config_file, "Multithreading", "multithreading", false));
    SETTINGS["num_threads"] =         Setting(load_setting<int>(config_file, "Multithreading", "num_threads", 1));
    // Input //
    SETTINGS["input_mode"] =          Setting(load_setting<int>(config_file, "Input", "input_mode", 0));
    SETTINGS["camera_speed"] =        Setting(load_setting<int>(config_file, "Input", "camera_speed", 1));
}

#endif
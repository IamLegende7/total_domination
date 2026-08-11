#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>
//#include <SDL3/touch.h>
//#include <SDL3/SDL_gamepad.h>
#include <string>
#include <vector>
#include <filesystem>
#include <SimpleIni.h>

#include "utils/logger.hpp"

#include "main.hpp"

#include "settings/main.hpp"
#include "settings/debug.hpp"
#include "settings/locations.hpp"

#include "inputs/inputs.hpp"
#include "inputs/keybinds.hpp"


InputHandler::InputHandler() {
    load_keybinds(LOCATIONS["config_dir"].get<std::filesystem::path>() / "keybinds.ini");
}

InputHandler::~InputHandler() {

}

bool InputHandler::load_keybinds(const std::filesystem::path& filename) {
    CSimpleIniA ini;
	ini.SetUnicode();

    const std::string filename_str = filename.u8string();
    SI_Error rc = ini.LoadFile(filename_str.c_str());
    if (rc < 0) {
        LOG(LogLevel::Error, "Could not load ini file \"%s\"", filename_str.c_str());
        return false;
    }

    for (size_t i = 0; i < keybind_constructors_count; i++) {
        Keybind& constructor = keybind_constructors[i];
        SDL_Scancode scancode;
        const char* keyname = ini.GetValue(constructor.category.c_str(), constructor.id.c_str());
        if (!keyname) {
            LOG(LogLevel::Warning, "Key %s not found in section %s", constructor.id.c_str(), constructor.category.c_str());
            scancode = constructor.scancode;
        } else {
            scancode = SDL_GetScancodeFromName(keyname); // TODO: make own SDL_Scancode <-> string mapping
        }
        keybinds[i] = {constructor.category, constructor.id, constructor.label, scancode, constructor.function, constructor.delay, constructor.initial_delay};
    }   
    return true;
}

bool InputHandler::update(SDL_Event* event) {
    if (SETTINGS["input_mode"].get<int>() == 0) {
        if (event->type == SDL_EVENT_KEY_DOWN) {
            if (keyboard_state[event->key.scancode] == 0)
                keyboard_state[event->key.scancode] = -2;
        }
        if (event->type == SDL_EVENT_KEY_UP) {
            keyboard_state[event->key.scancode] = 0;
        }
        return true;
    }
    return false;
}

bool InputHandler::process() {
    if (!(TICKS % SETTINGS["input_tick_rate"].get<int>() == 0)) return true;
    if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Processing inputs!");

    if (MODE == 1) {
        if (SETTINGS["input_mode"].get<int>() == 0) {
            return process_game_keyboard();
        }
    }
    return false;
}

bool InputHandler::process_game_keyboard() {
    for (size_t i = 0; i < keybind_count; i++) {
        Keybind& keybind = keybinds[i];
        int8_t& state = keyboard_state[keybind.scancode];
        if (state == -2) { // inital press
            keybind.function();
            if (keybind.initial_delay != -1)
                state = -keybind.initial_delay-2;
            else if (keybind.delay >= 0)
                state = keybind.delay+1;
            else
                state = -1;
        } else if (state == 1) { // held down
            keybind.function();
            state = keybind.delay+1;
        } else if (state > 1) { // normal delay
            state--;
        } else if (state < -2) {// initial delay
            state++;
            if (state >= -2)
                state = 1;
        }
    }
    return true;
}
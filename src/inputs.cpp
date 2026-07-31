#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>
//#include <SDL3/touch.h>
//#include <SDL3/SDL_gamepad.h>

#include <string>

#include "utils/logger.hpp"

#include "main.hpp"

#include "settings/main.hpp"
#include "settings/debug.hpp"

#include "inputs.hpp"

#include "renderring/render_agents.hpp"


// Reloading settings
#include "callback_functions.hpp"

InputHandler::InputHandler() {

}

InputHandler::~InputHandler() {

}

bool InputHandler::update(SDL_Event* event) {
    if (SETTINGS["input_mode"] == 0) {
        if (event->type == SDL_EVENT_KEY_DOWN) {
            keyboard_state[event->key.scancode] = 1;
        }
        if (event->type == SDL_EVENT_KEY_UP) {
            keyboard_state[event->key.scancode] = 0;
        }
        return true;
    }
    return false;
}

bool InputHandler::process() {
    if (!(TICKS % SETTINGS["input_tick_rate"] == 0)) return true;
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Processing inputs!");

    if (MODE == 1) {
        if (SETTINGS["input_mode"] == 0) {
            return process_game_keyboard();
        }
    }
    return false;
}

bool InputHandler::process_game_keyboard() {
    // Reloading settings
    if (keyboard_state[SDL_SCANCODE_R] != 0) {
        LOG(LogLevel::INFO, "Reloading Settings...");
        load_settings();
        SDL_Delay(100);       
    }
    // Camera
    if (keyboard_state[SDL_SCANCODE_LEFT] != 0) {
        CAMERA.x -= SETTINGS["camera_speed"];
        MAIN_RENDER_AGENT->dirty = true;
    }
    if (keyboard_state[SDL_SCANCODE_RIGHT] != 0) {
        CAMERA.x += SETTINGS["camera_speed"];
        MAIN_RENDER_AGENT->dirty = true;
    }
    if (keyboard_state[SDL_SCANCODE_UP] != 0) {
        CAMERA.y -= SETTINGS["camera_speed"];
        MAIN_RENDER_AGENT->dirty = true;
    }
    if (keyboard_state[SDL_SCANCODE_DOWN] != 0) {
        CAMERA.y += SETTINGS["camera_speed"];
        MAIN_RENDER_AGENT->dirty = true;
    }

    return true;
}
#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cmath>
#include <string>

#include "callback_functions.hpp"

#include "main.hpp"
#include "map.hpp"
#include "inputs.hpp"
#include "renderring/render_agents.hpp"
#include "renderring/textures.hpp"
#include "utils/logger.hpp"

#include "settings/debug.hpp"
#include "settings/main.hpp"
#include "settings/render.hpp"


SDL_AppResult SDL_AppIterate(void* appState) {
    static Uint32 last_time = SDL_GetTicks();
    static int ms_per_tick = 1000 / SETTINGS["tick_rate"];
    static int ms_per_frame = 1000 / SETTINGS["max_frame_rate"];
    static int accumulator_tick = 0;
    static int tick_count = 0; // Counting ticks / frames each second for FPS / tick rate calulations
    static int frame_count = 0;
    static int elapsed_time_tick_rate = 0;
    static int elapsed_time_frame_rate = 0;

    Uint32 current_time = SDL_GetTicks();
    int delta_time = current_time - last_time;
    last_time = current_time;
    accumulator_tick += delta_time;


    // Calulating FPS & tick rate
    elapsed_time_tick_rate += delta_time;
    elapsed_time_frame_rate += delta_time;

    if (elapsed_time_tick_rate >= 1000) {
        ACTUAL_TICK_RATE = (tick_count*1000.0f / elapsed_time_tick_rate);
        LOG(LogLevel::DEBUG, "TPS is %d: %d*1000 / %d",
            (int)(std::round(ACTUAL_TICK_RATE)),
            tick_count,
            elapsed_time_tick_rate
        );
        tick_count = 0;
        elapsed_time_tick_rate = 0;
    }

    if (elapsed_time_frame_rate >= 1000) {
        ACTUAL_FRAME_RATE = (frame_count*1000.0f / elapsed_time_frame_rate);
        LOG(LogLevel::DEBUG, "FPS is %d: %d*1000 / %d",
            (int)(std::round(ACTUAL_FRAME_RATE)),
            frame_count,
            elapsed_time_frame_rate
        );
        frame_count = 0;
        elapsed_time_frame_rate = 0;
    }


    // Processing
    while (accumulator_tick >= ms_per_tick) {
        if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Current tick = %d", TICKS);


        if (MODE == 0) {
            if (DEBUG["force_load_map"].get_str() == "none") {
                // Main Menu UI logic
            } else {
                MAIN_MAP = new Map(MAIN_RENDER_AGENT, DEBUG["force_load_map"]);
                CAMERA.zoom = SETTINGS["initial_camera_zoom"];
                MODE = 1;
            }
        }
        INPUTS->process();

        tick_count++;
        accumulator_tick -= ms_per_tick;
        TICKS++;
        if (TICKS > 20) TICKS = 0;
    }

    // Renderring
    if (MODE == 0) {

    } else if (MODE == 1) {
        if (DIRTY_SCREEN) {
            MAIN_RENDER_AGENT->render();
            DIRTY_SCREEN = false;
        }
    }
    frame_count++;

    int elapsed_time = SDL_GetTicks() - current_time;
    if (elapsed_time < ms_per_frame) {
        SDL_Delay(ms_per_frame - elapsed_time);
    }

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void* appState, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        SCREEN_WIDTH = event->window.data1;
        SCREEN_HEIGHT = event->window.data2;
        if (int(RENDER_SETTINGS["render_mode"]) == 1) SDL_SetRenderViewport(RENDERER, NULL);
        DIRTY_SCREEN = true;
    }
    INPUTS->update(event);

    return SDL_APP_CONTINUE;
}


// From init.cpp
SDL_AppResult SDL_AppInit(void** appState, int argc, char** argv) {
    return init(appState, argc, argv);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    quit(appstate, result);
}


#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cmath>
#include <string>

#include "callback_functions.hpp"

#include "main.hpp"
#include "map.hpp"
#include "inputs.hpp"
#include "renderring/render_agents.hpp"
#include "renderring/textures.hpp"
#include "renderring/shaders.hpp"
#include "utils/logger.hpp"

#include "settings/debug.hpp"
#include "settings/main.hpp"
#include "settings/render.hpp"


SDL_AppResult SDL_AppIterate(void* appState) {
    static Uint32 last_time = SDL_GetTicks();
    static int ms_per_tick = 1000 / SETTINGS["tick_rate"];
    static int ms_per_frame = 1000 / SETTINGS["max_frame_rate"];
    static int accumulator_tick = 0;
    static int tick_count = 0; // Counting ticks/frames each second for FPS/tick rate calulations
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
        int last_tick_rate = ACTUAL_TICK_RATE;
        ACTUAL_TICK_RATE = (tick_count*1000.0f / elapsed_time_tick_rate);
        //LOG(LogLevel::DEBUG, "TPS is %d: %d*1000 / %d",
        //    (int)(std::round(ACTUAL_TICK_RATE-0.5)), tick_count, elapsed_time_tick_rate);
        if (DEBUG["show_tps"]) {
            if (last_tick_rate != ACTUAL_TICK_RATE) {
                Text* tps_text = UI_RENDER_AGENT->get_text("TPS");
                if (tps_text != nullptr)
                    TTF_SetTextString(tps_text->text, ("TPS: "+std::to_string((int)(std::round(ACTUAL_TICK_RATE-0.5)))).c_str(), 0);
                UI_RENDER_AGENT->dirty = true;
            }
        }
        tick_count = 0;
        elapsed_time_tick_rate = 0;
    }

    if (elapsed_time_frame_rate >= 1000) {
        int last_frame_rate = ACTUAL_FRAME_RATE;
        ACTUAL_FRAME_RATE = (frame_count*1000.0f / elapsed_time_frame_rate);
        //LOG(LogLevel::DEBUG, "FPS is %d: %d*1000 / %d",
        //    (int)(std::round(ACTUAL_FRAME_RATE-0.5)), frame_count, elapsed_time_frame_rate);
        if (DEBUG["show_fps"]) {
            if (last_frame_rate != ACTUAL_FRAME_RATE) {
                Text* fps_text = UI_RENDER_AGENT->get_text("FPS");
                if (fps_text != nullptr)
                    TTF_SetTextString(fps_text->text, ("FPS: "+std::to_string((int)(std::round(ACTUAL_FRAME_RATE-0.5)))).c_str(), 0);
                UI_RENDER_AGENT->dirty = true;
            }
        }
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
        if (TICKS >= 20) TICKS = 0;
    }

    // Renderring
    if (MODE == 0) {

    } else if (MODE == 1) {
        bool changed_main;
        bool changed_ui;
        changed_main = MAIN_RENDER_AGENT->render(true);
        changed_ui = UI_RENDER_AGENT->render(true, {0, 0, 0, 0});
        if (changed_main && !changed_ui) {
            UI_RENDER_AGENT->render_target();
        } else if (!changed_main && changed_ui) {
            MAIN_RENDER_AGENT->render_target();
            UI_RENDER_AGENT->render_target();
        }
        SDL_RenderPresent(RENDERER);
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
        if (int(RENDER_SETTINGS["render_mode"]) == 1)
            SDL_SetRenderViewport(RENDERER, NULL);

        if (CURRENT_RENDER_STATE == 1) {
            CRTEffectUniforms uniforms;
            SDL_zero(uniforms);
            LOG(LogLevel::DEBUG, "Window Width: %d, Height: %d", SCREEN_WIDTH, SCREEN_HEIGHT);
            uniforms.texture_width = SCREEN_WIDTH;
            uniforms.texture_height = SCREEN_HEIGHT;
            if (!SDL_SetGPURenderStateFragmentUniforms(RENDER_STATES[CURRENT_RENDER_STATE].state, 0, &uniforms, sizeof(uniforms))) {
                SDL_Log("Couldn't set uniform data: %s", SDL_GetError());
                CURRENT_RENDER_STATE = 0;
            }
        }
        MAIN_RENDER_AGENT->dirty = true;
        UI_RENDER_AGENT->dirty = true;
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


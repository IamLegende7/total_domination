#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cmath>
#include <string>
#include <filesystem>

#include "callback_functions.hpp"

#include "main.hpp"
#include "map.hpp"
#include "inputs/inputs.hpp"
#include "renderring/render_agents.hpp"
#include "renderring/textures.hpp"
#include "renderring/shaders.hpp"
#include "utils/logger.hpp"

#include "settings/debug.hpp"
#include "settings/main.hpp"
#include "settings/render.hpp"


SDL_AppResult SDL_AppIterate(void* appState) {
    static Uint32 last_time = SDL_GetTicks();
    static int ms_per_tick = 1000 / SETTINGS["tick_rate"].get<int>();
    static int ms_per_frame = 1000 / SETTINGS["max_frame_rate"].get<int>();
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
        //LOG(LogLevel::Debug, "TPS is %d: %d*1000 / %d",
        //    (int)(std::round(ACTUAL_TICK_RATE-0.5)), tick_count, elapsed_time_tick_rate);
        if (DEBUG["show_tps"].get<bool>()) {
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
        //LOG(LogLevel::Debug, "FPS is %d: %d*1000 / %d",
        //    (int)(std::round(ACTUAL_FRAME_RATE-0.5)), frame_count, elapsed_time_frame_rate);
        if (DEBUG["show_fps"].get<bool>()) {
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
        if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Current tick = %d", TICKS);

        if (MODE == 0) {
            const std::string force_load_map_str = DEBUG["force_load_map"].get<std::filesystem::path>().u8string();
            if (force_load_map_str == "none") {
                // Main Menu UI logic
            } else {
                // map loading:
                // TODO: move to dedecated function
                MAIN_MAP = new Map(MAIN_RENDER_AGENT, DEBUG["force_load_map"].get<std::filesystem::path>());
                CAMERA.zoom = SETTINGS["initial_camera_zoom"].get<int>();
                int y = 20;
                if (DEBUG["show_fps"].get<bool>()) {
                    UI_RENDER_AGENT->add_text("FPS", "FPS: --", "def_font", 20, y);
                    y += 60;
                }
                if (DEBUG["show_tps"].get<bool>()) {
                    UI_RENDER_AGENT->add_text("TPS", "TPS: --", "def_font", 20, y);
                    y += 60;
                }
                if ((SETTINGS["input_mode"].get<int>() == 0) || (SETTINGS["input_mode"].get<int>() == 4)) {
                    // Change to palace actor pos
                    TILE_SELECTION_X = 0;
                    TILE_SELECTION_Y = 0;
                    std::string atlas_ui_textures[] = {
                        "td:selected_tile_top",
                        "td:selected_tile_left",
                        "td:selected_tile_right"
                    };
                    bake_atlas(MAIN_RENDER_AGENT, "atlas:ui", atlas_ui_textures, sizeof(atlas_ui_textures)/sizeof(atlas_ui_textures[0]));
                    MapTile* selected_tile = MAIN_MAP->get_tile(TILE_SELECTION_Y, TILE_SELECTION_X);
                    const int selected_tile_x = (16*(selected_tile->x-selected_tile->y));
                    const int selected_tile_y = (11*(selected_tile->x+selected_tile->y)-(16*(selected_tile->height-1)));
                    const auto [surrounding_height_top, surrounding_height_bottom, surrounding_height_left, surrounding_height_right] = MAIN_MAP->get_surrounding(TILE_SELECTION_Y, TILE_SELECTION_X);
                    const bool hide_left = (selected_tile->height <= surrounding_height_bottom);
                    const bool hide_right = (selected_tile->height <= surrounding_height_right);
                    if (!MAIN_RENDER_AGENT->add_entity("selected_tile_top", "td:selected_tile_top", selected_tile_x, selected_tile_y, -1, 0, false))
                        LOG(LogLevel::Error, "Could not add tile \"selected_tile_top\"");
                    if (!MAIN_RENDER_AGENT->add_entity("selected_tile_left", "td:selected_tile_left", selected_tile_x, selected_tile_y, -1, 0, hide_left))
                        LOG(LogLevel::Error, "Could not add tile \"selected_tile_left\"");
                    if (!MAIN_RENDER_AGENT->add_entity("selected_tile_right", "td:selected_tile_right", selected_tile_x, selected_tile_y, -1, 0, hide_right))
                        LOG(LogLevel::Error, "Could not add tile \"selected_tile_right\"");
                    RenderAgentEntity* selected_tile_top = MAIN_RENDER_AGENT->get_entity("selected_tile_top");
                    if (selected_tile_top != nullptr) {
                        SDL_Rect& selected_tile_top_rect = MAIN_RENDER_AGENT->get_sprite(selected_tile_top->sprite)->texture_rect;
                        CAMERA.x = (selected_tile_top->x+(int)(selected_tile_top_rect.w/2))-(int)((SCREEN_WIDTH/2)/CAMERA.zoom);
                        CAMERA.y = (selected_tile_top->y+(int)(selected_tile_top_rect.h/2))-(int)((SCREEN_HEIGHT/2)/CAMERA.zoom);
                    }
                    MAIN_RENDER_AGENT->dirty = true;
                    UI_RENDER_AGENT->dirty = true;
                }
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
        changed_main = MAIN_RENDER_AGENT->render(CAMERA.zoom, CAMERA.x, CAMERA.y, true, RENDER_SETTINGS["resolution"].get<int>());
        changed_ui = UI_RENDER_AGENT->render(UI_ZOOM, 0, 0, true, RENDER_SETTINGS["resolution"].get<int>(), {0, 0, 0, 0}); // TODO: Only overwrite the area of target tex of the old text if the text changes, not the whole target tex
        if (changed_main || changed_ui) {
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
        if (RENDER_SETTINGS["render_mode"].get<int>() == 1)
            SDL_SetRenderViewport(RENDERER, NULL);

        if (CURRENT_RENDER_STATE == 1) {
            CRTEffectUniforms uniforms;
            SDL_zero(uniforms);
            LOG(LogLevel::Debug, "Window Width: %d, Height: %d", SCREEN_WIDTH, SCREEN_HEIGHT);
            uniforms.texture_width = std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"].get<int>());
            uniforms.texture_height = std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"].get<int>());
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


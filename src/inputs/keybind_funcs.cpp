#include <cstdint>

#include "inputs/keybind_funcs.hpp"
#include "inputs/inputs.hpp"

#include "main.hpp"
#include "map.hpp"

#include "agents.hpp"
#include "player.hpp"
#include "renderring/render_agent.hpp"

#include "utils/logger.hpp"

// Reloading settings
#include "callback_functions.hpp"

// Helper functions
bool update_selected_tile() {
    MapTile* selected_tile = MAIN_MAP->get_tile(TILE_SELECTION_X, TILE_SELECTION_Y);
    if (selected_tile == nullptr) {
        LOG(LogLevel::Warning, "Tile (%d, %d) returned nullptr", TILE_SELECTION_X, TILE_SELECTION_Y);
        return false;
    }
    const int selected_tile_x = 16*(selected_tile->x-selected_tile->y);
    const int selected_tile_y = 11*(selected_tile->x+selected_tile->y)-(16*(selected_tile->height-1));
    if (selected_tile_indicator == nullptr)
        return false;
    if ((selected_tile_indicator->x != selected_tile_x) || (selected_tile_indicator->x != selected_tile_x)) {
        const auto [surrounding_height_top, surrounding_height_bottom, surrounding_height_left, surrounding_height_right] = MAIN_MAP->get_surrounding(TILE_SELECTION_X, TILE_SELECTION_Y);
        selected_tile_indicator->x = selected_tile_x;
        selected_tile_indicator->y = selected_tile_y;
        uint8_t animation = 0;
        if (selected_tile->height > surrounding_height_bottom)
            animation += 1;
        if (selected_tile->height > surrounding_height_right)
            animation += 2;
        selected_tile_indicator->animation = animation;
        SDL_Rect& selected_tile_rect = selected_tile_indicator->sprite->animations[0].texture_rects[0];
        CAMERA.x = (selected_tile_indicator->x+(int)(selected_tile_rect.w/2))-(int)((SCREEN_WIDTH/2)/CAMERA.zoom);
        CAMERA.y = (selected_tile_indicator->y+(int)(selected_tile_rect.h/2))-(int)((SCREEN_HEIGHT/2)/CAMERA.zoom);
    }

    //LOG(LogLevel::Debug, "Selected tile is: (%d, %d)", TILE_SELECTION_X, TILE_SELECTION_Y);
    //LOG(LogLevel::Debug, "Camera pos is: (%d, %d)", CAMERA.x, CAMERA.y);

    return true;
}


// Keybind funcs
namespace TDKeybind {
    void tile_selection_up() {
        if (TILE_SELECTION_Y > 0) {
            TILE_SELECTION_Y--;
            bool update_status = update_selected_tile();
            MAIN_RENDER_AGENT->set_dirty(update_status);
            ACTORS_RENDER_AGENT->set_dirty(update_status);
        }
    }
    void tile_selection_down() {
        MapTile* tile = MAIN_MAP->get_tile(TILE_SELECTION_X, TILE_SELECTION_Y+1, true);
        if (tile)
            if (tile->base != "td:none" || tile->top != "td:none") {
                TILE_SELECTION_Y++;
                bool update_status = update_selected_tile();
                MAIN_RENDER_AGENT->set_dirty(update_status);
                ACTORS_RENDER_AGENT->set_dirty(update_status);
            }
    }
    void tile_selection_left() {
        if (TILE_SELECTION_X > 0) {
            TILE_SELECTION_X--;
            bool update_status = update_selected_tile();
            MAIN_RENDER_AGENT->set_dirty(update_status);
            ACTORS_RENDER_AGENT->set_dirty(update_status);
        }
    }
    void tile_selection_right() {
        MapTile* tile = MAIN_MAP->get_tile(TILE_SELECTION_X+1, TILE_SELECTION_Y, true);
        if (tile)
            if (tile->base != "td:none" || tile->top != "td:none") {
                TILE_SELECTION_X++;
                bool update_status = update_selected_tile();
                MAIN_RENDER_AGENT->set_dirty(update_status);
                ACTORS_RENDER_AGENT->set_dirty(update_status);
            }
    }

    // tmp //
    void reload() {
        LOG(LogLevel::Info, "Reloading Settings...");
        load_settings();
    }
    void next_round() {
        ROUND++;
    }
    void spawn_lumberjack() {
        PLAYERS[CURRENT_PLAYER].actor_handler->spawn_actor("td:industrial_lumberjack", TILE_SELECTION_X, TILE_SELECTION_Y);
    }
}
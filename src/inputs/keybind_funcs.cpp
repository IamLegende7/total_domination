#include "inputs/keybind_funcs.hpp"
#include "inputs/inputs.hpp"

#include "main.hpp"
#include "map.hpp"

#include "agents.hpp"
#include "player.hpp"
#include "renderring/render_agent.hpp"

// Reloading settings
#include "callback_functions.hpp"

// Helper functions
bool update_selected_tile() {
    MapTile* selected_tile = MAIN_MAP->get_tile(TILE_SELECTION_Y, TILE_SELECTION_X);
    if (selected_tile == nullptr)
        return false;
    const int selected_tile_x = 16*(selected_tile->x-selected_tile->y);
    const int selected_tile_y = 11*(selected_tile->x+selected_tile->y)-(16*(selected_tile->height-1));
    if (selected_tile_top == nullptr)
        return false;
    if ((selected_tile_top->x != selected_tile_x) || (selected_tile_top->x != selected_tile_x)) {
        if (selected_tile_left == nullptr)
            return false;
        if (selected_tile_right == nullptr)
            return false;
        const auto [surrounding_height_top, surrounding_height_bottom, surrounding_height_left, surrounding_height_right] = MAIN_MAP->get_surrounding(TILE_SELECTION_Y, TILE_SELECTION_X);
        const bool hide_left = (selected_tile->height <= surrounding_height_bottom);
        const bool hide_right = (selected_tile->height <= surrounding_height_right);
        selected_tile_top->x = selected_tile_x;
        selected_tile_top->y = selected_tile_y;
        if (!hide_left) {
            selected_tile_left->x = selected_tile_x;
            selected_tile_left->y = selected_tile_y;
        } else {
            selected_tile_left->x = -10000;
            selected_tile_left->y = -10000;
        }
        if (!hide_right) {
            selected_tile_right->x = selected_tile_x;
            selected_tile_right->y = selected_tile_y;
        } else {
            selected_tile_right->x = -10000;
            selected_tile_right->y = -10000;
        }
        SDL_Rect& selected_tile_top_rect = selected_tile_top->sprite->animations[0].texture_rects[0];
        CAMERA.x = (selected_tile_top->x+(int)(selected_tile_top_rect.w/2))-(int)((SCREEN_WIDTH/2)/CAMERA.zoom);
        CAMERA.y = (selected_tile_top->y+(int)(selected_tile_top_rect.h/2))-(int)((SCREEN_HEIGHT/2)/CAMERA.zoom);
    }

    //LOG(LogLevel::Debug, "Selected tile is: X: %d Y: %d", TILE_SELECTION_X, TILE_SELECTION_Y);

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
        if (TILE_SELECTION_Y < (int)MAIN_MAP->rows-1) {
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
        if (TILE_SELECTION_X < (int)MAIN_MAP->cols-1) {
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
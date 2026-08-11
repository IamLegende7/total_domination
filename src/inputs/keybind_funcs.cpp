#include "inputs/keybind_funcs.hpp"
#include "inputs/inputs.hpp"

#include "main.hpp"
#include "map.hpp"

#include "renderring/render_agents.hpp"
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
    RenderAgentEntity* selected_tile_top = MAIN_RENDER_AGENT->get_entity("selected_tile_top");
    if (selected_tile_top == nullptr)
        return false;
    if ((selected_tile_top->x != selected_tile_x) || (selected_tile_top->x != selected_tile_x)) {
        RenderAgentEntity* selected_tile_left = MAIN_RENDER_AGENT->get_entity("selected_tile_left");
        if (selected_tile_left == nullptr)
            return false;
        RenderAgentEntity* selected_tile_right = MAIN_RENDER_AGENT->get_entity("selected_tile_right");
        if (selected_tile_right == nullptr)
            return false;
        const auto [surrounding_height_top, surrounding_height_bottom, surrounding_height_left, surrounding_height_right] = MAIN_MAP->get_surrounding(TILE_SELECTION_Y, TILE_SELECTION_X);
        const bool hide_left = (selected_tile->height <= surrounding_height_bottom);
        const bool hide_right = (selected_tile->height <= surrounding_height_right);
        selected_tile_top->x = selected_tile_x;
        selected_tile_top->y = selected_tile_y;
        selected_tile_left->x = selected_tile_x;
        selected_tile_left->y = selected_tile_y;
        selected_tile_left->hidden = hide_left;
        selected_tile_right->x = selected_tile_x;
        selected_tile_right->y = selected_tile_y;
        selected_tile_right->hidden = hide_right;
        SDL_Rect& selected_tile_top_rect = MAIN_RENDER_AGENT->get_sprite(selected_tile_top->sprite)->texture_rect;
        CAMERA.x = (selected_tile_top->x+(int)(selected_tile_top_rect.w/2))-(int)((SCREEN_WIDTH/2)/CAMERA.zoom);
        CAMERA.y = (selected_tile_top->y+(int)(selected_tile_top_rect.h/2))-(int)((SCREEN_HEIGHT/2)/CAMERA.zoom);
    }
    return true;
}


// Keybind funcs
namespace TDKeybind {
    void tile_selection_up() {
        if (TILE_SELECTION_Y > 0) {
            TILE_SELECTION_Y--;
            MAIN_RENDER_AGENT->dirty = update_selected_tile();
        }
    }
    void tile_selection_down() {
        if (TILE_SELECTION_Y < (int)MAIN_MAP->rows-1) {
            TILE_SELECTION_Y++;
            MAIN_RENDER_AGENT->dirty = update_selected_tile();
        }
    }
    void tile_selection_left() {
        if (TILE_SELECTION_X > 0) {
            TILE_SELECTION_X--;
            MAIN_RENDER_AGENT->dirty = update_selected_tile();
        }
    }
    void tile_selection_right() {
        if (TILE_SELECTION_X < (int)MAIN_MAP->cols-1) {
            TILE_SELECTION_X++;
            MAIN_RENDER_AGENT->dirty = update_selected_tile();
        }
    }
    void reload() {
        LOG(LogLevel::Info, "Reloading Settings...");
        load_settings();
    }
}
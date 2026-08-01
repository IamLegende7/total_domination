#ifndef KEYBINDS_HPP
#define KEYBINDS_HPP

#include <SDL3/SDL_scancode.h>

#include "inputs/keybind_struct.hpp"
#include "inputs/keybind_funcs.hpp"

inline Keybind keybind_constructors[] = {
    // Movement
    {"Movement", "tile_selection_up", "Move tile selection up", SDL_SCANCODE_W, TDKeybind::tile_selection_up, 2, 10},
    {"Movement", "tile_selection_down", "Move tile selection down", SDL_SCANCODE_S, TDKeybind::tile_selection_down, 2, 10},
    {"Movement", "tile_selection_left", "Move tile selection left", SDL_SCANCODE_A, TDKeybind::tile_selection_left, 2, 10},
    {"Movement", "tile_selection_right", "Move tile selection right", SDL_SCANCODE_D, TDKeybind::tile_selection_right, 2, 10},
    // Debug
    {"Debug", "reload", "Quick-reload settings", SDL_SCANCODE_R, TDKeybind::reload, -1}
};
inline constexpr std::size_t keybind_constructors_count = sizeof(keybind_constructors)/sizeof(keybind_constructors[0]);

#endif
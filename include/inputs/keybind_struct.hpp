#ifndef KEYBIND_STRUCT_HPP
#define KEYBIND_STRUCT_HPP

#include <SDL3/SDL_scancode.h>
#include <functional>
#include <string>
#include <stdint.h>

struct Keybind {
    std::string category;
    std::string id;
    std::string label;
    SDL_Scancode scancode;
    std::function<void()> function;
    int8_t delay; // 0 = none; -1 = until KEY_UP; in key processing ticks
    int8_t initial_delay=-1; // -1 = none
};


#endif
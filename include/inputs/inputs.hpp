#ifndef INPUTS_HPP
#define INPUTS_HPP

#include <SDL3/SDL_events.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "inputs/keybind_struct.hpp"
#include "inputs/keybinds.hpp"

class InputHandler {
    private:
        Keybind keybinds[keybind_constructors_count];
        const size_t keybind_count = keybind_constructors_count;
        /*
        x<(-2): inital delay (still (-x)-2 many ticks);
        x=-2: initial press
        x=-1: wait until key_up;
        x=0: not pressed;
        x=1: no cooldown and button pressed;
        x>1: delay (still x-1 many ticks)
        */
        int8_t keyboard_state[SDL_SCANCODE_COUNT] = {0};
        bool process_game_keyboard();
    public:
        InputHandler();
        ~InputHandler();

        bool load_keybinds(const std::string& filename);

        bool update(SDL_Event* event);
        bool process();
};

inline InputHandler* INPUTS;

#endif
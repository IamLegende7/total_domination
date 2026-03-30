#ifndef INPUTS_HPP
#define INPUTS_HPP

#include <SDL3/SDL_events.h>

class InputHandler {
    private:
        Uint8 keyboard_state[SDL_SCANCODE_COUNT] = {0};
        bool process_game_keyboard();
    public:
        bool update(SDL_Event* event);
        bool process();

        InputHandler();
        ~InputHandler();
};

inline InputHandler* INPUTS;

#endif
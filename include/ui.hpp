#ifndef UI_HPP
#define UI_HPP

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

struct Text {
    std::string id;
    TTF_Text* text;
    int x = 0;
    int y = 0;
    SDL_Color colour = {255, 255, 255, 255};
};

#endif
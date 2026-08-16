#ifndef SETUP_HPP
#define SETUP_HPP

#include <SDL3/SDL.h>
#include <vector>
#include "renderring/shaders.hpp"

// Windows, Renderer & GPU devices //
inline SDL_Window* WINDOW = nullptr;
inline SDL_Renderer* RENDERER = nullptr;
inline std::vector<RenderState> RENDER_STATES = {RenderState{}};
inline int CURRENT_RENDER_STATE = 0;

// The how manyth tick in a second we are on right now
inline Uint8 TICKS = 0;
inline Uint8 CURRENT_ANIMATION_FRAME = 0;
inline float ACTUAL_FRAME_RATE = 0;
inline float ACTUAL_TICK_RATE = 0;
inline int ACTUAL_ANIMATION_FRAME_RATE = 0;

// 0: main menu; 1: in-game, payer turn; 2: enemy turn; 3: pause menu
inline int MODE = 0;
inline bool PAUSE = false; // TODO: implement

inline int SCREEN_WIDTH = 640;
inline int SCREEN_HEIGHT = 480;

struct Camera {
    int x, y;
    int zoom;
    Camera(): x(0), y(0), zoom(4) {};
    Camera(int x, int y, int zoom): x(x), y(y), zoom(zoom) {};
};

inline Camera CAMERA = Camera(-100, -100, 1); // TODO: gentle camera movement // make pos independent of window size

// UI //
inline int UI_ZOOM = 4;
inline int TILE_SELECTION_X = -1;
inline int TILE_SELECTION_Y = -1;

#endif
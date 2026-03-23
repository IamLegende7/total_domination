#ifndef SETUP_HPP
#define SETUP_HPP

#include <SDL3/SDL.h>

// Windows, Renderer & GPU devices //
inline SDL_Window* WINDOW = nullptr;
inline SDL_Renderer* RENDERER = nullptr;
inline SDL_GPUDevice* GPU = nullptr;

// Frame rate //
const int TARGET_FPS = 60;
const float TIME_STEP = 1.0f / TARGET_FPS;

inline bool DIRTY_SCREEN = true;

// 0: main menu; 1: in-game, payer turn; 2: enemy turn; 3: pause menu
inline int MODE = 0;

inline int SCREEN_WIDTH = 640;
inline int SCREEN_HEIGHT = 480;

struct Camera {
    int x, y;
    int zoom;
    Camera(): x(0), y(0), zoom(4) {};
    Camera(int x, int y, int zoom): x(x), y(y), zoom(zoom) {};
};

inline Camera CAMERA = Camera(-100, -100, 1);

#endif
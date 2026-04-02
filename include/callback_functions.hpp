#ifndef CALLBACK_FUNCTIONS_HPP
#define CALLBACK_FUNCTIONS_HPP

bool load_settings();
SDL_AppResult init(void** appState, int argc, char** argv);
void quit(void *appstate, SDL_AppResult result);

#endif
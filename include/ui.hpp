#ifndef UI_HPP
#define UI_HPP

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include "utils/logger.hpp"
#include "settings/main.hpp"

class Text {
    public:
        Text(const std::string& content, const int& font_size, SDL_Color fg_colour = {0, 0, 0, 255}, SDL_Color bg_colour = {0, 0, 0, 0}) : font{TTF_OpenFont("Roboto-Medium.ttf", 50.0f)}, content(content) {
            if (!font) {
                LOG(LogLevel::ERROR, "Error loading font: %s", SDL_GetError());
            }
        }

        // Called by RenderAgent
        void render(SDL_Surface* dest_surface) {
            if (bg_colour.a != 0) {
                dest_surface = TTF_RenderText_Shaded(font, content.c_str(), 0, fg_colour, bg_colour);
            } else {
                dest_surface = TTF_RenderText_Solid(font, content.c_str(), 0, fg_colour);
            }
        }

        ~Text() {
            if (TTF_WasInit()) {
                TTF_CloseFont(font);
            }
        }
        
        Text(const Text&) = delete;
        Text& operator=(const Text&) = delete;

        private:
            std::string content;
            TTF_Font* font{nullptr};
};

#endif
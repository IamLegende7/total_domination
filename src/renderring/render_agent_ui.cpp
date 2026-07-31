#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <unordered_map>
#include <string>

#include "utils/logger.hpp"
#include "renderring/render_agent.hpp"
#include "ui.hpp"

TTF_Font* RenderAgent::get_font(const std::string& font_name, bool suppress_logs) {
    auto it = fonts.find(font_name);
    if (it != fonts.end()) {
        return it->second;
    }
    if (!suppress_logs)
        LOG(LogLevel::WARNING, "Requested non-existent font \"%s\"", font_name.c_str());
    return nullptr;
}

Text* RenderAgent::get_text(const std::string& id, bool suppress_logs) {
    for (auto& text : texts) {
        if (text.id == id) {
            return &text;
        }
    }
    if (!suppress_logs)
        LOG(LogLevel::WARNING, "Requested non-existent text \"%s\"", id.c_str());
    return nullptr;
}

bool RenderAgent::add_font(const std::string& font_name, const std::string& font_path, const float font_size) {
    if (get_font(font_name, true) != nullptr) {
        LOG(LogLevel::ERROR, "Tried adding font \"%s\" but a font with the same name already exists.", font_name.c_str());
        return false;
    }
    fonts[font_name] = TTF_OpenFont(font_path.c_str(), font_size);
    LOG(LogLevel::INFO, "Added new font \"%s\" from file \"%s\".", font_name.c_str(), font_path.c_str());
    return true;
}

bool RenderAgent::add_text(const std::string& id, const std::string& content, const std::string& font_name, const int x, const int y, const SDL_Color colour) {
    if (text_engine == nullptr) {
        LOG(LogLevel::WARNING, "Text is not enabled on this RenderAgent.");
        return false;
    }
    if (get_text(id, true) != nullptr) {
        LOG(LogLevel::WARNING, "Tried adding text \"%s\" but a text with the same name already exists.", id.c_str());
        return false;
    }
    TTF_Font* font = get_font(font_name, true);
    if (font == nullptr) {
        LOG(LogLevel::WARNING, "Adding text \"%s\" failed: font \"%s\" does not exist.", id.c_str(), font_name.c_str());
        return false;
    }
    TTF_Text* ttf_text = TTF_CreateText(text_engine, font, content.c_str(), 0);
    if (ttf_text == nullptr) {
        LOG(LogLevel::WARNING, "Adding text \"%s\" failed: creating text failed: %s", id.c_str(), SDL_GetError());
        return false;
    }
    Text text = {
        id,
        ttf_text,
        x, y,
        colour
    };
    if (!TTF_SetTextColor(text.text, colour.r, colour.g, colour.b, colour.a)) {
        LOG(LogLevel::WARNING, "Could not set text colour of \"%s\" to {%d, %d, %d, %d}: %s", id.c_str(), colour.r, colour.g, colour.b, colour.a, SDL_GetError());
        text.colour = {255, 255, 255, 255};
    }
    texts.push_back(text);
    return true;
}
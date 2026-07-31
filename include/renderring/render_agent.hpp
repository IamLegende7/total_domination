#ifndef RENDER_AGENT_HPP
#define RENDER_AGENT_HPP

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <variant>
#include <unordered_map>
#include <string>
#include <memory>

#include "main.hpp"
#include "ui.hpp"
#include "renderring/shaders.hpp"

#include "utils/logger.hpp"

#include "settings/render.hpp"
#include "settings/locations.hpp"
#include "settings/debug.hpp"

struct RenderAgentTexture {
    private:
        SDL_Texture* texture;
    public:
        int width, height;

        RenderAgentTexture()
            : texture(static_cast<SDL_Texture*>(nullptr)), width(0), height(0) {};
        RenderAgentTexture(SDL_Texture* texture, int width, int height)
            : texture(texture), width(width), height(height) {};
        void cleanup() {
            SDL_DestroyTexture(texture);
        };

        SDL_Texture* get_texture() {
            return texture;
        };
};

struct RenderAgentSprite {
    std::string texture;
    SDL_FRect texture_rect = {0, 0, 32, 32};

    RenderAgentSprite()
        : texture("atlas_interface"), texture_rect({0, 0, 16, 16}) {}

    RenderAgentSprite(std::string texture, SDL_FRect texture_rect)
        : texture(std::move(texture)), texture_rect(texture_rect) {}

    RenderAgentSprite(const RenderAgentSprite& other) = default;
    RenderAgentSprite& operator=(const RenderAgentSprite& other) = default;
};

struct RenderAgentEntity {
    std::string name;
    std::string sprite;
    int x, y;
    int size;
    int rotation;
    bool follow_map;
    bool use_ui_zoom;

    RenderAgentEntity()
        : name("missing"), sprite("td:missing"), x(-100), y(-100), size(10), rotation(0), follow_map(true), use_ui_zoom(false) {};
    RenderAgentEntity(std::string name, std::string sprite, int x, int y, int size, int rotation=0, bool follow_map=true, bool use_ui_zoom=false)
        : name(name), sprite(sprite), x(x), y(y), size(size), rotation(rotation), follow_map(follow_map), use_ui_zoom(use_ui_zoom) {};
};

struct TextureConstructor {
    std::string name;
    std::string file;
    int x, y;
    int size;
    RenderAgentTexture* texture;

    TextureConstructor()
        : name("td:missing"), file(LOCATIONS["missing_texture_tile"]), x(0), y(0), size(10) {};
    TextureConstructor(std::string name, std::string file, int x, int y, int size=1)
        : name(name), file(file), x(x), y(y), size(size) {};
};

class RenderAgent {
    private:
        std::unordered_map<std::string, RenderAgentTexture> agent_textures;
        std::unordered_map<std::string, RenderAgentSprite> agent_sprites;
        std::vector<RenderAgentEntity> agent_entitys;
        SDL_Renderer* renderer;
        SDL_Texture* target; // Render to this texture first before writing that to the screen
        TTF_TextEngine* text_engine = nullptr;
        std::unordered_map<std::string, TTF_Font*> fonts;
        std::vector<Text> texts;
        
    public:
        bool dirty = true;
        RenderAgent(SDL_Renderer* renderer, bool allow_text=false);
        ~RenderAgent();

        // Textures
        bool add_texture(const std::string& id, const std::string& texture_path);
        RenderAgentTexture load_texture(const std::string& texture_path);
        bool insert_texture(const std::string& id, RenderAgentTexture texture);
        bool texture_exists(const std::string& id);
        RenderAgentTexture* get_texture(const std::string& id, bool suppress_logs=false);
        void drop_texture(const std::string& id);
        RenderAgentTexture bake_texture(TextureConstructor* texture_constructors[], const int array_size, const bool force_file_loading=false);

        // Sprites
        bool add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width=-1, const int& height=-1);
        bool render(bool clear_renderer=true, SDL_Color clear_colour={26, 26, 26, 255});
        void render_target();

        // Entitys
        bool add_entity(const std::string& id, const std::string& sprite_id, const int& x, const int& y, const int& size, const int& rotation=0);
        RenderAgentEntity* get_entity(const std::string& id, bool suppress_logs=false);

        // Text
        TTF_Font* get_font(const std::string& font_name, bool suppress_logs=false);
        Text* get_text(const std::string& id, bool suppress_logs=false);
        bool add_font(const std::string& font_name, const std::string& font_path, const float font_size);
        bool add_text(const std::string& id, const std::string& content, const std::string& font_name, const int x, const int y, const SDL_Color colour = {255, 255, 255, 255});
};

#endif
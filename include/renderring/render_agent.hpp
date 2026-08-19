#ifndef RENDER_AGENT_HPP
#define RENDER_AGENT_HPP

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <variant>
#include <unordered_map>
#include <string>
#include <tuple>
#include <memory>
#include <filesystem>

#include "main.hpp"
#include "ui.hpp"
#include "renderring/shaders.hpp"

#include "utils/logger.hpp"
#include "utils/quadtree.hpp"

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

struct SpriteAnimation {
    SDL_Rect texture_rects[12] = {{0, 0, 16, 16}};
};

struct RenderAgentSprite {
    std::string texture;
    std::unordered_map<std::string, SpriteAnimation> animations;
    SDL_Rect max = {0, 0, 0, 0};

    RenderAgentSprite()
        : texture("atlas_interface"), animations({}) {}

    RenderAgentSprite(std::string texture)
        : texture(std::move(texture)), animations({}) {}
    
    RenderAgentSprite(std::string texture, const std::unordered_map<std::string, SpriteAnimation>& animations)
        : texture(std::move(texture)) {
        for (const auto& [key, animation] : animations) {
            add_animation(key, animation);
        }
    }

    RenderAgentSprite(const RenderAgentSprite& other) = default;
    RenderAgentSprite& operator=(const RenderAgentSprite& other) = default;

    SpriteAnimation* get_animation(const std::string& key, bool suppress_logs) {
        auto it = animations.find(key);
        if (it != animations.end())
            return &it->second;
        if (!suppress_logs)
            LOG(LogLevel::Warning, "Requested non-existent animation \"%s\"", key.c_str());
        return nullptr;
    };

    bool add_animation(const std::string& key, const SpriteAnimation animation) {
        if (get_animation(key, true) != nullptr) {
            LOG(LogLevel::Warning, "Could not add animation \"%s\": already exists.", key.c_str());
            return false;
        }
        animations[key] = animation;
        for (int i = 0; i < 12; ++i) {
            max.x = std::max(max.x, animation.texture_rects[i].x);
            max.y = std::max(max.y, animation.texture_rects[i].y);
            max.w = std::max(max.w, animation.texture_rects[i].w);
            max.h = std::max(max.h, animation.texture_rects[i].h);
        }
        return true;
    };
};

struct RenderAgentEntity {
    std::string name;
    std::string sprite;
    std::string animation;
    int x, y;
    int width, height;
    int layer;
    int rotation;
    bool hidden;

    RenderAgentEntity()
        : name("missing"), sprite("td:missing"), animation("default"), x(-100), y(-100), width(0), height(0), layer(-1), rotation(0), hidden(false) {};
    RenderAgentEntity(std::string name, std::string sprite, std::string animation, int x, int y, int width, int height, int layer, int rotation=0, bool hidden=false)
        : name(name), sprite(sprite), animation(animation), x(x), y(y), width(width), height(height), layer(layer), rotation(rotation), hidden(hidden) {};
};

struct TextureConstructor {
    std::string name;
    std::filesystem::path file;
    int x, y;
    int size;
    RenderAgentTexture* texture;

    TextureConstructor()
        : name("td:missing"), file(LOCATIONS["missing_texture_tile"].get<std::filesystem::path>()), x(0), y(0), size(10) {};
    TextureConstructor(std::string name, std::filesystem::path file, int x, int y, int size=1)
        : name(name), file(file), x(x), y(y), size(size) {};
};

class RenderAgent {
    private:
        std::unordered_map<std::string, RenderAgentTexture> agent_textures;
        std::unordered_map<std::string, RenderAgentSprite> agent_sprites;

        SDL_Renderer* renderer;

        SDL_Texture* target[12] = {nullptr}; // Render to this texture first before writing that to the screen
        
        std::string* tex_cache_name;
        SDL_Texture* tex_cache;

        TTF_TextEngine* text_engine = nullptr;
        std::unordered_map<std::string, TTF_Font*> fonts;
        std::vector<Text> texts;
        
    public:
        QuadtreeNode<RenderAgentEntity> agent_entitys;
        
        int heighest_layer = -1;

        bool dirty[12] = {true};
        int map_width;
        int map_height;
        RenderAgent(SDL_Renderer* renderer, bool allow_text=false);
        ~RenderAgent();
        std::tuple<int, int> set_dimensions(const int cols, const int rows, const int x=0, const int y=0);
        bool render(const int zoom=0, const int x_offset=0, const int y_offset=0, const bool clear_renderer=true, const int resolution=1, SDL_Color clear_colour={26, 26, 26, 255});
        void render_target();
        void set_dirty(bool value=true);

        // Textures
        bool add_texture(const std::string& id, const std::filesystem::path& texture_path);
        RenderAgentTexture load_texture(const std::filesystem::path& texture_path);
        bool insert_texture(const std::string& id, RenderAgentTexture texture);
        bool texture_exists(const std::string& id);
        RenderAgentTexture* get_texture(const std::string& id, bool suppress_logs=false);
        void drop_texture(const std::string& id);
        RenderAgentTexture bake_texture(TextureConstructor* texture_constructors[], const int array_size, const bool force_file_loading=false);

        // Sprites
        bool add_sprite(const std::string& id, const std::string& texture_id, const std::unordered_map<std::string, SpriteAnimation>& animations);
        bool add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width=-1, const int& height=-1);
        RenderAgentSprite* get_sprite(const std::string& id, bool suppress_logs=false);

        // Entitys
        bool add_entity(const std::string& id, const std::string& sprite_id, const std::string& animation, const int& x, const int& y, const int& layer=-1, const int& rotation=0, bool hidden=false, bool movable=false, bool allow_subdivision=true);
        bool trigger_subdivision();
        RenderAgentEntity* get_entity(const std::string& id, bool suppress_logs=false);

        // Text
        TTF_Font* get_font(const std::string& font_name, bool suppress_logs=false);
        Text* get_text(const std::string& id, bool suppress_logs=false);
        bool add_font(const std::string& font_name, const std::filesystem::path& font_path, const float font_size);
        bool add_text(const std::string& id, const std::string& content, const std::string& font_name, const int x, const int y, const SDL_Color colour = {255, 255, 255, 255});
};

#endif
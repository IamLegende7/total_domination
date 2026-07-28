#ifndef RENDER_AGENT_HPP
#define RENDER_AGENT_HPP

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>
#include <variant>
#include <unordered_map>
#include <string>
#include <memory>

#include "main.hpp"
#include "renderring/shaders.hpp"

#include "utils/logger.hpp"

#include "settings/render.hpp"
#include "settings/locations.hpp"
#include "settings/debug.hpp"

struct RenderAgentTexture {
    SDL_Texture* texture;
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
        : texture("atlas_interface"), texture_rect({0, 0, 16, 16}) {};
    RenderAgentSprite(const std::string texture, const SDL_FRect texture_rect)
        : texture(texture), texture_rect(texture_rect) {};
    RenderAgentSprite(const RenderAgentSprite& other)
    : texture(other.texture), texture_rect(other.texture_rect) {}
};

struct RenderAgentEntity {
    std::string name;
    std::string sprite;
    int x, y;
    int size;
    int rotation;

    RenderAgentEntity()
        : name("missing"), sprite("td:missing"), x(-100), y(-100), size(10), rotation(0) {};
    RenderAgentEntity(std::string name, std::string sprite, int x, int y, int size, int rotation=0)
        : name(name), sprite(sprite), x(x), y(y), size(size), rotation(rotation) {};
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
        
    public:
        RenderAgent(SDL_Window* window = WINDOW);
        //~RenderAgent();

        // Textures
        bool add_texture(const std::string& id, const std::string& texture_path);
        RenderAgentTexture load_texture(const std::string& id, const std::string& texture_path);
        bool insert_texture(const std::string& id, RenderAgentTexture texture);
        bool texture_exists(const std::string& id);
        RenderAgentTexture* get_texture(const std::string& id);
        void drop_texture(const std::string& id);
        RenderAgentTexture bake_texture(TextureConstructor* texture_constructors[], const int array_size, const bool force_file_loading=false);

        // Sprites
        bool add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width=-1, const int& height=-1);
        void render(bool clear_renderer=true);

        // Entitys
        bool add_entity(const std::string& id, const std::string& sprite_id, const int& x, const int& y, const int& size, const int& rotation=0);
        RenderAgentEntity* get_entity(const std::string& id);
};

#endif
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
    std::variant<SDL_Texture*, SDL_GPUTexture*> texture;
    int width, height;

    RenderAgentTexture()
        : texture(static_cast<SDL_Texture*>(nullptr)), width(0), height(0) {};
    RenderAgentTexture(SDL_Texture* texture, int width, int height)
        : texture(texture), width(width), height(height) {};
    RenderAgentTexture(SDL_GPUTexture* texture, int width, int height)
        : texture(texture), width(width), height(height) {};
    void cleanup() {
        if (texture.index() == 1) {
            SDL_ReleaseGPUTexture(GPU, std::get<SDL_GPUTexture*>(texture));
        } else {
            SDL_DestroyTexture(std::get<SDL_Texture*>(texture));
        }
    };

    SDL_Texture* get_texture() {
        if (!std::holds_alternative<SDL_Texture*>(texture) && !std::holds_alternative<SDL_GPUTexture*>(texture)) {
            LOG(LogLevel::ERROR, "Trying to access an invalid texture.");
            return nullptr;
        }

        if (texture.index() == 0) {
            return std::get<SDL_Texture*>(texture);
        } else {
            LOG(LogLevel::ERROR, "Tried to load SDL_Texture* while type is SDL_GPUTexture*.");
            return nullptr;
        }
    };

    SDL_GPUTexture* get_gpu_texture() {
        if (!std::holds_alternative<SDL_Texture*>(texture) && !std::holds_alternative<SDL_GPUTexture*>(texture)) {
            LOG(LogLevel::ERROR, "Trying to access an invalid texture.");
            return nullptr;
        }

        if (texture.index() == 1) {
            return std::get<SDL_GPUTexture*>(texture);
        } else {
            LOG(LogLevel::ERROR, "Tried to load SDL_GPUTexture* while type is SDL_Texture*.");
            return nullptr;
        }
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
        : sprite("td:missing"), x(-100), y(-100), size(10), rotation(0) {};
    RenderAgentEntity(std::string sprite, int x, int y, int size, int rotation=0)
        : sprite(sprite), x(x), y(y), size(size), rotation(rotation) {};
};

struct TextureConstructor {
    std::string file;
    int x, y;
    int size;
    RenderAgentTexture* texture;

    TextureConstructor()
        : file(LOCATIONS["missing_texture_tile"]), x(0), y(0), size(10) {};
    TextureConstructor(std::string file, int x, int y, int size=1)
        : file(file), x(x), y(y), size(size) {};
};

class RenderAgent {
    private:
        std::unordered_map<std::string, RenderAgentTexture> agent_textures;
        std::unordered_map<std::string, RenderAgentSprite> agent_sprites;
        std::vector<RenderAgentEntity> agent_entitys;
        SDL_GPUGraphicsPipeline *pipeline = NULL;
        SDL_GPUSampler* sampler = NULL;
        //SDL_GPUTransferBuffer* sprite_data_transfer_buffer = NULL;
        //SDL_GPUBuffer+ sprite_data_buffer = NULL;
        
    public:
        RenderAgent(SDL_Window* window = WINDOW);
        //~RenderAgent();
        RenderAgentTexture load_texture(const std::string& id, const std::string& texture_path);
        bool insert_texture(const std::string& id, RenderAgentTexture texture);
        bool add_texture(const std::string& id, const std::string& texture_path);
        bool add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width, const int& height);
        bool add_entity(const std::string& id, const std::string& sprite_id, const int& x, const int& y, const int& size, const int& rotation=0);
        //void drop_texture(std::string& id);
        void render(bool clear_renderer=true);
        RenderAgentTexture bake_texture_from_file(TextureConstructor* texture_constructors[], const int array_size);
};

#endif
#include "renderring/render_agent.hpp"

// A lot of the basics of SDL3_GPU taken from https://glusoft.com/sdl3-tutorials/display-texture-sdl3_gpu/ and https://glusoft.com/sdl3-tutorials/sprite-batching-sdl3-gpu/

struct SpriteInstance {
    float x, y, z;
    float rotation;
    float w, h, padding_a, padding_b;
    float tex_u, tex_v, tex_w, tex_h;
    float r, g, b, a;
};

RenderAgent::RenderAgent(SDL_Window* window) {};

RenderAgentTexture RenderAgent::load_texture(const std::string& id, const std::string& texture_path) {
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Loading Texture %s", texture_path.c_str());
    SDL_Surface* image_surface = IMG_Load(texture_path.c_str());
    if (image_surface == NULL) {
        LOG(LogLevel::WARNING, "Could not load Texture '%s': %s", texture_path.c_str(), SDL_GetError());
        return RenderAgentTexture();
    }

    if (int(RENDER_SETTINGS["render_mode"]) == 1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(RENDERER, image_surface);
        if (texture == NULL) {
            LOG(LogLevel::WARNING, "Could not create Texture from Surface: %s", SDL_GetError());
        }
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        RenderAgentTexture agent_texture = RenderAgentTexture(texture, image_surface->w, image_surface->h);
        SDL_DestroySurface(image_surface);
        return agent_texture;
    } else if (int(RENDER_SETTINGS["render_mode"]) == 2) {
        LOG(LogLevel::ERROR, "Render Mode \"2\" (Software Renderer) not supportet currently.");
        return RenderAgentTexture();
    }


    return RenderAgentTexture();
};

bool RenderAgent::insert_texture(const std::string& id, RenderAgentTexture texture) {
    agent_textures[id] = texture;
    return true;
}

bool RenderAgent::texture_exists(const std::string& id) {
    return agent_textures.find(id) != agent_textures.end();
}

RenderAgentTexture* RenderAgent::get_texture(const std::string& id) {
    auto it = agent_textures.find(id);
    if (it != agent_textures.end()) {
        return &it->second;
    }

    static RenderAgentTexture fallback{};
    LOG(LogLevel::WARNING, "Requested non-existent texture \"%s\"", id.c_str());
    return &fallback;
}

bool RenderAgent::add_texture(const std::string& id, const std::string& texture_path) {
    return insert_texture(id, load_texture(id, texture_path));
};

bool RenderAgent::add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width, const int& height) {
    int sprite_x = x;
    int sprite_y = y;
    int sprite_width = (width < 0) ? agent_textures[texture_id].width : width;
    int sprite_height = (height < 0) ? agent_textures[texture_id].height: height;

    SDL_FRect srcrect = {static_cast<float>(sprite_x), static_cast<float>(sprite_y), static_cast<float>(sprite_width), static_cast<float>(sprite_height)};
    agent_sprites[id] = RenderAgentSprite(texture_id, srcrect);
    LOG(LogLevel::DEBUG, "Added new Sprite %s: srcrect: %f, %f, %f, %f", id.c_str(), agent_sprites[id].texture_rect.x, agent_sprites[id].texture_rect.y, agent_sprites[id].texture_rect.w, agent_sprites[id].texture_rect.h);
    return true;
};

bool RenderAgent::add_entity(const std::string& id, const std::string& sprite_id, const int& x, const int& y, const int& size, const int& rotation) {
    agent_entitys.push_back(RenderAgentEntity(id, sprite_id, x, y, size, rotation));
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Added new Entity %s: X: %d, Y: %d, Size: %d, Rotation: %d", id.c_str(), x, y, size, rotation);
    return true;
};

void RenderAgent::render(bool clear_renderer) {
    //LOG(LogLevel::DEBUG, "Starting Renderpass");
    if (int(RENDER_SETTINGS["render_mode"]) == 1) {
        if (clear_renderer) {
            SDL_SetRenderDrawColor(RENDERER, 26, 26, 26, 255);
            SDL_RenderClear(RENDERER);
        }

        for (const RenderAgentEntity& entity : agent_entitys) {
            if (
                ((entity.x-CAMERA.x)*CAMERA.zoom > -500) &&
                ((entity.x-CAMERA.x)*CAMERA.zoom < SCREEN_WIDTH+500) &&
                ((entity.y-CAMERA.y)*CAMERA.zoom > -500) &&
                ((entity.y-CAMERA.y)*CAMERA.zoom < SCREEN_HEIGHT+500)
            ) {
                if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Renderring Entity %s at %d;%d", entity.name.c_str(), (entity.x-CAMERA.x)*CAMERA.zoom, (entity.y-CAMERA.y)*CAMERA.zoom);
                // Sprite
                if (!(agent_sprites.find(entity.sprite) != agent_sprites.end())) {
                    LOG(LogLevel::WARNING, "Sprite '%s' not found", entity.sprite.c_str());
                }
                const RenderAgentSprite* sprite = &agent_sprites[entity.sprite];
                SDL_FRect dstrect = {
                    (float)((entity.x-CAMERA.x)*CAMERA.zoom),
                    (float)((entity.y-CAMERA.y)*CAMERA.zoom),
                    (sprite->texture_rect.w*CAMERA.zoom),
                    (sprite->texture_rect.h*CAMERA.zoom)
                };
                SDL_FRect screen_rect = {
                    //(float)(DEBUG["show_tile_hiding"] ? 50 : 0),
                    //(float)(DEBUG["show_tile_hiding"] ? 50 : 0),
                    //(float)(DEBUG["show_tile_hiding"] ? SCREEN_WIDTH-100  : SCREEN_WIDTH),
                    //(float)(DEBUG["show_tile_hiding"] ? SCREEN_HEIGHT-100 : SCREEN_HEIGHT)
                    0.0f,
                    0.0f,
                    (float)(SCREEN_WIDTH),
                    (float)(SCREEN_HEIGHT)
                };
                if (SDL_HasRectIntersectionFloat(&dstrect, &screen_rect)) {
                    // Texture
                    if (!(agent_textures.find(sprite->texture) != agent_textures.end())) {
                        LOG(LogLevel::WARNING, "Texture '%s' not found", sprite->texture.c_str());
                    }
                    SDL_Texture* texture = agent_textures[sprite->texture].get_texture();
                    SDL_RenderTexture(RENDERER, texture, &sprite->texture_rect, &dstrect);
                }
            }
        }

        SDL_RenderPresent(RENDERER);

        if (int(RENDER_SETTINGS["render_mode"]) == 2) {
            SDL_UpdateWindowSurface(WINDOW);
        }

    }
}

RenderAgentTexture RenderAgent::bake_texture(TextureConstructor* texture_constructors[], const int array_size, const bool force_file_loading) {
    int surface_width = 0;
    int surface_height = 0;
    for (int i = 0; i < array_size; ++i) {
        TextureConstructor* constructor = texture_constructors[i];
        if (texture_exists(constructor->name) & !force_file_loading) constructor->texture = &(agent_textures[constructor->name]);
        else                                                         constructor->texture = new RenderAgentTexture(load_texture(constructor->file, constructor->file));
        surface_width = std::max(surface_width, (constructor->texture->width+constructor->x)*constructor->size);
        surface_height = std::max(surface_height, (constructor->texture->height+constructor->y)*constructor->size);
    }
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Bake Surface is %dx%d", surface_width, surface_height);
    if (RENDER_SETTINGS["render_mode"] == 1) {
        SDL_Texture* bake_texture = SDL_CreateTexture(RENDERER, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, surface_width, surface_height);
        SDL_SetTextureScaleMode(bake_texture, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderTarget(RENDERER, bake_texture);
        for (int i = 0; i < array_size; ++i) {
            TextureConstructor* constructor = texture_constructors[i];
            SDL_FRect dstrect = {(float)(constructor->x*constructor->size), (float)(constructor->y*constructor->size), (float)(constructor->texture->width*constructor->size), (float)(constructor->texture->height*constructor->size)};
            SDL_RenderTexture(RENDERER, constructor->texture->get_texture(), NULL, &dstrect);
        }
        SDL_SetRenderTarget(RENDERER, nullptr);
        return RenderAgentTexture(bake_texture, surface_width, surface_height);
    }
    return RenderAgentTexture();
}

void RenderAgent::drop_texture(const std::string& id) {
    agent_textures.erase(id.c_str());
}

RenderAgentEntity* RenderAgent::get_entity(const std::string& id) {
    for (auto& entity : agent_entitys) {
        if (entity.name == id) {
            //LOG(LogLevel::DEBUG, "Found entity %s!", id.c_str());
            return &entity;
        }
    }

    static RenderAgentEntity fallback{};
    LOG(LogLevel::WARNING, "Requested non-existent entity \"%s\"", id.c_str());
    return &fallback;
}
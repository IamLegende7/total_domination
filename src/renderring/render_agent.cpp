#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <filesystem>

#include "renderring/render_agent.hpp"
#include "renderring/shaders.hpp"
#include "ui.hpp"
#include "utils/quadtree.hpp"

RenderAgent::RenderAgent(SDL_Renderer* renderer, bool allow_text) {
    this->renderer = renderer;
    for (int i = 0; i < 12; ++i) {
        target[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"].get<int>()), std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"].get<int>()));
    }
    if (allow_text) {
        if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
            text_engine = TTF_CreateRendererTextEngine(renderer);
        }
    }
    agent_entitys.set_capacity(16);
}

RenderAgent::~RenderAgent() {
    if (text_engine != nullptr) {
        TTF_DestroyRendererTextEngine(text_engine);
    }
    for (auto pair : fonts) {
        if (TTF_WasInit()) {
            TTF_CloseFont(pair.second);
        }
    }
}

std::tuple<int, int> RenderAgent::set_dimensions(const int cols, const int rows, const int x, const int y) {
    int max_width = 0;
    int max_height = 0;
    for (const auto& pair : agent_sprites) {
        max_width = std::max(max_width, pair.second.max.w);
        max_height = std::max(max_height, pair.second.max.h);
    }
    map_width = (max_width+(16*(rows-1)))*2;
    map_height = (max_height+(11*(cols-1+rows-1)))*2;
    agent_entitys.set_dimensions(x, y, map_width, map_height);
    LOG(LogLevel::Debug, "Dimensions: x: %d, y: %d, map_width: %d, map_height: %d", x, y, map_width, map_height);
    return {map_width, map_height};
};

RenderAgentTexture RenderAgent::load_texture(const std::filesystem::path& texture_path) {
    const std::string texture_path_str = texture_path.u8string();
    if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Loading Texture %s", texture_path_str.c_str());
    SDL_Surface* image_surface = IMG_Load(texture_path_str.c_str());
    if (image_surface == NULL) {
        LOG(LogLevel::Warning, "Could not load Texture '%s': %s", texture_path_str.c_str(), SDL_GetError());
        return RenderAgentTexture();
    }

    if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, image_surface);
        if (texture == NULL) {
            LOG(LogLevel::Warning, "Could not create Texture from Surface: %s", SDL_GetError());
        }
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        RenderAgentTexture agent_texture = RenderAgentTexture(texture, image_surface->w, image_surface->h);
        SDL_DestroySurface(image_surface);
        return agent_texture;
    } else if (RENDER_SETTINGS["render_mode"].get<int>() == 2) {
        LOG(LogLevel::Error, "Render Mode \"2\" (Software Renderer) not supportet currently.");
        return RenderAgentTexture();
    }


    return RenderAgentTexture();
}

bool RenderAgent::insert_texture(const std::string& id, RenderAgentTexture texture) {
    agent_textures[id] = texture;
    return true;
}

bool RenderAgent::texture_exists(const std::string& id) {
    return agent_textures.find(id) != agent_textures.end();
}

RenderAgentTexture* RenderAgent::get_texture(const std::string& id, bool suppress_logs) {
    auto it = agent_textures.find(id);
    if (it != agent_textures.end()) {
        return &it->second;
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent texture \"%s\"", id.c_str());
    return nullptr;
}

bool RenderAgent::add_texture(const std::string& id, const std::filesystem::path& texture_path) {
    return insert_texture(id, load_texture(texture_path));
}

bool RenderAgent::add_sprite(const std::string& id, const std::string& texture_id, const std::unordered_map<std::string, SpriteAnimation>& animations) {
    if (get_sprite(id, true) != nullptr) {
        LOG(LogLevel::Warning, "Could not add sprite \"%s\": already exists.", id.c_str());
        return false;
    }
    agent_sprites[id] = RenderAgentSprite(texture_id, animations);
    LOG(LogLevel::Debug, "Added new Sprite %s", id.c_str());
    for (const auto& [key, animation] : animations) {
        LOG(LogLevel::Debug, "   %s", key.c_str());
        for (int i = 0; i < 12; ++i) {
            const SDL_Rect& rect = animation.texture_rects[i];
            LOG(LogLevel::Debug, "      Frame %d: %d, %d, %d, %d", i, rect.x, rect.y, rect.w, rect.h);
        }
    }
    return true;
}


bool RenderAgent::add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width, const int& height) {
    if (get_sprite(id, true) != nullptr) {
        LOG(LogLevel::Warning, "Could not add sprite \"%s\": already exists.", id.c_str());
        return false;
    }
    agent_sprites[id] = RenderAgentSprite(texture_id);

    int sprite_width = (width < 0) ? agent_textures[texture_id].width : width;
    int sprite_height = (height < 0) ? agent_textures[texture_id].height: height;
    SpriteAnimation animation;
    for (int i = 0; i < 12; ++i) {
        animation.texture_rects[i] = {x, y, sprite_width, sprite_height};
    }
    agent_sprites[id].add_animation("default", animation);

    LOG(LogLevel::Debug, "Added new Sprite %s: srcrect: %d, %d, %d, %d", id.c_str(), x, y, sprite_width, sprite_height);
    return true;
}

RenderAgentSprite* RenderAgent::get_sprite(const std::string& id, bool suppress_logs) {
    auto it = agent_sprites.find(id);
    if (it != agent_sprites.end()) {
        return &it->second;
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent sprite \"%s\"", id.c_str());
    return nullptr;
}

bool RenderAgent::add_entity(const std::string& id, const std::string& sprite_id, const std::string& animation, const int& x, const int& y, const int& layer, const int& rotation, bool hidden) {
    const RenderAgentSprite* sprite = get_sprite(sprite_id);
    if (sprite == nullptr) {
        LOG(LogLevel::Warning, "Could not add entity \"%s\": sprite \"%s\" does not exist.", id.c_str(), sprite_id.c_str());
        return false;
    }
    int width = sprite->max.w;
    int height = sprite->max.h;
    int entity_layer;
    if (layer == -1) {
        heighest_layer = heighest_layer+1;
        entity_layer = heighest_layer;
    } else {
        heighest_layer = std::max(heighest_layer, layer);
        entity_layer = layer;
    }
    //LOG(LogLevel::Debug, "Added new entity %s: X: %d, Y: %d, layer: %d, rotation: %d", id.c_str(), x, y, entity_layer, rotation);
    return agent_entitys.insert(id, RenderAgentEntity(id, sprite_id, animation, x, y, width, height, entity_layer, rotation, hidden));
};

bool RenderAgent::render(const int zoom, const int x_offset, const int y_offset, const bool clear_renderer, const int resolution, SDL_Color clear_colour) {
    if (!dirty[CURRENT_ANIMATION_FRAME])
        return false;

    if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
        // Set target //
        if (
            !target[CURRENT_ANIMATION_FRAME]
            || (target[CURRENT_ANIMATION_FRAME]->w != std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"].get<int>()))
            || (target[CURRENT_ANIMATION_FRAME]->h != std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"].get<int>()))
        ) {
            target[CURRENT_ANIMATION_FRAME] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"].get<int>()), std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"].get<int>()));
        }
        SDL_SetRenderTarget(renderer, target[CURRENT_ANIMATION_FRAME]);

        // Clear //
        if (clear_renderer) {
            SDL_SetRenderDrawColor(renderer, clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a);
            SDL_RenderClear(renderer);
        }

        const int screen_x = DEBUG["show_tile_hiding"].get<bool>() ? 100 : 0;
        const int screen_y = DEBUG["show_tile_hiding"].get<bool>() ? 100 : 0;
        const int screen_width = (DEBUG["show_tile_hiding"].get<bool>() ? SCREEN_WIDTH-200  : SCREEN_WIDTH);
        const int screen_height = (DEBUG["show_tile_hiding"].get<bool>() ? SCREEN_HEIGHT-200 : SCREEN_HEIGHT);

        const float inverse_zoom = 1.0f / zoom;
        const float inverse_resolution = 1.0f / resolution;
        const int query_x = (int)std::floor(x_offset + screen_x * inverse_zoom * resolution) - 1;
        const int query_y = (int)std::floor(y_offset + screen_y * inverse_zoom * resolution) - 1;
        const int query_w = (int)std::ceil(screen_width * inverse_zoom * resolution) + 2;
        const int query_h = (int)std::ceil(screen_height * inverse_zoom * resolution) + 2;

        // Render entitys //
        std::vector<RenderAgentEntity*> entitys_on_screen;
        agent_entitys.query(query_x, query_y, query_w, query_h, entitys_on_screen);
        std::sort(entitys_on_screen.begin(), entitys_on_screen.end(),
            [](const RenderAgentEntity* a, const RenderAgentEntity* b) {
                return a->layer < b->layer;
            }
        );
        for (const RenderAgentEntity* entity : entitys_on_screen) {
            if (entity->hidden)
                continue;

            const int real_x = (entity->x-x_offset)*zoom * inverse_resolution;
            const int real_y = (entity->y-y_offset)*zoom * inverse_resolution;

            if (DEBUG["all_debug_logs"].get<bool>())
                LOG(LogLevel::Debug, "Renderring Entity %s at %d;%d", entity->name.c_str(), real_x, real_y);

            // Sprite
            RenderAgentSprite* sprite = get_sprite(entity->sprite);
            if (sprite == nullptr)
                continue;
            // Texture
            if ((tex_cache_name == nullptr) || (tex_cache_name != &sprite->texture)) {
                tex_cache_name = &sprite->texture;
                tex_cache = get_texture(sprite->texture)->get_texture();
                if (tex_cache == nullptr)
                    continue;
            }
            SDL_FRect src_frect;
            SDL_FRect dst_frect = {
                (float)real_x,
                (float)real_y,
                (float)sprite->max.w*zoom * inverse_resolution,
                (float)sprite->max.h*zoom * inverse_resolution
            };
            SDL_RectToFRect(&sprite->animations[entity->animation].texture_rects[CURRENT_ANIMATION_FRAME], &src_frect);
            SDL_RenderTexture(renderer, tex_cache, &src_frect, &dst_frect);
        }

        // Render text //
        for (const Text& text : texts) { // TODO: Caching texts to texture
            TTF_DrawRendererText(text.text, text.x, text.y);
        }

        // Quadtree renderring
        if (DEBUG["show_quadtree"].get<bool>()) {
            agent_entitys.render(renderer, x_offset, y_offset, zoom);
        }
        entitys_on_screen.clear();
    }
    dirty[CURRENT_ANIMATION_FRAME] = false;
    return true;
}

void RenderAgent::render_target() {
    //LOG(LogLevel::Debug, "Starting Renderpass");
    SDL_SetRenderTarget(renderer, NULL);
    RenderState* current_state = &RENDER_STATES[CURRENT_RENDER_STATE];
    SDL_SetGPURenderState(renderer, current_state->state);
    SDL_RenderTexture(renderer, target[CURRENT_ANIMATION_FRAME], NULL, NULL);
    SDL_SetGPURenderState(renderer, NULL);
}

void RenderAgent::set_dirty(bool value) {
    for (bool &frame : dirty) frame = value;
}

RenderAgentTexture RenderAgent::bake_texture(TextureConstructor* texture_constructors[], const int array_size, const bool force_file_loading) {
    int surface_width = 0;
    int surface_height = 0;
    for (int i = 0; i < array_size; ++i) {
        TextureConstructor* constructor = texture_constructors[i];
        if (texture_exists(constructor->name) & !force_file_loading) constructor->texture = &(agent_textures[constructor->name]);
        else                                                         constructor->texture = new RenderAgentTexture(load_texture(constructor->file));
        surface_width = std::max(surface_width, (constructor->texture->width+constructor->x)*constructor->size);
        surface_height = std::max(surface_height, (constructor->texture->height+constructor->y)*constructor->size);
    }
    if (DEBUG["all_debug_logs"].get<bool>())
        LOG(LogLevel::Debug, "Bake Surface is %dx%d", surface_width, surface_height);
    if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
        SDL_Texture* bake_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, surface_width, surface_height);
        SDL_SetTextureScaleMode(bake_texture, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderTarget(renderer, bake_texture);
        for (int i = 0; i < array_size; ++i) {
            TextureConstructor* constructor = texture_constructors[i];
            SDL_FRect dstrect = {(float)(constructor->x*constructor->size), (float)(constructor->y*constructor->size), (float)(constructor->texture->width*constructor->size), (float)(constructor->texture->height*constructor->size)};
            SDL_RenderTexture(renderer, constructor->texture->get_texture(), NULL, &dstrect);
        }
        SDL_SetRenderTarget(renderer, nullptr);
        return RenderAgentTexture(bake_texture, surface_width, surface_height);
    }
    return RenderAgentTexture();
}

void RenderAgent::drop_texture(const std::string& id) {
    agent_textures.erase(id.c_str());
}

RenderAgentEntity* RenderAgent::get_entity(const std::string& id, bool suppress_logs) {
    std::vector<RenderAgentEntity*> result;
    agent_entitys.query_by_id(id, result);
    if (result.size() > 0) {
        //LOG(LogLevel::Debug, "Found entity %s!", id.c_str());
        return result[0];
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent entity \"%s\"", id.c_str());
    return nullptr;
}
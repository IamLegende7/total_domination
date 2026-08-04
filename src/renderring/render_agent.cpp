#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <algorithm>
#include <tuple>

#include "renderring/render_agent.hpp"
#include "renderring/shaders.hpp"
#include "ui.hpp"
#include "utils/quadtree.hpp"

RenderAgent::RenderAgent(SDL_Renderer* renderer, bool allow_text) {
    this->renderer = renderer;
    target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"]), std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"]));
    if (allow_text) {
        if (RENDER_SETTINGS["render_mode"] == 1) {
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
        max_width = std::max(max_width, pair.second.texture_rect.w);
        max_height = std::max(max_height, pair.second.texture_rect.h);
    }
    map_width = (max_width+(16*(rows-1)))*2;
    map_height = (max_height+(11*(cols-1+rows-1)))*2;
    agent_entitys.set_dimensions(x, y, map_width, map_height);
    LOG(LogLevel::DEBUG, "Dimensions: x: %d, y: %d, map_width: %d, map_height: %d", x, y, map_width, map_height);
    return {map_width, map_height};
};

RenderAgentTexture RenderAgent::load_texture(const std::string& texture_path) {
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Loading Texture %s", texture_path.c_str());
    SDL_Surface* image_surface = IMG_Load(texture_path.c_str());
    if (image_surface == NULL) {
        LOG(LogLevel::WARNING, "Could not load Texture '%s': %s", texture_path.c_str(), SDL_GetError());
        return RenderAgentTexture();
    }

    if (int(RENDER_SETTINGS["render_mode"]) == 1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, image_surface);
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
        LOG(LogLevel::WARNING, "Requested non-existent texture \"%s\"", id.c_str());
    return nullptr;
}

bool RenderAgent::add_texture(const std::string& id, const std::string& texture_path) {
    return insert_texture(id, load_texture(texture_path));
}

bool RenderAgent::add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width, const int& height) {
    int sprite_x = x;
    int sprite_y = y;
    int sprite_width = (width < 0) ? agent_textures[texture_id].width : width;
    int sprite_height = (height < 0) ? agent_textures[texture_id].height: height;

    SDL_Rect srcrect = {sprite_x, sprite_y, sprite_width, sprite_height};
    agent_sprites[id] = RenderAgentSprite(texture_id, srcrect);
    LOG(LogLevel::DEBUG, "Added new Sprite %s: srcrect: %d, %d, %d, %d", id.c_str(), agent_sprites[id].texture_rect.x, agent_sprites[id].texture_rect.y, agent_sprites[id].texture_rect.w, agent_sprites[id].texture_rect.h);
    return true;
}

RenderAgentSprite* RenderAgent::get_sprite(const std::string& id, bool suppress_logs) {
    auto it = agent_sprites.find(id);
    if (it != agent_sprites.end()) {
        return &it->second;
    }

    if (!suppress_logs)
        LOG(LogLevel::WARNING, "Requested non-existent sprite \"%s\"", id.c_str());
    return nullptr;
}

bool RenderAgent::add_entity(const std::string& id, const std::string& sprite_id, const int& x, const int& y, const int& layer, const int& rotation, bool hidden) {
    const RenderAgentSprite* sprite = get_sprite(sprite_id);
    if (sprite == nullptr) {
        LOG(LogLevel::WARNING, "Could not add entity \"%s\": sprite \"%s\" does not exist.", id.c_str(), sprite_id.c_str());
        return false;
    }
    int width = sprite->texture_rect.w;
    int height = sprite->texture_rect.h;
    int entity_layer;
    if (layer == -1) {
        heighest_layer = heighest_layer+1;
        entity_layer = heighest_layer;
    } else {
        heighest_layer = std::max(heighest_layer, layer);
        entity_layer = layer;
    }
    //LOG(LogLevel::DEBUG, "Added new entity %s: X: %d, Y: %d, layer: %d, rotation: %d", id.c_str(), x, y, entity_layer, rotation);
    return agent_entitys.insert(id, RenderAgentEntity(id, sprite_id, x, y, width, height, entity_layer, rotation, hidden));
};

void RenderAgent::render_target() {
    SDL_SetRenderTarget(renderer, NULL);
    RenderState* current_state = &RENDER_STATES[CURRENT_RENDER_STATE];
    SDL_SetGPURenderState(renderer, current_state->state);
    SDL_RenderTexture(renderer, target, NULL, NULL);
    SDL_SetGPURenderState(renderer, NULL);
}

bool RenderAgent::render(const int zoom, const int x_offset, const int y_offset, const bool clear_renderer, const int resolution, SDL_Color clear_colour) {
    if (!dirty)
        return false;

    //LOG(LogLevel::DEBUG, "Starting Renderpass");
    if (int(RENDER_SETTINGS["render_mode"]) == 1) {
        // Set target //
        if ((target->w != SCREEN_WIDTH) || (target->h != SCREEN_HEIGHT)) {
            target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"]), std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"]));
        }
        SDL_SetRenderTarget(renderer, target);

        // Clear //
        if (clear_renderer) {
            SDL_SetRenderDrawColor(renderer, clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a);
            SDL_RenderClear(renderer);
        }

        const int screen_x = DEBUG["show_tile_hiding"] ? 100 : 0;
        const int screen_y = DEBUG["show_tile_hiding"] ? 100 : 0;
        const int screen_width = (DEBUG["show_tile_hiding"] ? SCREEN_WIDTH-200  : SCREEN_WIDTH);
        const int screen_height = (DEBUG["show_tile_hiding"] ? SCREEN_HEIGHT-200 : SCREEN_HEIGHT);

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

            if (DEBUG["all_debug_logs"])
                LOG(LogLevel::DEBUG, "Renderring Entity %s at %d;%d", entity->name.c_str(), real_x, real_y);

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
                (float)sprite->texture_rect.w*zoom * inverse_resolution,
                (float)sprite->texture_rect.h*zoom * inverse_resolution
            };
            SDL_RectToFRect(&sprite->texture_rect, &src_frect);
            SDL_RenderTexture(renderer, tex_cache, &src_frect, &dst_frect);
        }

        // Render text //
        for (const Text& text : texts) { // TODO: Caching texts to texture
            TTF_DrawRendererText(text.text, text.x, text.y);
        }

        // Quadtree renderring
        if (DEBUG["show_quadtree"]) { // TODO: make into setting
            agent_entitys.render(renderer, x_offset, y_offset, zoom);
        }
        entitys_on_screen.clear();
    }
    dirty = false;
    return true;
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
    if (DEBUG["all_debug_logs"])
        LOG(LogLevel::DEBUG, "Bake Surface is %dx%d", surface_width, surface_height);
    if (RENDER_SETTINGS["render_mode"] == 1) {
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
        //LOG(LogLevel::DEBUG, "Found entity %s!", id.c_str());
        return result[0];
    }

    if (!suppress_logs)
        LOG(LogLevel::WARNING, "Requested non-existent entity \"%s\"", id.c_str());
    return nullptr;
}
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdint>
#include <deque>
#include <cmath>
#include "rapidjson/document.h"

#include "renderring/render_agent.hpp"
#include "renderring/shaders.hpp"

#include "main.hpp"
#include "registry.hpp"

#include "utils/logger.hpp"
#include "utils/json.hpp"

#include "settings/render.hpp"
#include "settings/locations.hpp"
#include "settings/debug.hpp"

RenderAgent::RenderAgent(SDL_Renderer* renderer, bool allow_text) {
    this->renderer = renderer;
    for (int i = 0; i < animation_frames_count; ++i) {
        target[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_TARGET, std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"].get<int>()), std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"].get<int>()));
    }
    if (allow_text) {
        if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
            text_engine = TTF_CreateRendererTextEngine(renderer);
        }
    }
    agent_quadtree.set_capacity(16);
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



void RenderAgent::set_dimensions(const int& x, const int& y, const int& width, const int& height) {
    agent_quadtree.set_dimensions(x, y, width, height);
    LOG(LogLevel::Debug, "Dimensions: x: %d, y: %d, map_width: %d, map_height: %d", x, y, width, height);
}



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
            target[CURRENT_ANIMATION_FRAME] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_TARGET, std::ceil(SCREEN_WIDTH/RENDER_SETTINGS["resolution"].get<int>()), std::ceil(SCREEN_HEIGHT/RENDER_SETTINGS["resolution"].get<int>()));
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

        std::vector<RenderAgentEntity*> entitys_on_screen;
        agent_quadtree.query(query_x, query_y, query_w, query_h, entitys_on_screen);
        std::sort(entitys_on_screen.begin(), entitys_on_screen.end(),
            [](const RenderAgentEntity* a, const RenderAgentEntity* b) {
                return a->layer < b->layer;
            }
        );
        for (const RenderAgentEntity* entity : entitys_on_screen) {
            if (entity == nullptr) {
                LOG(LogLevel::Warning, "nullptr in entitys_on_screen");
                continue;
            }
            if (entity->layer == -1)
                continue;
            if (!entity->sprite || !entity->sprite->texture || !entity->sprite->texture->texture)
                continue; // TODO: logging maybe

            const int real_x = (entity->x-x_offset)*zoom * inverse_resolution;
            const int real_y = (entity->y-y_offset)*zoom * inverse_resolution;

            //LOG(LogLevel::Debug, "Renderring Entity %s at %d;%d", entity->name.c_str(), real_x, real_y);
            SDL_FRect src_frect;
            SDL_FRect dst_frect = {
                (float)real_x,
                (float)real_y,
                (float)entity->sprite->animations[0].texture_rects[0].w*zoom * inverse_resolution,
                (float)entity->sprite->animations[0].texture_rects[0].h*zoom * inverse_resolution
            };
            SDL_RectToFRect(&entity->sprite->animations[entity->animation].texture_rects[CURRENT_ANIMATION_FRAME], &src_frect);
            if (!SDL_RenderTexture(renderer, entity->sprite->texture->texture, &src_frect, &dst_frect))
                LOG(LogLevel::Warning, "Could not render entity at %d x %d: %s", entity->x, entity->y, SDL_GetError());
        }
        
        // Render text //
        for (const Text& text : texts) { // TODO: Caching texts to texture
            TTF_DrawRendererText(text.text, text.x, text.y);
        }

        // Quadtree renderring
        if (DEBUG["show_quadtree"].get<bool>()) {
            agent_quadtree.render(renderer, x_offset, y_offset, zoom, resolution);
        }
        entitys_on_screen.clear();
    }
    dirty[CURRENT_ANIMATION_FRAME] = false;
    return true;
}

void RenderAgent::render_target() {
    //LOG(LogLevel::Debug, "Starting Renderpass");
    SDL_SetTextureScaleMode(target[CURRENT_ANIMATION_FRAME], SDL_SCALEMODE_NEAREST);
    SDL_SetRenderTarget(renderer, NULL);
    RenderState* current_state = &RENDER_STATES[CURRENT_RENDER_STATE];
    SDL_SetGPURenderState(renderer, current_state->state);
    SDL_RenderTexture(renderer, target[CURRENT_ANIMATION_FRAME], NULL, NULL);
    SDL_SetGPURenderState(renderer, NULL);
}

void RenderAgent::set_dirty(bool value) {
    for (bool &frame : dirty) frame = value;
}



SDL_Texture* RenderAgent::load_texture(const std::string& texture) {
    // Get path
    std::filesystem::path texture_path = get_png_path(texture);
    if (!std::filesystem::exists(texture_path))
        return nullptr;
    if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Loading Texture \"%s\"", texture_path.u8string().c_str());

    // Surface
    SDL_Surface* image_surface = IMG_Load(texture_path.u8string().c_str());
    if (!image_surface) {
        LOG(LogLevel::Warning, "Could not load Texture \"%s\": %s", texture_path.u8string().c_str(), SDL_GetError());
        return nullptr;
    }

    // To texture
    if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, image_surface);
        if (!texture) {
            LOG(LogLevel::Warning, "Could not create Texture from Surface: %s", SDL_GetError());
        }
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(image_surface);
        return texture;
    } else if (RENDER_SETTINGS["render_mode"].get<int>() == 2) {
        LOG(LogLevel::Error, "Render Mode \"2\" (Software Renderer) not supportet currently.");
        return nullptr;
    }

    return nullptr;
}

bool RenderAgent::insert_texture(const std::string& id, RenderAgentTexture texture) {
    agent_textures[id] = std::move(texture);
    LOG(LogLevel::Debug, "New texture \"%s\": ptr: \"%p\"; SDL_Texture*: \"%p\"", id.c_str(), static_cast<void*>(&agent_textures[id]), static_cast<void*>((&agent_textures[id])->texture));
    return true;
}

RenderAgentTexture* RenderAgent::get_texture(const std::string& id, bool suppress_logs) {
    for (auto& [key, texture] : agent_textures) {
        if (key == id)
            return &texture;
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent texture \"%s\"", id.c_str());
    return nullptr;
}

void RenderAgent::drop_texture(const std::string& id) {
    agent_textures.erase(id.c_str());
}

bool RenderAgent::add_texture(const std::string& id, const std::string& texture_path) {
    SDL_Texture* texture = load_texture(texture_path);
    if (!texture)
        return false;
    RenderAgentTexture render_agent_texture{texture};
    insert_texture(id, std::move(render_agent_texture));
    return true;
}

SDL_Texture* RenderAgent::bake_texture(std::vector<TextureConstructor*> constructors) {
    int array_size = constructors.size();
    std::vector<SDL_Texture*> textures = std::vector<SDL_Texture*>(array_size, nullptr);

    // Loading Textures
    for (int i = 0; i < array_size; ++i) {
        if (!constructors[i])
            continue;
        RenderAgentTexture* tmp_texture = get_texture(constructors[i]->texture, true);
        if (!tmp_texture) {
            textures[i] = load_texture(constructors[i]->texture);
            if (!textures[i])
                LOG(LogLevel::Warning, "Could not load texture while baking; skipping.");
        } else {
            textures[i] = tmp_texture->texture;
        }
    } 

    // Size
    int surface_width = 0;
    int surface_height = 0;
    for (int i = 0; i < array_size; ++i) {
        if (!constructors[i])
            continue;
        if (!textures[i])
            continue;
        surface_width = std::max(surface_width, constructors[i]->x+textures[i]->w);
        surface_height = std::max(surface_height, constructors[i]->y+textures[i]->h);
    }
    if (DEBUG["all_debug_logs"].get<bool>())
        LOG(LogLevel::Debug, "Bake surface is %d x %d", surface_width, surface_height);

    // Renderring
    //SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, surface_width, surface_height);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_TARGET, surface_width, surface_height); // Use super small pixelformats
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    for (int i = 0; i < array_size; ++i) {
        if (!constructors[i])
            continue;
        if (!textures[i])
            continue;
        SDL_FRect dstrect = {
            (float)(constructors[i]->x),
            (float)(constructors[i]->y),
            (float)(textures[i]->w),
            (float)(textures[i]->h)
        };
        if (!SDL_RenderTexture(renderer, textures[i], NULL, &dstrect))
            LOG(LogLevel::Warning, "Could not render texture \"%s\": %s", constructors[i]->texture.c_str(), SDL_GetError());
    }

    // Cleanup
    for (int i = 0; i < array_size; ++i) {
        if (textures[i])
            SDL_DestroyTexture(textures[i]);
    }
    SDL_SetRenderTarget(renderer, nullptr);
    return texture;
}

bool RenderAgent::check_texture_ptr(RenderAgentTexture* ptr) {
    bool found = false;
    for (auto& [key, agent_texture] : agent_textures) {
        if (ptr == &agent_texture) {
            found = true;
            LOG(LogLevel::Debug, "Texture \"%s\": ptr: \"%p\"; SDL_Texture*: \"%p\"", key.c_str(), static_cast<void*>(ptr), static_cast<void*>(ptr->texture));
            break;
        }
    }
    return found;
}



std::filesystem::path RenderAgent::get_png_path(const std::string& name) {
    std::filesystem::path path = REGISTRY->get("textures", name, REGISTRY->get("textures", "td:tile_missing", std::filesystem::path("none"))); // TODO: change to "missing" after adding such a texture
    if ((path.extension() == ".json") || (path.extension() == ".jsonc")) {
        rapidjson::Document spritesheet_json = open_json(path);
        if (!spritesheet_json.IsObject()) {
            LOG(LogLevel::Warning, "%s is not a valid sprite sheet: root is not an object!", path.u8string().c_str());
            return LOCATIONS["missing_texture"].get<std::filesystem::path>();
        }
        if (!spritesheet_json.HasMember("texture")) {
            LOG(LogLevel::Warning, "%s is not a valid sprite sheet: it does not contain value \"texture\"", path.u8string().c_str());
            return LOCATIONS["missing_texture"].get<std::filesystem::path>();
        }
        std::filesystem::path texture_path = replace_locations(spritesheet_json["texture"].GetString());
        LOG(LogLevel::Debug, "texture_path extracted from json is: \"%s\"", texture_path.u8string().c_str());
        return texture_path;
    } else {
        return path;
    }
}

bool RenderAgent::find_atlas_pos(RenderAgentTexture* atlas, SDL_Texture* texture, TextureConstructor* constructor) {
    if (!texture)
        return false;
    if (!atlas)
        return false;

    std::vector<int>& rows_x = atlas->rows_x;
    std::vector<int>& rows_h = atlas->rows_h;

    int y = 0;
    int iteration = 0;
    while (iteration <= 1000000) {
        // Rows height is diffrent from textures height
        for (std::size_t row = 0; row < rows_x.size(); ++row) {
            if ((texture->h == rows_h[row]) && (rows_x[row]+texture->w <= atlas->size)) {
                constructor->x = rows_x[row];
                constructor->y = y;
                rows_x[row] += texture->w;
                return true;
            }
            y += rows_h[row];
        }
        // New row, not expanding atlas
        if (y+texture->h <= atlas->size) {
            constructor->x = 0;
            constructor->y = y;
            rows_x.push_back(texture->w);
            rows_h.push_back(texture->h);
            return true;
        }
        // Expanding atlas, starting anew
        int required_size = y + texture->h;
        for (std::size_t row = 0; row < rows_x.size(); ++row) {
            required_size = std::max(
                required_size,
                rows_x[row] + texture->w
            );
        }
        if (required_size <= atlas->size)
            return false;
        atlas->size = required_size;

        iteration++;
    };
    LOG(LogLevel::Error, "Too many iterations! Stopped.");
    return false;
}

bool RenderAgent::bake_atlas(const std::string& atlas_name, const std::vector<std::string> texture_names) {
    LOG(LogLevel::Debug, "Baking Atlas \"%s\"..", atlas_name.c_str());
    const int array_size = texture_names.size();
    std::vector<SDL_Texture*> textures = std::vector<SDL_Texture*>((size_t)array_size, nullptr);
    std::vector<TextureConstructor*> constructors = std::vector<TextureConstructor*>((size_t)array_size, nullptr);
    int atlas_size = RENDER_SETTINGS["texture_atlas_size"].get<int>();
    if (!insert_texture(atlas_name, RenderAgentTexture{nullptr, atlas_size}))
        return false;
    RenderAgentTexture* atlas = get_texture(atlas_name);
    
    // Making constructors
    for (int i = 0; i < array_size; ++i) {
        constructors[i] = new TextureConstructor{texture_names[i], -1, -1};
    }

    // Loading Textures
    for (int i = 0; i < array_size; ++i) {
        RenderAgentTexture* tmp_texture = get_texture(texture_names[i], true);
        if (!tmp_texture) {
            textures[i] = load_texture(texture_names[i]);
            if (!textures[i])
                LOG(LogLevel::Warning, "Could not load texture while baking atlas; skipping.");
        } else {
            textures[i] = tmp_texture->texture;
        }   
    }
    
    // Finding positions
    LOG(LogLevel::Debug, "Finding positions..");
    for (int i = 0; i < array_size; ++i) {
        if (!textures[i])
            continue;
        find_atlas_pos(atlas, textures[i], constructors[i]);
    }
    LOG(LogLevel::Debug, "Done finding positions!");

    // Making sprites
    for (int i = 0; i < array_size; ++i) {
        add_sprite(texture_names[i], atlas_name, constructors[i]->x, constructors[i]->y, textures[i]->w, textures[i]->h, REGISTRY->get("textures", texture_names[i], std::filesystem::path("none")));
    }

    // Saving
    if (DEBUG["save_texture_atlases"].get<bool>()) {
        if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
            SDL_SetRenderTarget(renderer, atlas->texture);
            SDL_Surface* save_surface = SDL_RenderReadPixels(renderer, NULL);
            SDL_SetRenderTarget(renderer, nullptr);
            const std::filesystem::path atlas_file_path = LOCATIONS["log_dir"].get<std::filesystem::path>() / std::filesystem::path("atlas-'" + atlas_name + "'.png");
            const std::string atlas_file_path_str = atlas_file_path.u8string();
            LOG(LogLevel::Debug, "Saving Atlas \"%s\" to \"%s\".", atlas_name.c_str(), atlas_file_path_str.c_str());
            IMG_SavePNG(save_surface, atlas_file_path_str.c_str());
            SDL_DestroySurface(save_surface);
        }
    }

    // Cleanup
    for (int i = 0; i < array_size; ++i) {
        if (textures[i])
            SDL_DestroyTexture(textures[i]);
    }

    // Baking
    atlas->texture = bake_texture(constructors);
    if (!atlas->texture)
        return false;

    LOG(LogLevel::Debug, "Done!");
    return true;
}

bool RenderAgent::add_to_atlas(const std::string& atlas_name, const std::string& texture_name) {
    RenderAgentTexture* atlas = get_texture(atlas_name);
    if (!atlas)
        return false;
    std::vector<TextureConstructor*> constructors{2, nullptr};
    constructors[0] = new TextureConstructor{atlas_name, 0, 0};
    constructors[1] = new TextureConstructor{texture_name, -1, -1};

    SDL_Texture* texture = nullptr;
    RenderAgentTexture* tmp_texture = get_texture(texture_name, true);
    if (!texture) {
        texture = load_texture(texture_name);
        if (!texture) {
            LOG(LogLevel::Warning, "Could not load texture while baking atlas; abording.");
            SDL_DestroyTexture(texture);
            return false;
        }
    } else {
        texture = tmp_texture->texture;
    }

    if (!texture)
        find_atlas_pos(atlas, texture, constructors[1]);

    add_sprite(texture_name, atlas_name, constructors[1]->x, constructors[1]->y, texture->w, texture->h, REGISTRY->get("textures", texture_name, std::filesystem::path("none")));

    // Saving
    if (DEBUG["save_texture_atlases"].get<bool>()) {
        if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
            SDL_SetRenderTarget(RENDERER, atlas->texture);
            SDL_Surface* save_surface = SDL_RenderReadPixels(RENDERER, NULL);
            SDL_SetRenderTarget(RENDERER, nullptr);
            const std::filesystem::path atlas_file_path = LOCATIONS["log_dir"].get<std::filesystem::path>() / std::filesystem::path("atlas-'" + atlas_name + "'.png");
            const std::string atlas_file_path_str = atlas_file_path.u8string();
            LOG(LogLevel::Debug, "Saving Atlas \"%s\" to \"%s\".", atlas_name.c_str(), atlas_file_path_str.c_str());
            IMG_SavePNG(save_surface, atlas_file_path_str.c_str());
            SDL_DestroySurface(save_surface);
        }
    }

    SDL_DestroyTexture(texture);

    atlas->texture = bake_texture(constructors);
    if (!atlas->texture)
        return false;

    return true;
}



RenderAgentSprite* RenderAgent::get_sprite(const std::string& id, bool suppress_logs) {
    auto it = agent_sprites.find(id);

    if (it != agent_sprites.end())
        return &it->second;

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent sprite \"%s\"", id.c_str());
    return nullptr;
}

bool RenderAgent::add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& w, const int& h, const std::filesystem::path& sprite_sheet_path) {
    if (get_sprite(id, true) != nullptr) {
        LOG(LogLevel::Warning, "Could not add sprite \"%s\": already exists.", id.c_str());
        return false;
    }
    std::vector<SpriteAnimation> animations;
    if ((sprite_sheet_path.extension() == ".json") || (sprite_sheet_path.extension() == ".jsonc")) {
        rapidjson::Document sprite_sheet = open_json(sprite_sheet_path);
        for (auto it = sprite_sheet["frames"].MemberBegin(); it != sprite_sheet["frames"].MemberEnd(); ++it) {
            std::string key = it->name.GetString();

            SpriteAnimation animation;
            const rapidjson::Value& array = it->value;
            for (rapidjson::SizeType frame_index = 0; frame_index < array.Size(); ++frame_index) {
                if ((int)frame_index >= animation_frames_count) {
                    LOG(LogLevel::Warning, "Too many frames (>%d) in sprite \"%s\"; animation \"%s\"", animation_frames_count, sprite_sheet_path.u8string().c_str(), key.c_str());
                    break;
                }
                const rapidjson::Value& rect = array[frame_index];
                animation.texture_rects[frame_index] = {
                    x+rect["x"].GetInt(),
                    y+rect["y"].GetInt(),
                    rect["w"].GetInt(),
                    rect["h"].GetInt()
                };
            }
            animations.push_back(animation);
        }
        if (animations.size() == 0) {
            LOG(LogLevel::Error, "Animations is empty while creating spirte \"%s\"", id.c_str());
        }
    } else {
        SpriteAnimation animation;
        for (int i = 0; i < animation_frames_count; ++i) {
            animation.texture_rects[i] = {x, y, w, h};
        }
        animations.push_back(animation);
    }

    RenderAgentTexture* texture = get_texture(texture_id);
    if (!texture)
        return false;
    agent_sprites[id] = RenderAgentSprite{id, texture, animations};
    LOG(LogLevel::Debug, "Added new Sprite %s", id.c_str());
    /*int animation_index = 0;
    for (const auto& animation : animations) {
        LOG(LogLevel::Debug, "   %d", animation_index);
        for (int i = 0; i < 12; ++i) {
            const SDL_Rect& rect = animation.texture_rects[i];
            LOG(LogLevel::Debug, "      Frame %d: %d, %d, %d, %d", i, rect.x, rect.y, rect.w, rect.h);
        }
        animation_index++;
    }*/
    return true;
}

void RenderAgent::list_sprites() {
    for (auto& [key, sprite]: agent_sprites) {
        LOG(LogLevel::Debug, "\"%s\": \"%s\"", key.c_str(), sprite.name.c_str());
    }
}

bool RenderAgent::check_sprite_ptr(RenderAgentSprite* ptr) {
    bool found = false;
    for (auto& [key, agent_sprite] : agent_sprites) {
        if (ptr == &agent_sprite) {
            found = true;
            break;
        }
    }
    return found;
}



RenderAgentEntity* RenderAgent::add_entity(const std::string& sprite_id, const uint8_t& animation, const int& x, const int& y, const int& layer, const int& rotation, bool movable, bool allow_subdivision) {
    RenderAgentEntity entity;
    entity.sprite = get_sprite(sprite_id);
    if (!entity.sprite) {
        LOG(LogLevel::Warning, "Could not add entity at %d x %d: sprite \"%s\" does not exist.", x, y, sprite_id.c_str());
        return nullptr;
    }
    if (layer == -1) {
        heighest_layer = heighest_layer+1;
        entity.layer = heighest_layer;
    } else {
        heighest_layer = std::max(heighest_layer, layer);
        entity.layer = layer;
    }

    entity.x = x;
    entity.y = y;
    entity.animation = animation;
    entity.animation_frame = 0;
    entity.rotation = rotation;
    entity.movable = movable;

    agent_entitys.push_back(entity);
    bool status = agent_quadtree.insert(&agent_entitys.back(), allow_subdivision);
    if (!status) {
        LOG(LogLevel::Warning, "Could not add new entity at X: %d, Y: %d, layer: %d, rotation: %d: quadtree did not accept entry", x, y, entity.layer, rotation);
        return nullptr;
    }
    return &agent_entitys.back();
}

RenderAgentEntity* RenderAgent::insert_entity(RenderAgentEntity& entity, bool allow_subdivision) {
    agent_entitys.push_back(entity);
    bool status = agent_quadtree.insert(&agent_entitys.back(), allow_subdivision);
    if (!status) {
        LOG(LogLevel::Warning, "Could not insert entity at X: %d, Y: %d, layer: %d, rotation: %d: quadtree did not accept entry", entity.x, entity.y, entity.layer, entity.rotation);
        return nullptr;
    }
    return &agent_entitys.back();
}

bool RenderAgent::trigger_subdivision() {
    return agent_quadtree.subdivide();
}

RenderAgentEntity* RenderAgent::get_entity(const int& x, const int& y, bool suppress_logs) {
    std::vector<RenderAgentEntity*> result;
    agent_quadtree.query(x, y, agent_quadtree.width-x, agent_quadtree.height-y, result);
    if (result.size() > 0) {
        return result[0];
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent entity at %d x %d", x, y);
    return nullptr;
}
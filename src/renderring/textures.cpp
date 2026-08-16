#include "renderring/textures.hpp"

#include <SDL3_image/SDL_image.h>
#include <filesystem>
#include <unordered_map>

#include "utils/json.hpp"
#include "settings/locations.hpp"
#include "utils/logger.hpp"

std::filesystem::path get_texture_path(const std::string& name) {
    rapidjson::Document textures_json = open_json(LOCATIONS["textures_json"].get<std::filesystem::path>());
    if (!textures_json.IsObject()) {
        LOG(LogLevel::Warning, "\"%s\" is not a valid textures.json file: root is not an object!", LOCATIONS["textures_json"].get<std::filesystem::path>().u8string().c_str());
        return LOCATIONS["missing_texture"].get<std::filesystem::path>();
    }
    if (!textures_json.HasMember("data")) {
        LOG(LogLevel::Warning, "\"%s\" is not a valid textures.json file: it does not contain value \"data\"", LOCATIONS["textures_json"].get<std::filesystem::path>().u8string().c_str());
        return LOCATIONS["missing_texture"].get<std::filesystem::path>();
    }
    if (!textures_json["data"].IsObject()) {
        LOG(LogLevel::Warning, "\"%s\" is not a valid textures.json file: \"data\" is not an object", LOCATIONS["textures_json"].get<std::filesystem::path>().u8string().c_str());
        return LOCATIONS["missing_texture"].get<std::filesystem::path>();
    }
    if (!textures_json["data"].HasMember(name.c_str())) {
        LOG(LogLevel::Warning, "Could not get texture of \"%s\": \"%s\" does not contain value \"%s\"", name.c_str(), LOCATIONS["textures_json"].get<std::filesystem::path>().u8string().c_str(), name.c_str());
        return LOCATIONS["missing_texture"].get<std::filesystem::path>();
    }
    //if (!textures_json["data"][name.c_str()].IsString()) { // FIXME: Results in core dump
    //    LOG(LogLevel::Warning, "Could not get texture of %s: %s is not a object", name.c_str(), name.c_str());
    //    return LOCATIONS["missing_texture"].get<std::filesystem::path>();
    //}
    return replace_locations(std::filesystem::path(textures_json["data"][name.c_str()].GetString()));
}

std::filesystem::path get_png_path(const std::string& name) {
    std::filesystem::path path = get_texture_path(name);
    std::string path_str = path.u8string();
    if ((path.extension() == ".json") || (path.extension() == ".jsonc")) {
        rapidjson::Document spritesheet_json = open_json(path);
        if (!spritesheet_json.IsObject()) {
            LOG(LogLevel::Warning, "%s is not a valid sprite sheet: root is not an object!", path_str.c_str());
            return LOCATIONS["missing_texture"].get<std::filesystem::path>();
        }
        if (!spritesheet_json.HasMember("texture")) {
            LOG(LogLevel::Warning, "%s is not a valid sprite sheet: it does not contain value \"texture\"", path_str.c_str());
            return LOCATIONS["missing_texture"].get<std::filesystem::path>();
        }
        std::filesystem::path texture_path = replace_locations(spritesheet_json["texture"].GetString());
        LOG(LogLevel::Debug, "texture_path extracted from json is: \"%s\"", texture_path.u8string().c_str());
        return texture_path;
    } else {
        return path;
    }
};

bool add_texture(RenderAgent* agent, const std::string& texture_name) {
    std::filesystem::path texture_path = get_png_path(texture_name);
    return agent->add_texture(texture_name, texture_path);
}

bool bake_atlas(RenderAgent* agent, const std::string& atlas_name, const std::string* texture_names, const int& texture_names_length, const bool force_file_loading) {
    TextureConstructor* constructors[texture_names_length];
    int atlas_size = RENDER_SETTINGS["texture_atlas_size"].get<int>();
    if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Atlas size is %dx%d", atlas_size, atlas_size);

    int rows_y[texture_names_length] = {0};
    int rows_x[texture_names_length] = {0};
    bool found_pos;
    for (int i = 0; i < texture_names_length; ++i) {

        std::string current_texture_name = texture_names[i];
        std::filesystem::path current_texture_path = (agent->texture_exists(current_texture_name) & !force_file_loading)
            ? std::filesystem::path("")
            : get_png_path(current_texture_name);

        std::filesystem::path current_texture_json_path = get_texture_path(current_texture_name);
        bool is_sprite_sheet = !((agent->texture_exists(current_texture_name) & !force_file_loading) || (current_texture_json_path == current_texture_path));

        const std::string current_texture_path_str = current_texture_path.u8string();
        if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "current_texture_path: %s", current_texture_path_str.c_str());

        constructors[i] = new TextureConstructor(current_texture_name, current_texture_path, 0, 0, 1);

        RenderAgentTexture current_texture;
        if (agent->texture_exists(current_texture_name) & !force_file_loading) current_texture = *agent->get_texture(current_texture_name);
        else                                                                   current_texture = agent->load_texture(current_texture_path);

        found_pos = false;
        int current_row = 0;
        int current_y = 0;
        while (!found_pos) {
            if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Current Texture %s: %d|%d :: %d|%d", current_texture_name.c_str(), rows_x[current_row], rows_y[current_row], atlas_size, atlas_size);
            if ((current_texture.height != rows_y[current_row]) and (rows_y[current_row] != 0)) {
                current_y += rows_y[current_row];
                current_row++;
                continue;
            }
            if ((rows_x[current_row]+current_texture.width <= atlas_size) || (current_texture.width > atlas_size)) { // Basic: found position
                if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Texture %s found a position at %d|%d", current_texture_name.c_str(), rows_x[current_row], current_y);
                constructors[i]->x = rows_x[current_row];
                constructors[i]->y = current_y;

                if (is_sprite_sheet) {
                    rapidjson::Document sprite_sheet = open_json(current_texture_json_path);
                    std::unordered_map<std::string, SpriteAnimation> animations;
                    for (auto it = sprite_sheet["frames"].MemberBegin(); it != sprite_sheet["frames"].MemberEnd(); ++it) {
                        std::string key = it->name.GetString();

                        SpriteAnimation animation;
                        const rapidjson::Value& array = it->value;
                        for (rapidjson::SizeType frame_index = 0; frame_index < array.Size(); ++frame_index) {
                            if (frame_index >= 12) {
                                LOG(LogLevel::Warning, "Too many frames (>12) in sprite \"%s\"; animation \"%s\"", current_texture_json_path.u8string().c_str(), key.c_str());
                                break;
                            }
                            const rapidjson::Value& rect = array[frame_index];
                            animation.texture_rects[frame_index] = {
                                constructors[i]->x+rect["x"].GetInt(),
                                constructors[i]->y+rect["y"].GetInt(),
                                rect["w"].GetInt(),
                                rect["h"].GetInt()
                            };
                        }
                        animations.emplace(std::move(key), std::move(animation));
                    }
                    agent->add_sprite(texture_names[i], atlas_name, animations);
                } else {
                    agent->add_sprite(texture_names[i], atlas_name, constructors[i]->x, constructors[i]->y, current_texture.width, current_texture.height);
                }

                found_pos = true;
                if (rows_x[current_row] == 0) { // New Row
                    if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "New row by texture %s", current_texture_name.c_str());
                    rows_y[current_row] = current_texture.height;
                    if (current_y+current_texture.height > atlas_size) {
                        atlas_size = std::max(current_y+current_texture.height, atlas_size);
                    }
                }
                rows_x[current_row] += current_texture.width;
                if (current_texture.width > atlas_size) {
                    atlas_size = current_texture.width;
                }
            } else if (rows_x[current_row] == 0) { // New Row & Expanding atlas
                if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Texture %s found a position at %d|%d", current_texture_name.c_str(), 0, current_y);
                if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "New row by texture %s & expanding the atlas", current_texture_name.c_str());
                constructors[i]->x = 0;
                constructors[i]->y = current_y;

                if (is_sprite_sheet) {
                    rapidjson::Document sprite_sheet = open_json(current_texture_json_path);
                    std::unordered_map<std::string, SpriteAnimation> animations;
                    for (auto it = sprite_sheet.MemberBegin(); it != sprite_sheet.MemberEnd(); ++it) {
                        std::string key = it->name.GetString();

                        SpriteAnimation animation;
                        const rapidjson::Value& array = it->value;
                        for (rapidjson::SizeType i = 0; i < array.Size(); ++i) {
                            if (i >= 12) {
                                LOG(LogLevel::Warning, "Too many frames (>12) in sprite \"%s\"; animation \"%s\"", current_texture_json_path.u8string().c_str(), key.c_str());
                                break;
                            }
                            const rapidjson::Value& rect = array[i];
                            animation.texture_rects[i] = {
                                rect["x"].GetInt(),
                                rect["y"].GetInt(),
                                rect["w"].GetInt(),
                                rect["h"].GetInt()
                            };
                        }
                        animations.emplace(std::move(key), std::move(animation));
                    }
                    agent->add_sprite(texture_names[i], atlas_name, animations);
                } else {
                    agent->add_sprite(texture_names[i], atlas_name, constructors[i]->x, constructors[i]->y, current_texture.width, current_texture.height);
                }

                found_pos = true;
                rows_x[current_row] += current_texture.width;
                rows_y[current_row] = current_texture.height;
            } else {
                //if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Incrementing current_y by %d from %d to %d", rows_y[current_row], current_y, current_y+rows_y[current_row]);
                current_y += rows_y[current_row];
                current_row++;
            }
        }
    }
    RenderAgentTexture atlas_texture = agent->bake_texture(constructors, texture_names_length);
    agent->insert_texture(atlas_name, atlas_texture);
    if (DEBUG["save_texture_atlases"].get<bool>()) {
        if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
            SDL_SetRenderTarget(RENDERER, atlas_texture.get_texture());
            SDL_Surface* save_surface = SDL_RenderReadPixels(RENDERER, NULL);
            SDL_SetRenderTarget(RENDERER, nullptr);
            const std::filesystem::path atlas_file_path = LOCATIONS["log_dir"].get<std::filesystem::path>() / std::filesystem::path("atlas-'" + atlas_name + "'.png");
            const std::string atlas_file_path_str = atlas_file_path.u8string();
            LOG(LogLevel::Debug, "Saving Atlas \"%s\" to \"%s\".", atlas_name.c_str(), atlas_file_path_str.c_str());
            IMG_SavePNG(save_surface, atlas_file_path_str.c_str());
            SDL_DestroySurface(save_surface);
        }
    }
    return true;
}

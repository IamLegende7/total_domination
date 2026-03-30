#include "renderring/textures.hpp"

#include <SDL3_image/SDL_image.h>

#include "utils/json.hpp"
#include "settings/locations.hpp"
#include "utils/logger.hpp"

std::string get_texture_path(const std::string& name) {
    rapidjson::Document textures_json = open_json(LOCATIONS["textures_json"]);
    if (!textures_json.IsObject()) { // <-- Seg fault here
        LOG(LogLevel::WARNING, "%s is not a valid textures.json file: root is not an object!", LOCATIONS["textures_json"].get_c_str());
        return LOCATIONS["missing_texture"];
    }
    if (!textures_json.HasMember("data")) {
        LOG(LogLevel::WARNING, "%s is not a valid textures.json file: it does not contain value 'data'", LOCATIONS["textures_json"].get_c_str());
        return LOCATIONS["missing_texture"];
    }
    if (!textures_json["data"].IsObject()) {
        LOG(LogLevel::WARNING, "%s is not a valid textures.json file: 'data' is not an object", LOCATIONS["textures_json"].get_c_str());
        return LOCATIONS["missing_texture"];
    }
    if (!textures_json["data"].HasMember(name.c_str())) {
        LOG(LogLevel::WARNING, "Could not get texture of %s: %s does not contain value '%s'", name.c_str(), LOCATIONS["textures_json"].get_c_str(), name.c_str());
        return LOCATIONS["missing_texture"];
    }
    //if (!textures_json["data"][name.c_str()].IsString()) { // Results in core dump
    //    LOG(LogLevel::WARNING, "Could not get texture of %s: %s is not a object", name.c_str(), name.c_str());
    //    return LOCATIONS["missing_texture"];
    //}
    std::string texture_path_raw = textures_json["data"][name.c_str()].GetString();
    return replace_locations(texture_path_raw);
};

bool add_texture(RenderAgent* agent, const std::string& texture_name) {
    std::string texture_path = get_texture_path(texture_name);
    return agent->add_texture(texture_name, texture_path);
}

bool bake_atlas(RenderAgent* agent, const std::string& atlas_name, const std::string* texture_names, const int& texture_names_length, const bool force_file_loading) {
    TextureConstructor* constructors[texture_names_length];
    int atlas_size = RENDER_SETTINGS["texture_atlas_size"];
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Atlas size is %dx%d", atlas_size, atlas_size);

    int rows_y[texture_names_length] = {0};
    int rows_x[texture_names_length] = {0};
    bool found_pos;
    for (int i = 0; i < texture_names_length; ++i) {
        std::string current_texture_name = texture_names[i];
        std::string current_texture_path = (agent->texture_exists(current_texture_name) & !force_file_loading) ? "" : get_texture_path(current_texture_name);
        constructors[i] = new TextureConstructor(current_texture_name, current_texture_path, 0, 0, 1);
        if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "current_texture_path: %s", current_texture_path.c_str());
        RenderAgentTexture current_texture;
        if (agent->texture_exists(current_texture_name) & !force_file_loading) current_texture = agent->get_texture(current_texture_name);
        else                                                                   current_texture = agent->load_texture(current_texture_name, current_texture_path);

        found_pos = false;
        int current_row = 0;
        int current_y = 0;
        while (!found_pos) {
            if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Current Texture %s: %d|%d :: %d|%d", current_texture_name.c_str(), rows_x[current_row], rows_y[current_row], atlas_size, atlas_size);
            if ((current_texture.height != rows_y[current_row]) and (rows_y[current_row] != 0)) {
                current_y += rows_y[current_row];
                current_row++;
                continue;
            }
            if ((rows_x[current_row]+current_texture.width <= atlas_size) || (current_texture.width > atlas_size)) { // Basic: found position
                if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Texture %s found a position at %d|%d", current_texture_name.c_str(), rows_x[current_row], current_y);
                constructors[i]->x = rows_x[current_row];
                constructors[i]->y = current_y;
                agent->add_sprite(texture_names[i], atlas_name, constructors[i]->x, constructors[i]->y, current_texture.width, current_texture.height);
                found_pos = true;
                if (rows_x[current_row] == 0) { // New Row
                    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "New row by texture %s", current_texture_name.c_str());
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
                if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Texture %s found a position at %d|%d", current_texture_name.c_str(), 0, current_y);
                if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "New row by texture %s & expanding the atlas", current_texture_name.c_str());
                constructors[i]->x = 0;
                constructors[i]->y = current_y;
                agent->add_sprite(texture_names[i], atlas_name, constructors[i]->x, constructors[i]->y, current_texture.width, current_texture.height);
                found_pos = true;
                rows_x[current_row] += current_texture.width;
                rows_y[current_row] = current_texture.height;
            } else {
                //if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Incrementing current_y by %d from %d to %d", rows_y[current_row], current_y, current_y+rows_y[current_row]);
                current_y += rows_y[current_row];
                current_row++;
            }
        }
    }
    RenderAgentTexture atlas_texture = agent->bake_texture(constructors, texture_names_length);
    agent->insert_texture(atlas_name, atlas_texture);
    if (DEBUG["save_texture_atlases"]) {
        if (RENDER_SETTINGS["render_mode"] == 1) {
            SDL_SetRenderTarget(RENDERER, atlas_texture.get_texture());
            SDL_Surface* save_surface = SDL_RenderReadPixels(RENDERER, NULL);
            SDL_SetRenderTarget(RENDERER, nullptr);
            std::string atlas_file_path = LOCATIONS["log_dir"].get_str()+"/atlas-'"+atlas_name+"'.png";
            IMG_SavePNG(save_surface, atlas_file_path.c_str());
            SDL_DestroySurface(save_surface);
        }
    }
    return true;
}

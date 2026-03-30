#include "map.hpp"

#include <SDL3/SDL.h>
#include <set>
#include <SDL3/SDL_timer.h>
#include "rapidjson/document.h"

#include "utils/json.hpp"
#include "renderring/textures.hpp"
#include "settings/locations.hpp"
#include "settings/main.hpp"
#include "settings/debug.hpp"
#include "utils/logger.hpp"

Map::Map(RenderAgent* agent, const std::string& map_path) {
    LOG(LogLevel::INFO, "Loading map \"%s\"", replace_locations(map_path).c_str());
    Uint64 load_start_time = SDL_GetPerformanceCounter();
    Uint64 preformance_frequency = SDL_GetPerformanceFrequency();

    this->agent = agent;
    // CHECKS AND BASIC DATA //
    this->map_path = map_path;
    const rapidjson::Document& map_json = open_json(replace_locations(map_path));
    if (!map_json.IsObject()) {
        LOG(LogLevel::ERROR, "Could not load map %s: root is not an object.", map_path.c_str());
        return;
    }
    if (!map_json.HasMember("name")) {
        LOG(LogLevel::ERROR, "Could not load map %s: json does not contain a \"name\" object.", map_path.c_str());
        return;
    }
    map_name = std::string(map_json["name"].GetString());
    if (map_json.HasMember("description")) {
        map_description = std::string(map_json["description"].GetString());
    } else {
        map_description = "No discription given";
    }
    if (!map_json.HasMember("data")) {
        LOG(LogLevel::ERROR, "Could not load map %s: json does not contain a \"data\" object.", map_name.c_str());
        return;
    }
    if (map_json["data"].Size() == 0) {
        LOG(LogLevel::ERROR, "Could not load map %s: \"data\" object can not be empty.", map_name.c_str());
        return;
    }

    // DECLARATIONS //
    if (map_json.HasMember("declarations")) {
        if (map_json["declarations"].HasMember("textures")) {
            for (const auto& declaration : map_json["declarations"]["textures"].GetObject()) {
                TextureConstructor* texture_constructors[declaration.value.Size()];
                for (size_t constructor_index = 0; constructor_index < declaration.value.Size(); ++constructor_index) {
                    texture_constructors[constructor_index] = new TextureConstructor(
                        declaration.value[constructor_index].HasMember("texture") ?
                            declaration.value[constructor_index]["texture"].GetString() :
                            "td:missing",
                        agent->texture_exists(declaration.value[constructor_index]["texture"].GetString()) ?
                            "td:none" :
                            (declaration.value[constructor_index].HasMember("texture") ?
                                get_texture_path(declaration.value[constructor_index]["texture"].GetString()) :
                                get_texture_path("td:missing")),
                        declaration.value[constructor_index].HasMember("x") ?
                            declaration.value[constructor_index]["x"].GetInt() :
                            0,
                        declaration.value[constructor_index].HasMember("y") ?
                            declaration.value[constructor_index]["y"].GetInt() :
                            0,
                        declaration.value[constructor_index].HasMember("size") ?
                            declaration.value[constructor_index]["size"].GetInt() :
                            1
                    );
                }
                agent->insert_texture(declaration.name.GetString(), agent->bake_texture(texture_constructors, (sizeof(texture_constructors) / sizeof(texture_constructors[0]))));
            }
        }
    }

    // LOADING MAP DATA //
    // Allocate //
    rows = map_json["data"].Size();
    cols = map_json["data"][0].Size();
    map_data = new MapTile*[rows];

    // Load //
    if (SETTINGS["multithreading"]) {
        // TODO: multithreading
    } else {
        std::set<std::string> tile_textures;
        for (size_t index = 0; index < map_json["data"].Size(); ++index) {
            cols = std::max(int(cols), int(map_json["data"][index].Size()));
            map_data[index] = new MapTile[map_json["data"][index].Size()];
            load_row(map_json["data"][index], index, tile_textures);
        }
        tile_textures.erase("td:none");

        std::string tile_texture_names[tile_textures.size()];
        int tile_texture_index = 0;
        for (auto texture = tile_textures.begin(); texture != tile_textures.end(); ++texture) {
            tile_texture_names[tile_texture_index] = *texture;
            tile_texture_index++;
        }

        const std::string atlas_tile_textures_name = "map:"+map_name+":atlas:tile_textures";
        bake_atlas(agent, atlas_tile_textures_name, tile_texture_names, tile_textures.size());
    }

    // CLEANUP //
    if (map_json.HasMember("declarations")) {
        if (map_json["declarations"].HasMember("textures")) {
            for (const auto& declaration : map_json["declarations"]["textures"].GetObject()) {
                agent->drop_texture(declaration.name.GetString());
            }
        }
    }

    Uint64 elapsed_ticks = SDL_GetPerformanceCounter() - load_start_time;
    double elapsed_ms = (elapsed_ticks / (double)preformance_frequency) * 1000.0;
    LOG(LogLevel::INFO, "Loaded Map \"%s\" in %f ms", map_name.c_str(), elapsed_ms);
}

Map::~Map() {
    for (size_t index = 0; index < rows; ++index) {
        delete[] map_data[index];
    }
    delete[] map_data;
}



bool Map::load_row(const rapidjson::GenericValue<rapidjson::UTF8<>>& row_json, const size_t row, std::set<std::string>& tile_textures) {
    for (size_t col_index = 0; col_index < row_json.Size(); ++col_index) {
        const auto& tile_json = (row_json[col_index].IsObject()) ? row_json[col_index] : row_json[col_index][0];

        // Checking for errors //
        if ((!row_json[col_index].IsArray()) && (!row_json[col_index].IsObject())) {
            LOG(LogLevel::ERROR, "Could not load row %d of map %s: tile %d;%d seems be neither an object nor an array", int(row), map_name.c_str(), int(col_index), int(row));
            return false;
        }
        if (!(tile_json.HasMember("base"))) {
            LOG(LogLevel::ERROR, "Could not load row %d of map %s: tile %d;%d seems to be missing a \"base\" object.", int(row), map_name.c_str(), int(col_index), int(row));
            return false;
        }

        // Loading tile //
        MapTile current_tile = MapTile(tile_json["base"].GetString(), int(col_index), int(row));
        current_tile.top_tile = (tile_json.HasMember("top_tile")) ? tile_json["top_tile"].GetString() : "td:none";
        current_tile.top = (tile_json.HasMember("top"))           ? tile_json["top"].GetString()      : "td:none";
        current_tile.height = (tile_json.HasMember("height"))     ? tile_json["height"].GetInt()      : 1;
        current_tile.size = (tile_json.HasMember("size"))         ? tile_json["size"].GetInt()        : 1;
        // TODO: loading resoures

        map_data[row][col_index] = current_tile;

        tile_textures.insert(current_tile.base);
        tile_textures.insert(current_tile.top_tile);
        tile_textures.insert(current_tile.top);
    
        std::string map_entity_name = "map:"+map_name+":tile:"+std::to_string(current_tile.x)+"x"+std::to_string(current_tile.y);
        for (int height_index = 0; height_index < current_tile.height; ++height_index) {
            if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Entity %s: height_index = %d; y = %d - 16*%d + 11*%d = %d", map_entity_name.c_str(), height_index, current_tile.y, height_index, current_tile.x, current_tile.y-(16*height_index)+(11*current_tile.x));
            if ((current_tile.top_tile != "td:none") && (height_index == current_tile.height-1)) {
                agent->add_entity(map_entity_name+":top_tile", current_tile.top_tile, 16*(current_tile.x-current_tile.y), 11*(current_tile.y+current_tile.x)-(16*height_index), current_tile.size*4);
            } else {
                agent->add_entity(map_entity_name+":base", current_tile.base, 16*(current_tile.x-current_tile.y), 11*(current_tile.y+current_tile.x)-(16*height_index), current_tile.size*4);
            }
        }

        // Loading building // // TODO: load any actor
        if (row_json[col_index].IsArray() && (row_json[col_index].Size() > 1)) {
            if (row_json[col_index].Size() > 2) {
                LOG(LogLevel::WARNING, "Tile %d;%d of map %s has more than 1 building! Only the first building will be loaded.", int(col_index), int(row), map_name.c_str());
            }
            if (!row_json[col_index][1].IsObject()) {
                LOG(LogLevel::ERROR, "Could not load building on tile %d;%d of map %s: building has to be a json object.", int(col_index), int(row), map_name.c_str());
            } else {
                if (!row_json[col_index][1].HasMember("type")) {
                    LOG(LogLevel::ERROR, "Could not load building on tile %d;%d of map %s: no \"type\" object found.", int(col_index), int(row), map_name.c_str());
                } else {
                    if (!(std::string(row_json[col_index][1]["type"].GetString()) == "td:Building")) {
                        LOG(LogLevel::ERROR, "Could not load building on tile %d;%d of map %s: type is not of \"td:Building\" but is \"%s\".", int(col_index), int(row), map_name.c_str(), row_json[col_index][1]["type"].GetString());
                    } else {
                        LOG(LogLevel::WARNING, "Building / Actor loading not implemented yet!");
                    }
                }
            }
        }
    }
    return true;
}
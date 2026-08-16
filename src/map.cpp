#include "map.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>

#include <set>
#include <string>
#include <vector>
#include <tuple>
#include <SDL3/SDL_timer.h>
#include "BS_thread_pool.hpp"
#include <atomic>
#include <filesystem>
#include "rapidjson/document.h"

#include "utils/json.hpp"
#include "renderring/textures.hpp"
#include "settings/locations.hpp"
#include "settings/main.hpp"
#include "settings/debug.hpp"
#include "utils/logger.hpp"

// Windows is stupid
#ifdef GetObject
#undef GetObject
#endif

std::tuple<int, int, int, int> Map::get_surrounding(const int row, const int col) {
    return std::tuple<int, int, int, int> {
        (row > 0)
            ? map_data[row-1][col].height
            : -1,
        (row < (int)rows-1)
            ? map_data[row+1][col].height
            : -1,
        (col > 0)
            ? map_data[row][col-1].height
            : -1,
        (col < (int)cols-1)
            ? map_data[row][col+1].height
            : -1
    };
}

MapTile* Map::get_tile(const int row, const int col, const bool suppress_logs) {
    if (!(row < (int)rows) || !(col < (int)cols)) {
        if (!suppress_logs)
            LOG(LogLevel::Warning, "Requested non-existent tile at %dx%d", col, row);
        return nullptr;
    }   
    return &map_data[row][col];
}

void Map::add_tile_enity(RenderAgent* agent, const std::string& name, const std::string& sprite_id, const int& x, const int& y, const int& height_index) {
    RenderAgentSprite* sprite = agent->get_sprite(sprite_id);
    if (sprite != nullptr) {
        std::vector<std::string> keys;
        for (auto& [key, animation] : sprite->animations) {
            if (key.rfind("alt", 0) == 0) {
                keys.push_back(key);
            }
        }
        std::string selected_animation = "default";
        if (!keys.empty()) {
            int random_index = SDL_rand(keys.size());
            selected_animation = keys[random_index];
        }
        agent->add_entity(
            name,
            sprite_id,
            selected_animation,
            (16*(x-y)),
            (11*(y+x)-(16*height_index)),
            -1
        );
    }
}

Map::Map(RenderAgent* agent, const std::filesystem::path& map_path) {
    const std::string map_path_str = replace_locations(map_path).u8string();
    LOG(LogLevel::Info, "Loading map \"%s\"", map_path_str.c_str());
    Uint64 load_start_time = SDL_GetPerformanceCounter();
    Uint64 preformance_frequency = SDL_GetPerformanceFrequency();

    this->agent = agent;
    // CHECKS AND BASIC DATA //
    this->map_path = map_path;
    const rapidjson::Document& map_json = open_json(replace_locations(map_path));
    if (!map_json.IsObject()) {
        LOG(LogLevel::Error, "Could not load map %s: root is not an object.", map_path_str.c_str());
        return;
    }
    if (!map_json.HasMember("name")) {
        LOG(LogLevel::Error, "Could not load map %s: json does not contain a \"name\" object.", map_path_str.c_str());
        return;
    }
    map_name = std::string(map_json["name"].GetString());
    if (map_json.HasMember("description")) {
        map_description = std::string(map_json["description"].GetString());
    } else {
        map_description = "No discription given";
    }
    if (!map_json.HasMember("data")) {
        LOG(LogLevel::Error, "Could not load map %s: json does not contain a \"data\" object.", map_name.c_str());
        return;
    }
    if (map_json["data"].Size() == 0) {
        LOG(LogLevel::Error, "Could not load map %s: \"data\" object can not be empty.", map_name.c_str());
        return;
    }

    add_texture(agent, "td:tile_missing");
    add_texture(agent, "td:top_missing");

    // DECLARATIONS //
    if (map_json.HasMember("declarations")) {
        if (map_json["declarations"].HasMember("textures")) {
            for (const auto& declaration : map_json["declarations"]["textures"].GetObject()) {
                TextureConstructor* texture_constructors[declaration.value.Size()];
                for (size_t constructor_index = 0; constructor_index < declaration.value.Size(); ++constructor_index) {
                    texture_constructors[constructor_index] = new TextureConstructor(
                        declaration.value[constructor_index].HasMember("texture") ?
                            declaration.value[constructor_index]["texture"].GetString() :
                            "td:tile_missing",
                        agent->texture_exists(declaration.value[constructor_index]["texture"].GetString()) ?
                            "td:none" :
                            (declaration.value[constructor_index].HasMember("texture") ?
                                get_png_path(declaration.value[constructor_index]["texture"].GetString()) :
                                get_png_path("td:tile_missing")),
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
    cols = 0;
    map_data = new MapTile*[rows];

    // Load //
    std::set<std::string> tile_textures;
    if (SETTINGS["multithreading"].get<bool>()) {
        for (size_t r = 0; r < rows; ++r) {
            cols = std::max(cols, (size_t)map_json["data"][r].Size());
        }
        // cols is only informational for now; ok to keep.

        const int configured_threads = SETTINGS["num_threads"].get<int>();
        const size_t num_threads = (configured_threads == -1)
            ? std::max(1u, std::thread::hardware_concurrency())
            : (size_t)std::max(1, configured_threads);

        BS::thread_pool pool(num_threads);

        std::vector<std::set<std::string>> thread_texture_sets(num_threads);
        std::atomic<bool> ok{true};

        // IMPORTANT: store futures so we don't ignore nodiscard
        std::vector<std::future<void>> futures;
        futures.reserve(rows);

        for (size_t r = 0; r < rows; ++r) {
            futures.emplace_back(
                pool.submit_task([this, &map_json, r, &ok, &thread_texture_sets, num_threads]() {
                    if (!ok.load(std::memory_order_relaxed)) return;

                    size_t bucket = r % num_threads;

                    const auto& row_val = map_json["data"][r];
                    map_data[r] = new MapTile[row_val.Size()];

                    bool row_ok = this->load_row(row_val, r, thread_texture_sets[bucket]);
                    if (!row_ok) ok.store(false, std::memory_order_relaxed);
                })
            );
        }

        // Wait + propagate exceptions
        for (auto& f : futures) f.get();

        if (!ok.load()) {
            for (size_t r = 0; r < rows; ++r) {
                delete[] map_data[r];
            }
            delete[] map_data;
            return;
        }

        for (auto& s : thread_texture_sets) {
            tile_textures.insert(s.begin(), s.end());
        }
        tile_textures.erase("td:none");

    } else {
        for (size_t index = 0; index < rows; ++index) {
            cols = std::max(int(cols), int(map_json["data"][index].Size()));
            map_data[index] = new MapTile[map_json["data"][index].Size()];
            load_row(map_json["data"][index], index, tile_textures);
        }
        tile_textures.erase("td:none");
    }

    // Baking Atlas //
    std::string tile_texture_names[tile_textures.size()];
    int tile_texture_index = 0;
    for (auto texture = tile_textures.begin(); texture != tile_textures.end(); ++texture) {
        tile_texture_names[tile_texture_index] = *texture;
        tile_texture_index++;
    }
    const std::string atlas_tile_textures_name = "map:"+map_name+":atlas:tile_textures";
    bake_atlas(agent, atlas_tile_textures_name, tile_texture_names, tile_textures.size());
    agent->set_dimensions(cols, rows, -(16*(rows-1)), 0); // FIXME: tiles with a large height can be above y=0

    // Add entitys //
    for (size_t r = 0; r < rows; ++r) {
        const auto row_size = map_json["data"][r].Size();
        for (size_t c = 0; c < row_size; ++c) {
            MapTile& current_tile = map_data[r][c];
            const auto [surrounding_height_top, surrounding_height_bottom, surrounding_height_left, surrounding_height_right] = get_surrounding(r, c);
            int surrounding_height = std::min(
                std::min(
                    surrounding_height_top,
                    surrounding_height_bottom
                ),
                std::min(
                    surrounding_height_left,
                    surrounding_height_right
                )
            );

            std::string map_entity_name = "map:"+map_name+":tile:"+std::to_string(current_tile.x)+"x"+std::to_string(current_tile.y);
            for (int height_index = 0; height_index < current_tile.height; ++height_index) {
                if ((height_index != current_tile.height-1) && (height_index <= surrounding_height)) {
                    //LOG(LogLevel::Debug, "Skipping   : %dx%d height: %d", c, r, height_index);
                    continue;
                } else if ((current_tile.top_tile != "td:none") && (height_index == current_tile.height-1)) {
                    add_tile_enity(
                        agent,
                        map_entity_name+":top_tile",
                        current_tile.top_tile,
                        current_tile.x,
                        current_tile.y,
                        height_index
                    );
                } else {
                    add_tile_enity(
                        agent,
                        map_entity_name+":base",
                        current_tile.base,
                        current_tile.x,
                        current_tile.y,
                        height_index
                    );
                }
            }
            if ((current_tile.top_tile == "td:none") && (current_tile.top != "td:none")) {
                add_tile_enity(
                    agent,
                    map_entity_name+":top",
                    current_tile.top,
                    current_tile.x,
                    current_tile.y,
                    current_tile.height-1
                );
            }
        }
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
    LOG(LogLevel::Info, "Loaded Map \"%s\" in %f ms", map_name.c_str(), elapsed_ms);

    //for (size_t col_index = 0; col_index < cols; ++col_index) {LOG(LogLevel::Debug, "Tile %s at %dx%d", map_data[0][col_index].top_tile.c_str(), map_data[0][col_index].x, map_data[0][col_index].y);}
}

Map::~Map() {
    for (size_t index = 0; index < rows; ++index) {
        delete[] map_data[index];
    }
    delete[] map_data;
}



bool Map::load_row(const rapidjson::GenericValue<rapidjson::UTF8<>>& row_json, const size_t row_index, std::set<std::string>& tile_textures) {
    for (size_t col_index = 0; col_index < row_json.Size(); ++col_index) {
        const auto& tile_json = (row_json[col_index].IsObject()) ? row_json[col_index] : row_json[col_index][0];

        // Checking for errors //
        if ((!row_json[col_index].IsArray()) && (!row_json[col_index].IsObject())) {
            LOG(LogLevel::Error, "Could not load row %d of map %s: tile %d;%d seems be neither an object nor an array", int(row_index), map_name.c_str(), int(col_index), int(row_index));
            return false;
        }
        if (!(tile_json.HasMember("base"))) {
            LOG(LogLevel::Error, "Could not load row %d of map %s: tile %d;%d seems to be missing a \"base\" object.", int(row_index), map_name.c_str(), int(col_index), int(row_index));
            return false;
        }

        // Loading tile //
        MapTile current_tile = MapTile(tile_json["base"].GetString(), int(col_index), int(row_index));
        current_tile.top_tile = (tile_json.HasMember("top_tile")) ? tile_json["top_tile"].GetString() : "td:none";
        current_tile.top = (tile_json.HasMember("top"))           ? tile_json["top"].GetString()      : "td:none";
        current_tile.height = (tile_json.HasMember("height"))     ? tile_json["height"].GetInt()      : 1;
        current_tile.size = (tile_json.HasMember("size"))         ? tile_json["size"].GetInt()        : 1;
        // TODO: loading resoures

        map_data[row_index][col_index] = current_tile;

        tile_textures.insert(current_tile.base);
        tile_textures.insert(current_tile.top_tile);
        tile_textures.insert(current_tile.top);
    
        /*
        std::string map_entity_name = "map:"+map_name+":tile:"+std::to_string(current_tile.x)+"x"+std::to_string(current_tile.y);
        for (int height_index = 0; height_index < current_tile.height; ++height_index) {
            if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Entity %s: height_index = %d; y = %d - 16*%d + 11*%d = %d", map_entity_name.c_str(), height_index, current_tile.y, height_index, current_tile.x, current_tile.y-(16*height_index)+(11*current_tile.x));
            if ((current_tile.top_tile != "td:none") && (height_index == current_tile.height-1)) {
                agent->add_entity(map_entity_name+":top_tile", current_tile.top_tile, "default", 16*(current_tile.x-current_tile.y), 11*(current_tile.y+current_tile.x)-(16*height_index), current_tile.size*4);
            } else {
                agent->add_entity(map_entity_name+":base", current_tile.base, "default", 16*(current_tile.x-current_tile.y), 11*(current_tile.y+current_tile.x)-(16*height_index), current_tile.size*4);
            }
        }
        
        // Loading building // // TODO: load any actor
        if (row_json[col_index].IsArray() && (row_json[col_index].Size() > 1)) {
            if (row_json[col_index].Size() > 2) {
                LOG(LogLevel::Warning, "Tile %d;%d of map %s has more than 1 building! Only the first building will be loaded.", int(col_index), int(row_index), map_name.c_str());
            }
            if (!row_json[col_index][1].IsObject()) {
                LOG(LogLevel::Error, "Could not load building on tile %d;%d of map %s: building has to be a json object.", int(col_index), int(row_index), map_name.c_str());
            } else {
                if (!row_json[col_index][1].HasMember("type")) {
                    LOG(LogLevel::Error, "Could not load building on tile %d;%d of map %s: no \"type\" object found.", int(col_index), int(row_index), map_name.c_str());
                } else {
                    if (!(std::string(row_json[col_index][1]["type"].GetString()) == "td:Building")) {
                        LOG(LogLevel::Error, "Could not load building on tile %d;%d of map %s: type is not of \"td:Building\" but is \"%s\".", int(col_index), int(row_index), map_name.c_str(), row_json[col_index][1]["type"].GetString());
                    } else {
                        LOG(LogLevel::Warning, "Building / Actor loading not implemented yet!");
                    }
                }
            }
        }
        */
    }
    return true;
}
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <filesystem>
#include "rapidjson/rapidjson.h"
#include "BS_thread_pool.hpp"
#include <atomic>
#include <cmath>

#include "utils/json.hpp"
#include "utils/logger.hpp"

#include "settings/locations.hpp"
#include "settings/render.hpp"
#include "settings/main.hpp"
#include "settings/debug.hpp"

#include "player.hpp"
#include "main.hpp"

#include "map.hpp"

#include "agents.hpp" // TMP

bool Map::load_chunk_data(rapidjson::Value& chunk_json, const size_t& chunk_x, const size_t& chunk_y, std::set<std::string>& tile_textures, std::vector<ActorConstructor>& actor_constructors) {
    map_data[chunk_y][chunk_x] = MapChunk();
    MapChunk* chunk = get_chunk(chunk_x, chunk_y);
    for (int row_index = 0; row_index < chunk_size; ++row_index) {
        for (int col_index = 0; col_index < chunk_size; ++col_index) {
            chunk->data[row_index][col_index] = MapTile{
                "td:none",
                "td:none",
                0,
                (int(chunk_x)*chunk_size)+col_index,
                (int(chunk_y)*chunk_size)+row_index,
                nullptr,
                std::vector<RenderAgentEntity*>(),
                std::vector<ActorInstance*>(1, nullptr),
                std::vector<MapTileEffectInstance>()
            };
        }
    }

    if (chunk_json.Size() == 0) {
        LOG(LogLevel::Warning, "Chunk (%d, %d) is empty.", chunk_x, chunk_y);
        return true;
    }
    if (chunk_json.Size() > chunk_size) {
        LOG(LogLevel::Warning, "Chunk (%d, %d): count of rows > chunk size (%d).", chunk_x, chunk_y, chunk_size);
    }

    for (int row_index = 0; row_index < chunk_size; ++row_index) {
        if (row_index >= int(chunk_json.Size()))
            break;
        if (chunk_json[row_index].Size() > chunk_size) {
            LOG(LogLevel::Warning, "Chunk (%d, %d): row %d: count of colums > chunk size (%d).", chunk_x, chunk_y, row_index, chunk_size);
        }

        for (int col_index = 0; col_index < chunk_size; ++col_index) {
            if (col_index >= int(chunk_json[row_index].Size()))
                break;

            rapidjson::Value& tile_json = chunk_json[row_index][col_index];
            if ((!tile_json.IsArray()) && (!tile_json.IsObject())) {
                LOG(LogLevel::Error, "Chunk (%d, %d): tile (%d, %d) seems be neither an object nor an array.", int(chunk_x), int(chunk_y), col_index, row_index);
                continue;
            }

            // Loading tile //
            MapTile& tile = chunk->data[row_index][col_index];
            if (tile_json.HasMember("base")) {
                if (tile_json["base"].IsString())
                    tile.base = tile_json["base"].GetString();
                else
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): attibute \"base\" should be of type string.", int(chunk_x), int(chunk_y), col_index, row_index);
            }
            if (tile_json.HasMember("top")) {
                if (tile_json["top"].IsString())
                    tile.top = tile_json["top"].GetString();
                else
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): attibute \"top\" should be of type string.", int(chunk_x), int(chunk_y), col_index, row_index);
            }
            if (tile_json.HasMember("height")) {
                if (tile_json["height"].IsInt())
                    tile.height = tile_json["height"].GetInt();
                else
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): attibute \"height\" should be of type int.", int(chunk_x), int(chunk_y), col_index, row_index);
            }

            // TODO: effects

            // Actors //
            if (tile_json.HasMember("actors")) {
                if (tile_json["actors"].IsArray()) {
                    ActorConstructor constructor = ActorConstructor{"td:none", -1, (int(chunk_x)*chunk_size)+col_index, (int(chunk_y)*chunk_size)+row_index};
                    for (rapidjson::SizeType actor_index = 0; actor_index < tile_json["actors"].Size(); ++actor_index) {
                        rapidjson::Value& actor_json = tile_json["actors"][actor_index];
                        if (actor_json.HasMember("id")) {
                            if (actor_json["id"].IsString()) {
                                constructor.id = actor_json["id"].GetString();
                                if (actor_json.HasMember("owner")) {
                                    if (actor_json["owner"].IsInt()) {
                                        constructor.owner = actor_json["owner"].GetInt();
                                        LOG(LogLevel::Debug, "New actor \"%s\" at (%d, %d): owner: %d", constructor.id.c_str(), constructor.col, constructor.row, constructor.owner);
                                        actor_constructors.push_back(constructor);
                                    } else {
                                        LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): actor %d: attibute \"owner\" should be of type int.", int(chunk_x), int(chunk_y), col_index, row_index, int(actor_index));
                                    }
                                } else {
                                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): actor %d is missing an \"owner\" attribute.", int(chunk_x), int(chunk_y), col_index, row_index, int(actor_index));
                                }
                            } else {
                                LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): actor %d: attibute \"id\" should be of type string.", int(chunk_x), int(chunk_y), col_index, row_index, int(actor_index));
                            }
                        } else {
                            LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): actor %d is missing an \"id\" attribute.", int(chunk_x), int(chunk_y), col_index, row_index, int(actor_index));
                        }
                    }
                } else
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): attibute \"actors\" should be of type array.", int(chunk_x), int(chunk_y), col_index, row_index);
            }

            tile_textures.insert(tile.base);
            tile_textures.insert(tile.top);
        }
    }

    tile_textures.erase("td:none");

    return true;
};

bool Map::load_chunk_entities(const int& chunk_x, const int& chunk_y, std::vector<std::tuple<int, int, RenderAgentEntity>>& entities) {
    MapChunk* chunk = get_chunk(chunk_x, chunk_y);
    if (!chunk){
        LOG(LogLevel::Warning, "Could not load chunk entities: chunk (%d, %d) not loaded/does not exist.", chunk_x, chunk_y);
        return false;
    }

    std::string sprite_id;
    RenderAgentEntity entity;
    int x, y;
    int actual_height_index;
    for (int row_index = 0; row_index < chunk_size; ++row_index) {
        for (int col_index = 0; col_index < chunk_size; ++col_index) {
            MapTile& current_tile = chunk->data[row_index][col_index];

            x = (chunk_x*chunk_size)+col_index;
            y = (chunk_y*chunk_size)+row_index;
            for (int height_index = 0; height_index <= current_tile.height; ++height_index) {
                if (height_index == current_tile.height) {
                    sprite_id = current_tile.top;
                } else {
                    sprite_id = current_tile.base;
                }
                if (sprite_id == "td:none")
                    continue;

                entity = RenderAgentEntity();
                entity.sprite = agent->get_sprite(sprite_id);
                if (!entity.sprite) {
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): could not add entity for height index %d: sprite \"%s\" does not exist.", chunk_x, chunk_y, col_index, row_index, height_index, sprite_id.c_str());
                    entity.sprite = agent->get_sprite("td:missing_tile");
                }
                //               |-------------------rows before--------------------| |----------this row----------|             |-this tile-|
                entity.layer = (((((chunk_y*chunk_size)+row_index)*(cols*chunk_size))+(chunk_x*chunk_size)+col_index)*chunk_size)+height_index;

                actual_height_index = (height_index == current_tile.height)
                    ? height_index-1
                    : height_index;

                entity.x = (16*(x-y));
                entity.y = (11*(y+x)-(16*actual_height_index));
                if (entity.sprite)
                    entity.animation = SDL_rand(entity.sprite->animations.size());
                entity.rotation = 0;
                entity.movable = false;

                entities.push_back(std::tuple<int, int, RenderAgentEntity>(x, y, entity));
            }
        }
    }

    return true;
};



std::tuple<int, int, int, int> Map::get_surrounding(const int& col, const int& row) {
    MapTile* tile_up = get_tile(col, row-1, true);
    MapTile* tile_down = get_tile(col, row+1, true);
    MapTile* tile_left = get_tile(col-1, row, true);
    MapTile* tile_right = get_tile(col+1, row, true);
    return std::tuple<int, int, int, int> {
        (tile_up)
            ? tile_up->height
            : -1,
        (tile_down)
            ? tile_down->height
            : -1,
        (tile_left)
            ? tile_left->height
            : -1,
        (tile_right)
            ? tile_right->height
            : -1
    };
};

MapChunk* Map::get_chunk(const int& chunk_x, const int& chunk_y, const bool& suppress_logs) {
    if (chunk_y >= int(map_data.size())) {
        if (!suppress_logs)
            LOG(LogLevel::Warning, "Requested non-existend chunk at (%d, %d)", chunk_x, chunk_y);
        return nullptr;
    }
    if (chunk_x >= int(map_data[chunk_y].size())) {
        if (!suppress_logs)
            LOG(LogLevel::Warning, "Requested non-existend chunk at (%d, %d)", chunk_x, chunk_y);
        return nullptr;
    }

    return &map_data[chunk_y][chunk_x];
};

MapTile* Map::get_tile(const int& col, const int& row, const bool& suppress_logs) {
    int chunk_y = std::floor(row/chunk_size);
    int chunk_x = std::floor(col/chunk_size);
    int tile_y = row % chunk_size;
    int tile_x = col % chunk_size;

    if (chunk_y >= int(map_data.size())) {
        if (!suppress_logs)
            LOG(LogLevel::Warning, "Requested non-existend chunk at (%d, %d)", chunk_x, chunk_y);
        return nullptr;
    }
    if (chunk_x >= int(map_data[chunk_y].size())) {
        if (!suppress_logs)
            LOG(LogLevel::Warning, "Requested non-existend chunk at (%d, %d)", chunk_x, chunk_y);
        return nullptr;
    }

    return &map_data[chunk_y][chunk_x].data[tile_y][tile_x];
};



bool Map::load_chunks(const std::vector<std::tuple<int, int>>& chunk_positions) {
    const int chunk_count = chunk_positions.size();
    rapidjson::Document map_json = open_json(replace_locations(path));
    if (!map_json.HasMember("data")) {
        LOG(LogLevel::Error, "Could not load map %s: json does not contain a \"data\" object.", name.c_str());
        return false;
    }
    if (map_json["data"].Size() == 0) {
        LOG(LogLevel::Error, "Could not load map %s: \"data\" object can not be empty.", name.c_str());
        return false;
    }

    // Load //
    LOG(LogLevel::Debug, "Loading tiles..");
    std::set<std::string> tile_textures;
    std::vector<std::vector<ActorConstructor>> actor_constructors(chunk_count);
    if (SETTINGS["multithreading"].get<bool>()) {
        const int configured_threads = SETTINGS["num_threads"].get<int>();
        const size_t num_threads = (configured_threads == -1)
            ? std::max(1u, std::thread::hardware_concurrency())
            : (size_t)std::max(1, configured_threads);

        BS::thread_pool pool(num_threads);

        std::vector<std::set<std::string>> thread_texture_sets(chunk_count);
        std::atomic<bool> ok{true};
        std::vector<std::future<void>> futures;
        futures.reserve(chunk_count);

        int i = 0;
        for (auto& [chunk_x, chunk_y] : chunk_positions) {
            futures.emplace_back(
                pool.submit_task([this, i, &map_json, chunk_x, chunk_y, &ok, &thread_texture_sets, &actor_constructors]() {
                    if (!ok.load(std::memory_order_relaxed)) return;

                    bool chunk_ok = this->load_chunk_data(map_json["data"][chunk_y][chunk_x], chunk_x, chunk_y, thread_texture_sets[i], actor_constructors[i]);
                    if (!chunk_ok) ok.store(false, std::memory_order_relaxed);
                })
            );
            i++;
        }

        for (auto& future : futures) future.get();

        if (!ok.load()) {
            for (int chunk_y = 0; chunk_y < int(rows); ++chunk_y) {
                map_data[chunk_y].clear();
            }
            return false;
        }

        for (auto& set : thread_texture_sets) {
            tile_textures.insert(set.begin(), set.end());
        }
    } else {
        int i = 0;
        for (auto& [chunk_x, chunk_y] : chunk_positions) {
            if (!load_chunk_data(map_json["data"][chunk_y][chunk_x], chunk_x, chunk_y, tile_textures, actor_constructors[i])) {
                LOG(LogLevel::Warning, "Could not load chunk (%d, %d)", chunk_x, chunk_y);
            }
            i++;
        }
    }

    LOG(LogLevel::Debug, "Baking atlas..");
    std::vector<std::string> vector_tile_textures;
    for (auto& texture : tile_textures) {
        vector_tile_textures.push_back(texture);
    }
    if (!agent->get_texture(atlas_name, true)) {
        vector_tile_textures.push_back("td:tile_missing");
        vector_tile_textures.push_back("td:top_missing");
        vector_tile_textures.push_back("td:missing");
        if (!agent->bake_atlas(atlas_name, vector_tile_textures))
            LOG(LogLevel::Error, "Could not bake map atlas \"%s\"", atlas_name.c_str());
    } else {
        for (auto& texture_id : vector_tile_textures) {
            if (!agent->get_sprite(texture_id, false)) {
                if (!agent->add_to_atlas(atlas_name, texture_id))
                    LOG(LogLevel::Error, "Could not add texture \"%s\" to map atlas.", texture_id.c_str());
            }
        }
    }

    // Entities //
    LOG(LogLevel::Debug, "Making Entitys..");
    std::vector<std::vector<std::tuple<int, int, RenderAgentEntity>>> entity_cache(chunk_count);
    if (SETTINGS["multithreading"].get<bool>()) {
        const int configured_threads = SETTINGS["num_threads"].get<int>();
        const size_t num_threads = (configured_threads == -1)
            ? std::max(1u, std::thread::hardware_concurrency())
            : (size_t)std::max(1, configured_threads);

        BS::thread_pool pool(num_threads);
        std::atomic<bool> ok{true};
        std::vector<std::future<void>> futures;
        futures.reserve(chunk_count);

        int i = 0;
        for (auto& [chunk_x, chunk_y] : chunk_positions) {
            futures.emplace_back(
                pool.submit_task([this, i, chunk_x, chunk_y, &ok, &entity_cache]() {
                    if (!ok.load(std::memory_order_relaxed)) return;

                    bool chunk_ok = this->load_chunk_entities(chunk_x, chunk_y, entity_cache[i]);
                    if (!chunk_ok) ok.store(false, std::memory_order_relaxed);
                })
            );
            i++;
        }

        for (auto& future : futures) future.get();

        if (!ok.load()) {
            for (size_t i = 0; i < entity_cache.size(); ++i) {
                entity_cache[i].clear();
            }
            return false;
        }
    } else {
        int i = 0;
        for (auto& [chunk_x, chunk_y] : chunk_positions) {
            if (!load_chunk_entities(chunk_x, chunk_y, entity_cache[i])) {
                LOG(LogLevel::Warning, "Could not load chunk (%d, %d)", chunk_x, chunk_y);
            }
            i++;
        }
    }

    // Dimensions //
    LOG(LogLevel::Debug, "Setting dimesions of quadtrees..");
    int quadtree_tx, quadtree_ty, quadtree_w, quadtree_h;
    agent->get_dimensions(quadtree_tx, quadtree_ty, quadtree_w, quadtree_h);
    int quadtree_bx = quadtree_w+quadtree_tx;
    int quadtree_by = quadtree_h+quadtree_ty;
    for (auto& chunk_entities : entity_cache) {
        for (auto& [entity_col, entity_row, entity] : chunk_entities) {
            quadtree_tx = std::min(quadtree_tx, entity.x);
            quadtree_ty = std::min(quadtree_ty, entity.y);
            quadtree_bx = std::max(quadtree_bx, entity.x+entity.sprite->animations[0].texture_rects[0].w);
            quadtree_by = std::max(quadtree_by, entity.y+entity.sprite->animations[0].texture_rects[0].h);
        }
    }
    agent->set_dimensions(quadtree_tx, quadtree_ty, quadtree_bx-quadtree_tx, quadtree_by-quadtree_ty);
    ACTORS_RENDER_AGENT->set_dimensions(quadtree_tx, quadtree_ty-100, quadtree_bx-quadtree_tx, quadtree_by-quadtree_ty+100); // Do this in the Map constructor

    LOG(LogLevel::Debug, "Spawning actors..");
    for (auto& chunk_actors : actor_constructors) {
        for (ActorConstructor& constructor : chunk_actors) {
            if ((constructor.owner < 0) || (constructor.owner >= PLAYER_COUNT))
                continue;
            map_data[std::floor(constructor.row/chunk_size)][std::floor(constructor.col/chunk_size)].data[constructor.row%chunk_size][constructor.col%chunk_size].actors[0] = PLAYERS[constructor.owner].actor_handler->spawn_actor(constructor.id, constructor.col, constructor.row);
        }
    }


    // Inserting Entities //
    LOG(LogLevel::Debug, "Adding Entitys..");
    for (std::vector<std::tuple<int, int, RenderAgentEntity>>& chunk_entities : entity_cache) {
        for (auto& [entity_col, entity_row, entity] : chunk_entities) {
            if (&entity == &std::get<2>(chunk_entities.back()))
                map_data[std::floor(entity_row/chunk_size)][std::floor(entity_col/chunk_size)].data[entity_row%chunk_size][entity_col%chunk_size].top_entity = agent->insert_entity(entity, false);
            else
                map_data[std::floor(entity_row/chunk_size)][std::floor(entity_col/chunk_size)].data[entity_row%chunk_size][entity_col%chunk_size].base_entities.push_back(agent->insert_entity(entity, false));
        }
    }
    LOG(LogLevel::Debug, "Subdividing quadtree..");
    agent->trigger_subdivision();

    return true;
};



Map::Map(RenderAgent* agent, const std::filesystem::path& map_path) {
    this->agent = agent;
    this->path = map_path;
    LOG(LogLevel::Debug, "Loading json data");
    rapidjson::Document map_json = open_json(replace_locations(path));
    LOG(LogLevel::Debug, "Done loading json data");
    if (!map_json.IsObject()) {
        LOG(LogLevel::Error, "Could not load map \"%s\": root is not an object.", path.u8string().c_str());
        return;
    }
    if (!map_json.HasMember("name")) {
        LOG(LogLevel::Error, "Could not load map \"%s\": json does not contain a \"name\" object.", path.u8string().c_str());
        return;
    }
    name = std::string(map_json["name"].GetString());

    if (map_json.HasMember("description")) {
        description = std::string(map_json["description"].GetString());
    } else {
        description = "No discription given";
    }

    if (!map_json.HasMember("data")) {
        LOG(LogLevel::Error, "Could not load map \"%s\": json does not contain a \"data\" object.", name.c_str());
        return;
    }
    if (map_json["data"].Size() == 0) {
        LOG(LogLevel::Error, "Could not load map \"%s\": \"data\" object can not be empty.", name.c_str());
        return;
    }

    // Allocate //
    LOG(LogLevel::Debug, "Allocating");
    rows = map_json["data"].Size();
    cols = map_json["data"][0].Size();
    map_data = std::vector<std::vector<MapChunk>>(rows, std::vector<MapChunk>(cols));
    atlas_name = "map:"+name+":atlas:tile_textures";
};

Map::~Map() {};
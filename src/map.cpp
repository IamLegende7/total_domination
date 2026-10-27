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
#include <memory>
#include <iterator>
#include <algorithm>

#include "utils/json.hpp"
#include "utils/logger.hpp"
#include "utils/iff.hpp"

#include "settings/locations.hpp"
#include "settings/render.hpp"
#include "settings/main.hpp"
#include "settings/debug.hpp"

#include "player.hpp"
#include "main.hpp"

#include "map.hpp"

#include "agents.hpp" // TMP

bool Map::load_chunk_data_json(rapidjson::Value& chunk_json, const size_t& chunk_x, const size_t& chunk_y, std::set<std::string>& tile_textures) {
    MapChunk* chunk = get_chunk(chunk_x, chunk_y);

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
                if (tile_json["base"].IsString()) {
                    tile.base = tile_json["base"].GetString();
                    if (tile.base == "")
                        tile.base = "td:none";
                } else
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): attibute \"base\" should be of type string.", int(chunk_x), int(chunk_y), col_index, row_index);
            }
            if (tile_json.HasMember("top")) {
                if (tile_json["top"].IsString()) {
                    tile.top = tile_json["top"].GetString();
                    if (tile.top == "")
                        tile.top = "td:none";
                } else
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): attibute \"top\" should be of type string.", int(chunk_x), int(chunk_y), col_index, row_index);
            }
            if (tile_json.HasMember("height")) {
                if (tile_json["height"].IsInt())
                    tile.height = tile_json["height"].GetInt();
                else
                    LOG(LogLevel::Warning, "Chunk (%d, %d): tile (%d, %d): attibute \"height\" should be of type int.", int(chunk_x), int(chunk_y), col_index, row_index);
            }

            if ((tile.base != "") && (tile.base != "td:none"))
                tile_textures.insert(tile.base);
            if ((tile.top != "") && (tile.top != "td:none"))
                tile_textures.insert(tile.top);

            if (tile.actors.size() != 0)
                LOG(LogLevel::Debug, "Chunk (%d, %d): tile (%d, %d): tile has actors.", int(chunk_x), int(chunk_y), col_index, row_index);
            for (ActorInstance* actor : tile.actors) {
                actor->y -= 16*tile.height;
                actor->entity->y -= 16*tile.height;
            }
        }
    }

    return true;
};

bool Map::load_actors_json(rapidjson::Value& actors_json) {
    if (actors_json.IsArray()) {
        std::string id;
        Uint64 row, col;
        Uint8 owner;
        Uint32 chunk_x, chunk_y, tile_x, tile_y;
        for (rapidjson::SizeType actor_index = 0; actor_index < actors_json.Size(); ++actor_index) {
            rapidjson::Value& actor_json = actors_json[actor_index];
            if (!actor_json.HasMember("id")) {
                LOG(LogLevel::Warning, "Could not load actor %d: missing an \"id\" attribute.", int(actor_index));
                continue;
            }
            if (!actor_json["id"].IsString()) {
                LOG(LogLevel::Warning, "Could not load actor %d: attibute \"id\" should be of type string.", int(actor_index));
                continue;
            }
            if (!actor_json.HasMember("owner")) {
                LOG(LogLevel::Warning, "Could not load actor %d: missing an \"owner\" attribute.", int(actor_index));
                continue;
            }
            if (!actor_json["owner"].IsInt()) {
                LOG(LogLevel::Warning, "Could not load actor %d: attibute \"owner\" should be of type int.", int(actor_index));
                continue;
            }
            if (!actor_json.HasMember("x")) {
                LOG(LogLevel::Warning, "Could not load actor %d: missing an \"x\" attribute.", int(actor_index));
                continue;
            }
            if (!actor_json["x"].IsInt()) {
                LOG(LogLevel::Warning, "Could not load actor %d: attibute \"x\" should be of type int.", int(actor_index));
                continue;
            }
            if (!actor_json.HasMember("y")) {
                LOG(LogLevel::Warning, "Could not load actor %d: missing an \"y\" attribute.", int(actor_index));
                continue;
            }
            if (!actor_json["y"].IsInt()) {
                LOG(LogLevel::Warning, "Could not load actor %d: attibute \"y\" should be of type int.", int(actor_index));
                continue;
            }

            id = actor_json["id"].GetString();
            owner = actor_json["owner"].GetInt();
            col = actor_json["x"].GetInt();
            row = actor_json["y"].GetInt();
            if (owner >= PLAYER_COUNT)
                continue;
            chunk_y = std::floor(row/chunk_size);
            chunk_x = std::floor(col/chunk_size);
            tile_y = row%chunk_size;
            tile_x = col%chunk_size;
            if (chunk_x >= cols) {
                LOG(LogLevel::Warning, "Could not load actor: chunk_x out of bounds: %d >= %d", chunk_x, cols);
                continue;
            }
            if (chunk_y >= rows) {
                LOG(LogLevel::Warning, "Could not load actor: chunk_y out of bounds: %d >= %d", chunk_y, rows);
                continue;
            }
            LOG(LogLevel::Debug, "New actor \"%s\" in chunk (%d, %d): owner: %d", id.c_str(), chunk_x, chunk_y, owner);
            PLAYERS[owner].actor_handler->spawn_actor(id, col, row);
        }
        return true;
    } else {
        LOG(LogLevel::Error, "Could not load actors: attibute \"actors\" should be of type array.");
        return false;
    }
};

bool Map::load_chunk_data_iff(IFF_RawChunk* chunk_data, const size_t& chunk_x, const size_t& chunk_y) {
    MapChunk* chunk = get_chunk(chunk_x, chunk_y);
    
    if (chunk_data->type != "CHNK") {
        LOG(LogLevel::Warning, "Could not load chunk data: chunk has wrong type \"%s\", expected \"CHNK\".", chunk_data->type.c_str());
        return false;
    }
    constexpr Sint32 expected_chunk_size = static_cast<Sint32>(chunk_size*chunk_size*3);
    if (chunk_data->size != expected_chunk_size) {
        LOG(LogLevel::Warning, "Could not load chunk data: chunk is too small: is %d, expected %d.", chunk_data->size, expected_chunk_size);
        return false;
    }

    chunk_data->offset = 0;
    for (std::size_t y = 0; y < chunk_size; ++y) {
        for (std::size_t x = 0; x < chunk_size; ++x) {
            MapTile& tile = chunk->data[y][x];
            Sint32 top_index = chunk_data->load_uint8();
            Sint32 base_index = chunk_data->load_uint8();
            tile.height = chunk_data->load_uint8();
            if (base_index >= static_cast<Sint32>(tile_ids.size())) {
                LOG(LogLevel::Warning, "Could not load tile base: index out of bounds: %d >= %d", base_index, tile_ids.size());
                tile.base = "td:none";
            } else {
                tile.base = tile_ids[static_cast<std::size_t>(base_index)];
            }
            if (top_index >= static_cast<Sint32>(tile_ids.size())) {
                LOG(LogLevel::Warning, "Could not load tile base: index out of bounds: %d >= %d", top_index, tile_ids.size());
                tile.top = "td:none";
            } else {
                tile.top = tile_ids[static_cast<std::size_t>(top_index)];
            }

            for (ActorInstance* actor : tile.actors) {
                actor->y -= 16*tile.height;
                actor->entity->y -= 16*tile.height;
            }
            //LOG(LogLevel::Debug, "Tile (%d, %d): %s, %s, %d", (chunk_x*chunk_size)+x, (chunk_y*chunk_size)+y, tile.top.c_str(), tile.base.c_str(), tile.height);
        }
    }

    return true;
};

bool Map::load_actors_iff(IFF_FormChunk* actor_list) {
    LOG(LogLevel::Debug, "Spawning actors..");
    if (actor_list->form_type != "ACTS") {
        LOG(LogLevel::Critical, "Could not load actor list: chunk has wrong type \"%s\", expected \"ACTS\".", actor_list->type.c_str());
        return false;
    }
    constexpr Sint32 expected_actor_size = 20;
    Sint8 id_index;
    Uint32 row, col;
    Uint8 owner;
    std::string id;
    Uint32 chunk_x, chunk_y, tile_x, tile_y;
    while (actor_list->offset < actor_list->size-4) {
        LOG(LogLevel::Debug, "actor_list->offset (%d) < actor_list->size (%d)", actor_list->offset, actor_list->size-4);
        const Sint32 old_offset = actor_list->offset;
        std::unique_ptr<IFF_Chunk> ptr = actor_list->get_child();
        IFF_RawChunk* actor_chunk = dynamic_cast<IFF_RawChunk*>(ptr.get());
        if (!actor_chunk) {
            LOG(LogLevel::Warning, "Could not load actor: chunk is not a raw chunk.");
            continue;
        }
        if (actor_list->offset <= old_offset) {
            LOG(LogLevel::Error, "Could not load actors: failed to read child chunk at %d", old_offset);
            break;
        }
        
        if (actor_chunk->type != "ACTR") {
            LOG(LogLevel::Warning, "Could not load actor: chunk has wrong type \"%s\", expected \"ACTR\".", actor_chunk->type.c_str());
            continue;
        }
        if (actor_chunk->size != expected_actor_size) {
            LOG(LogLevel::Warning, "Could not load actor: chunk has wrong size: is %d, expected %d.", actor_chunk->size, expected_actor_size);
            continue;
        }

        id_index = actor_chunk->load_uint8();
        col = actor_chunk->load_uint32();
        row = actor_chunk->load_uint32();
        owner = actor_chunk->load_uint8();
        // TODO: implement overwriting default values

        if (owner >= PLAYER_COUNT) {
            LOG(LogLevel::Warning, "Could not load actor: owner not valid: %d", owner);
            continue;
        }
        if (id_index >= static_cast<Sint8>(actor_ids.size())) {
            LOG(LogLevel::Warning, "Could not load actor: index out of bounds: %d >= %d", id_index, actor_ids.size());
            continue;
        } else {
            id = actor_ids[static_cast<std::size_t>(id_index)];
        }

        chunk_y = std::floor(row/chunk_size);
        chunk_x = std::floor(col/chunk_size);
        tile_y = row%chunk_size;
        tile_x = col%chunk_size;
        if (chunk_x >= cols) {
            LOG(LogLevel::Warning, "Could not load actor: chunk_x out of bounds: %d >= %d", chunk_x, cols);
            continue;
        }
        if (chunk_y >= rows) {
            LOG(LogLevel::Warning, "Could not load actor: chunk_y out of bounds: %d >= %d", chunk_y, rows);
            continue;
        }
        LOG(LogLevel::Debug, "New actor \"%s\" at (%d, %d): owner: %d", id.c_str(), (chunk_x*chunk_size)+tile_x, (chunk_y*chunk_size)+tile_y, owner);
        PLAYERS[owner].actor_handler->spawn_actor(id, col, row);
    }
    return true;
};

bool Map::load_chunk_entities(const int& chunk_x, const int& chunk_y, std::vector<std::tuple<Sint64, Sint64, RenderAgentEntity>>& entities) {
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

void Map::allocate() {
    LOG(LogLevel::Debug, "Allocating %d rows, %d cols..", rows, cols);
    map_data = std::vector<std::vector<MapChunk>>(rows, std::vector<MapChunk>(cols));
    for (Uint32 chunk_y = 0; chunk_y < rows; ++chunk_y) {
        for (Uint32 chunk_x = 0; chunk_x < cols; ++chunk_x) {
            for (int row_index = 0; row_index < chunk_size; ++row_index) {
                for (int col_index = 0; col_index < chunk_size; ++col_index) {
                    map_data[chunk_y][chunk_x].data[row_index][col_index] = MapTile{
                        "td:none",
                        "td:none",
                        0,
                        (int(chunk_x)*chunk_size)+col_index,
                        (int(chunk_y)*chunk_size)+row_index,
                        nullptr,
                        std::vector<RenderAgentEntity*>(),
                        std::vector<ActorInstance*>(),
                        std::vector<MapTileEffectInstance>()
                    };
                    //LOG(LogLevel::Debug, "Inititalized tile (%d, %d)", map_data[chunk_y][chunk_x].data[row_index][col_index].x, map_data[chunk_y][chunk_x].data[row_index][col_index].y);
                }
            }
        }
    }
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



bool Map::load_actors() {
    LOG(LogLevel::Debug, "Loading actors...");
    if (iff) {
        IFF_File file(path);
        std::unique_ptr<IFF_FormChunk> actors = file.find_form_chunk("ACTS", 0);
        if (!actors) {
            LOG(LogLevel::Error, "Could not load actors: could not find FORM chunk of form type \"ACTS\".");
            return false;
        }
        if (!load_actors_iff(actors.get())) {
            LOG(LogLevel::Critical, "Could not load actors from IFF file.");
            return false;
        }
    } else {
        rapidjson::Document map_json = open_json(path);
        if (!map_json.IsObject()) {
            LOG(LogLevel::Error, "Could not load map \"%s\": root is not an object.", path.u8string().c_str());
            return false;
        }
        if (!map_json.HasMember("actors")) {
            LOG(LogLevel::Error, "Could not load map \"%s\": json does not contain a \"actors\" object.", name.c_str());
            return false;
        }
        if (!load_actors_json(map_json["actors"])) {
            LOG(LogLevel::Critical, "Could not load map \"%s\": could not load actors.", name.c_str());
            return false;
        }
    }
    return true;
};

bool Map::load_chunks(const std::vector<std::tuple<Uint32, Uint32>>& chunk_positions) {
    const int chunk_count = chunk_positions.size();
    if (iff) {
        IFF_File file(path);
        std::unique_ptr<IFF_FormChunk> data = file.find_form_chunk("DATA", 0);
        if (!data) {
            LOG(LogLevel::Error, "Could not load chunks: DATA chunk is not a FORM chunk.");
            return false;
        }
        LOG(LogLevel::Debug, "Loading tiles..");
        if (SETTINGS["multithreading"].get<bool>()) {
            const int configured_threads = SETTINGS["num_threads"].get<int>();
            const size_t num_threads = (configured_threads == -1)
                ? std::max(1u, std::thread::hardware_concurrency())
                : (size_t)std::max(1, configured_threads);
            BS::thread_pool pool(num_threads);

            std::atomic<bool> ok{true};
            std::vector<std::future<void>> futures;
            futures.reserve(chunk_count);

            std::vector<std::unique_ptr<IFF_Chunk>> chunk_owners;
            chunk_owners.reserve(chunk_positions.size());
            for (auto& [chunk_x, chunk_y] : chunk_positions) {
                Sint32 chunk_offset = ((chunk_y*cols)+chunk_x)*776;
                std::unique_ptr<IFF_Chunk> ptr = data->get_child(chunk_offset);
                chunk_owners.push_back(std::move(ptr));
                IFF_RawChunk* data_chunk = dynamic_cast<IFF_RawChunk*>(chunk_owners.back().get());

                futures.emplace_back(
                    pool.submit_task([this, data_chunk, chunk_x, chunk_y, &ok]() {
                        if (!ok.load(std::memory_order_relaxed)) return;
                        bool chunk_ok = this->load_chunk_data_iff(data_chunk, chunk_x, chunk_y);
                        if (!chunk_ok) ok.store(false, std::memory_order_relaxed);
                    })
                );
            }

            for (auto& future : futures) future.get();

            if (!ok.load()) {
                LOG(LogLevel::Warning, "Failed to load chunks.");
                return false;
            }
        } else {
            Sint32 chunk_offset;
            for (auto& [chunk_x, chunk_y] : chunk_positions) {
                chunk_offset = ((chunk_y*cols)+chunk_x)*776;
                std::unique_ptr<IFF_Chunk> ptr = data->get_child(chunk_offset);
                IFF_RawChunk* data_chunk = dynamic_cast<IFF_RawChunk*>(ptr.get());
                if (!load_chunk_data_iff(data_chunk, chunk_x, chunk_y))
                    LOG(LogLevel::Warning, "Could not load chunk (%d, %d).", chunk_x, chunk_y);
            }
        }
    } else {
        rapidjson::Document map_json = open_json(path);

        LOG(LogLevel::Debug, "Loading tiles..");
        std::set<std::string> tile_textures;
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
                    pool.submit_task([this, i, &map_json, chunk_x, chunk_y, &ok, &thread_texture_sets]() {
                        if (!ok.load(std::memory_order_relaxed)) return;
                        bool chunk_ok = this->load_chunk_data_json(map_json["data"][chunk_y][chunk_x], chunk_x, chunk_y, thread_texture_sets[i]);
                        if (!chunk_ok) ok.store(false, std::memory_order_relaxed);
                    })
                );
                i++;
            }

            for (auto& future : futures) future.get();

            if (!ok.load()) {
                LOG(LogLevel::Warning, "Failed to load chunks.");
                return false;
            }

            for (auto& set : thread_texture_sets) {
                tile_textures.insert(set.begin(), set.end());
            }
        } else {
            for (auto& [chunk_x, chunk_y] : chunk_positions) {
                if (!load_chunk_data_json(map_json["data"][chunk_y][chunk_x], chunk_x, chunk_y, tile_textures))
                    LOG(LogLevel::Warning, "Could not load chunk (%d, %d).", chunk_x, chunk_y);
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
    }
    
    // Entities //
    LOG(LogLevel::Debug, "Making Entities..");
    std::vector<std::vector<std::tuple<Sint64, Sint64, RenderAgentEntity>>> entity_cache(chunk_count);
    if (SETTINGS["multithreading"].get<bool>()) {
        const int configured_threads = SETTINGS["num_threads"].get<int>();
        const size_t num_threads = (configured_threads == -1)
            ? std::max(1u, std::thread::hardware_concurrency())
            : (size_t)std::max(1, configured_threads);
        BS::thread_pool pool(num_threads);

        std::atomic<bool> ok{true};
        std::vector<std::future<void>> futures;
        futures.reserve(chunk_count);

        LOG(LogLevel::Debug, "Submitting tasks...");
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

        LOG(LogLevel::Debug, "Getting futures...");
        for (auto& future : futures) future.get();

        if (!ok.load()) {
            for (size_t i = 0; i < entity_cache.size(); ++i)
                entity_cache[i].clear();
            return false;
        }
    } else {
        int i = 0;
        for (auto& [chunk_x, chunk_y] : chunk_positions) {
            if (!load_chunk_entities(chunk_x, chunk_y, entity_cache[i]))
                LOG(LogLevel::Warning, "Could not load chunk entities at (%d, %d).", chunk_x, chunk_y);
            i++;
        }
    }

    // Dimensions //
    LOG(LogLevel::Debug, "Setting dimensions of quadtrees..");
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
    ACTORS_RENDER_AGENT->set_dimensions(quadtree_tx, quadtree_ty-100, quadtree_bx-quadtree_tx, quadtree_by-quadtree_ty+100); // Move this to another place

    // Inserting Entities //
    LOG(LogLevel::Debug, "Inserting Entities..");
    for (std::vector<std::tuple<Sint64, Sint64, RenderAgentEntity>>& chunk_entities : entity_cache) {
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

bool Map::save(const std::filesystem::path& path) {
    if ((path.extension() == ".json") || (path.extension() == ".jsonc")) {
        // TODO: add saving to a json file
    } else if (path.extension() == ".iff") { // FIXME: does not align the chunk size to an even number
        IFF_File file(path);
        std::unique_ptr<IFF_RawChunk> meta = std::make_unique<IFF_RawChunk>("META");
        std::unique_ptr<IFF_RawChunk> tile_id_references = std::make_unique<IFF_RawChunk>("TLID");
        std::unique_ptr<IFF_RawChunk> actor_id_references = std::make_unique<IFF_RawChunk>("ACID");
        std::unique_ptr<IFF_FormChunk> data = std::make_unique<IFF_FormChunk>("DATA");
        std::unique_ptr<IFF_FormChunk> actors = std::make_unique<IFF_FormChunk>("ACTS");

        meta->save_uint32(version);
        meta->save_nstr(author);
        meta->save_nstr(creation_date);
        meta->save_nstr(name);
        meta->save_nstr(description);
        meta->save_uint32(rows);
        meta->save_uint32(cols);

        std::vector<std::string> save_tile_ids;
        std::vector<std::string> save_actor_ids;
        Uint32 row = 0;
        Uint32 col = 0;
        for (auto& chunk_row : map_data) {
            col = 0;
            for (auto& map_chunk : chunk_row) {
                std::unique_ptr<IFF_RawChunk> chunk = std::make_unique<IFF_RawChunk>("CHNK");
                for (int y = 0; y < chunk_size; ++y) {
                    for (int x = 0; x < chunk_size; ++x) {
                        MapTile* tile = &map_chunk.data[y][x];
                        if (!tile)
                            continue;
                        if (std::find(save_tile_ids.begin(), save_tile_ids.end(), tile->top) == save_tile_ids.end()) {
                            LOG(LogLevel::Debug, "Saving id \"%s\"", tile->top.c_str());
                            save_tile_ids.push_back(tile->top);
                            tile_id_references->save_nstr(tile->top);
                        }
                        if (std::find(save_tile_ids.begin(), save_tile_ids.end(), tile->base) == save_tile_ids.end()) {
                            LOG(LogLevel::Debug, "Saving id \"%s\"", tile->base.c_str());
                            save_tile_ids.push_back(tile->base);
                            tile_id_references->save_nstr(tile->base);
                        }
                        auto it_top  = std::find(save_tile_ids.begin(), save_tile_ids.end(), tile->top);
                        auto it_base = std::find(save_tile_ids.begin(), save_tile_ids.end(), tile->base);
                        Uint8 top_index  = static_cast<Uint8>(std::distance(save_tile_ids.begin(), it_top));
                        Uint8 base_index = static_cast<Uint8>(std::distance(save_tile_ids.begin(), it_base));
                        chunk->save_uint8(top_index);
                        chunk->save_uint8(base_index);
                        chunk->save_uint8(static_cast<Uint8>(tile->height));
                        //LOG(LogLevel::Debug, "Saved Tile (%d, %d): %d (%s), %d (%s), %d", x, y, top_index, tile->top.c_str(), base_index, tile->base.c_str(), tile->height);

                        for (ActorInstance* actor : tile->actors) {
                            if (!actor)
                                continue;
                            if (!actor->parent)
                                continue;
                            LOG(LogLevel::Debug, "Saving Actor \"%s\"...", actor->parent->id.c_str());
                            std::unique_ptr<IFF_RawChunk> actor_chunk = std::make_unique<IFF_RawChunk>("ACTR");
                            if (std::find(save_actor_ids.begin(), save_actor_ids.end(), actor->parent->id) == save_actor_ids.end()) {
                                LOG(LogLevel::Debug, "Saving id \"%s\"", actor->parent->id.c_str());
                                save_actor_ids.push_back(actor->parent->id);
                                actor_id_references->save_nstr(actor->parent->id);
                            }
                            auto it = std::find(save_actor_ids.begin(), save_actor_ids.end(), actor->parent->id);
                            Uint8 index  = static_cast<Uint8>(std::distance(save_actor_ids.begin(), it));
                            actor_chunk->save_uint8(index);
                            actor_chunk->save_uint32(static_cast<Uint16>(x)+(16*col));
                            actor_chunk->save_uint32(static_cast<Uint16>(y)+(16*row));
                            actor_chunk->save_uint8(static_cast<Uint8>(actor->owner));
                            actor_chunk->save_uint16( actor->max_hp == actor->parent->max_hp
                                ? static_cast<Uint16>(65535)
                                : static_cast<Uint16>(actor->max_hp));
                            actor_chunk->save_uint16( actor->hp == actor->max_hp
                                ? static_cast<Uint16>(65535)
                                : static_cast<Uint16>(actor->hp));
                            actor_chunk->save_uint16( actor->movement_speed == actor->parent->movement_speed
                                ? static_cast<Uint16>(65535)
                                : static_cast<Uint16>(actor->movement_speed));
                            actor_chunk->save_uint16( actor->attack_value == actor->parent->attack_value
                                ? static_cast<Uint16>(65535)
                                : static_cast<Uint16>(actor->attack_value));
                            actor_chunk->save_uint16( actor->defence_value == actor->parent->defence_value
                                ? static_cast<Uint16>(65535)
                                : static_cast<Uint16>(actor->defence_value));
                            actors->add_child(std::move(actor_chunk));
                        }
                    }
                }
                data->add_child(std::move(chunk));
                col++;
            }
            row++;
        }

        file.write_chunk(std::move(meta));
        file.write_chunk(std::move(tile_id_references));
        file.write_chunk(std::move(actor_id_references));
        file.write_chunk(std::move(data));
        file.write_chunk(std::move(actors));
        file.move(4);
        file.write_sint32(file.get_size());
    }
    return true;
};


Map::Map(RenderAgent* agent, const std::filesystem::path& map_path) {
    this->agent = agent;
    this->path = replace_locations(map_path);
    if ((this->path.extension() == ".json") || (this->path.extension() == ".jsonc")) {
        iff = false;
        LOG(LogLevel::Debug, "Loading json data");
        rapidjson::Document map_json = open_json(this->path);
        if (!map_json.IsObject()) {
            LOG(LogLevel::Error, "Could not load map \"%s\": root is not an object.", path.u8string().c_str());
            return;
        }
        if (!map_json.HasMember("name")) {
            LOG(LogLevel::Error, "Could not load map \"%s\": json does not contain a \"name\" object.", path.u8string().c_str());
            return;
        }
        name = std::string(map_json["name"].GetString());

        if (map_json.HasMember("description"))
            description = std::string(map_json["description"].GetString());
        else
            description = "No discription given";

        if (map_json.HasMember("author"))
            author = std::string(map_json["author"].GetString());
        else
            author = "Unknown";

        if (map_json.HasMember("creation_date"))
            creation_date = std::string(map_json["creation_date"].GetString());
        else
            creation_date = "xxxx-xx-xx";

        if (map_json.HasMember("version"))
            version = map_json["version"].GetInt();
        else
            version = 1;

        if (!map_json.HasMember("data")) {
            LOG(LogLevel::Error, "Could not load map \"%s\": json does not contain a \"data\" object.", name.c_str());
            return;
        }
        if (map_json["data"].Size() == 0) {
            LOG(LogLevel::Error, "Could not load map \"%s\": \"data\" object can not be empty.", name.c_str());
            return;
        }

        rows = static_cast<Uint32>(map_json["data"].Size());
        cols = static_cast<Uint32>(map_json["data"][0].Size());
        allocate();
        atlas_name = "map:"+name+":atlas:tile_textures";
    } else if (this->path.extension() == ".iff") {
        iff = true;
        IFF_File file(this->path);
        std::unique_ptr<IFF_Chunk> meta_unique = file.find_chunk("META", 0);
        IFF_RawChunk* meta = static_cast<IFF_RawChunk*>(meta_unique.get());
        version = meta->load_uint32();
        author = meta->load_nstr();
        creation_date = meta->load_nstr();
        name = meta->load_nstr();
        description = meta->load_nstr();
        rows = meta->load_uint32();
        cols = meta->load_uint32();

        allocate();
        atlas_name = "map:"+name+":atlas:tile_textures";

        std::unique_ptr<IFF_Chunk> tile_ids_chunk_unique =  file.find_chunk("TLID", 0);
        std::unique_ptr<IFF_Chunk> actor_ids_chunk_unique = file.find_chunk("ACID", 0);
        IFF_RawChunk* tile_ids_chunk =  static_cast<IFF_RawChunk*>(tile_ids_chunk_unique.get());
        IFF_RawChunk* actor_ids_chunk = static_cast<IFF_RawChunk*>(actor_ids_chunk_unique.get());
        std::string current_id;
        while (tile_ids_chunk->offset < tile_ids_chunk->size) {
            current_id = tile_ids_chunk->load_nstr();
            if (current_id != "")
                tile_ids.push_back(current_id);
        }
        while (actor_ids_chunk->offset < actor_ids_chunk->size) {
            current_id = actor_ids_chunk->load_nstr();
            if (current_id != "")
                actor_ids.push_back(current_id);
        }

        LOG(LogLevel::Debug, "Baking atlas..");
        std::vector<std::string> atlas_tile_ids = tile_ids;
        auto it = std::find(atlas_tile_ids.begin(), atlas_tile_ids.end(), "td:none");
        if (it != atlas_tile_ids.end())
            atlas_tile_ids.erase(it);
        atlas_tile_ids.push_back("td:tile_missing");
        atlas_tile_ids.push_back("td:top_missing");
        atlas_tile_ids.push_back("td:missing");
        if (!agent->bake_atlas(atlas_name, atlas_tile_ids))
            LOG(LogLevel::Error, "Could not bake map atlas \"%s\"", atlas_name.c_str());
    }
};

Map::~Map() {};
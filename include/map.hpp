#ifndef MAP_HPP
#define MAP_HPP

#include <SDL3/SDL_stdinc.h>
#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <filesystem>
#include "rapidjson/rapidjson.h"

#include "renderring/render_agent.hpp"

#include "utils/iff.hpp"

#include "actors.hpp"

inline constexpr int chunk_size = 16;

struct MapTile {
    std::string base = "td:none";
    std::string top = "td:none";
    int height = 0;
    int x, y;
    RenderAgentEntity* top_entity;
    std::vector<RenderAgentEntity*> base_entities;
    std::vector<ActorInstance*> actors;
};

struct MapChunk {
    MapTile data[chunk_size][chunk_size];

    MapChunk() {};
};

class Map {
    private:
        RenderAgent* agent;
        std::vector<std::vector<MapChunk>> map_data;
        std::vector<std::string> tile_ids; // only for loading iff files
        std::vector<std::string> actor_ids; // only for loading iff files

        bool load_chunk_data_json(rapidjson::Value& chunk_json, const size_t& chunk_x, const size_t& chunk_y, std::set<std::string>& tile_textures);
        bool load_actors_json(rapidjson::Value& actors_json);
        bool load_chunk_data_iff(IFF_RawChunk* chunk_data, const size_t& chunk_x, const size_t& chunk_y);
        bool load_actors_iff(IFF_FormChunk* actor_list);
        bool load_chunk_entities(const int& chunk_x, const int& chunk_y, std::vector<std::tuple<Sint64, Sint64, RenderAgentEntity>>& entities);
        void allocate();

    public:
        std::string name;
        std::filesystem::path path;
        std::string description;
        std::string author;
        std::string creation_date;
        Uint32 version;
        Uint32 rows; // rows of chunks
        Uint32 cols;
        std::string atlas_name;
        bool iff; // true = iff; false = json

        std::tuple<int, int, int, int> get_surrounding(const int& col, const int& row);
        MapChunk* get_chunk(const int& chunk_x, const int& chunk_y, const bool& suppress_logs=false);
        MapTile* get_tile(const int& col, const int& row, const bool& suppress_logs=false);

        bool load_actors();
        bool load_chunks(const std::vector<std::tuple<Uint32, Uint32>>& chunk_positions);
        bool save(const std::filesystem::path& path);

        // INIT & CLEANUP //
        Map(RenderAgent* agent, const std::filesystem::path& map_path);
        ~Map();
};

inline Map* MAIN_MAP;

#endif
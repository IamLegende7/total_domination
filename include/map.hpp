#ifndef MAP_HPP
#define MAP_HPP

#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <filesystem>
#include "rapidjson/rapidjson.h"

#include "renderring/render_agent.hpp"

#include "actors.hpp"

inline constexpr int chunk_size = 16;

// TODO: tile effects
struct MapTileEffect {
    std::string id;
};

struct MapTileEffectInstance {
    MapTileEffect* parent;
};

struct MapTile {
    std::string base;
    std::string top;
    int height;
    int x, y;
    RenderAgentEntity* top_entity;
    std::vector<RenderAgentEntity*> base_entities;
    std::vector<ActorInstance*> actors;
    std::vector<MapTileEffectInstance> effects;
};

struct ActorConstructor {
    std::string id;
    int owner;
    int col, row;
};

struct MapChunk {
    MapTile data[chunk_size][chunk_size];
};

class Map {
    private:
        RenderAgent* agent;
        std::vector<std::vector<MapChunk>> map_data;

        bool load_chunk_data(rapidjson::Value& chunk_json, const size_t& chunk_x, const size_t& chunk_y, std::set<std::string>& tile_textures, std::vector<ActorConstructor>& actor_constructors);
        bool load_chunk_entities(const int& chunk_x, const int& chunk_y, std::vector<std::tuple<int, int, RenderAgentEntity>>& entities);

    public:
        std::string name;
        std::filesystem::path path;
        std::string description;
        size_t rows;
        size_t cols;
        std::string atlas_name;

        std::tuple<int, int, int, int> get_surrounding(const int& col, const int& row);
        MapChunk* get_chunk(const int& chunk_x, const int& chunk_y, const bool& suppress_logs=false);
        MapTile* get_tile(const int& col, const int& row, const bool& suppress_logs=false);

        bool load_chunks(const std::vector<std::tuple<int, int>>& chunk_positions);

        // INIT & CLEANUP //
        Map(RenderAgent* agent, const std::filesystem::path& map_path);
        ~Map();
};

inline Map* MAIN_MAP;

#endif
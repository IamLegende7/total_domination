#ifndef MAP_HPP
#define MAP_HPP

#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <filesystem>
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

#include "renderring/render_agent.hpp"

struct TileResources {

};

struct MapTile {
    std::string base;
    std::string top_tile;
    std::string top;
    int height;
    int x, y;
    int size;
    TileResources resources;

    MapTile():
        base("td:missing"), top_tile("td:none"), top("td:none"), height(1), x(-100), y(-100), size(1) {};
    MapTile(const std::string& base, int x, int y):
        base(base), top_tile("td:none"), top("td:none"), height(1), x(x), y(y), size(1) {};
};

class Map {
    private:
        RenderAgent* agent;
        MapTile** map_data;
        std::vector<RenderAgentEntity>** entity_cache;
        
        bool load_row(const rapidjson::GenericValue<rapidjson::UTF8<>>& row_json, const size_t row, std::set<std::string>& tile_textures);
        RenderAgentEntity make_tile_entity(const std::string& name, const std::string& sprite_id, const int& x, const int& y, const int& height_index);
        bool make_row_entitys(MapTile row[], size_t row_size, int row_index);
    public:
        std::string map_name;
        std::filesystem::path map_path;
        std::string map_description;
        size_t rows;
        size_t cols;

        std::tuple<int, int, int, int> get_surrounding(const int row, const int col);
        MapTile* get_tile(const int row, const int col, const bool suppress_logs=false);

        // INIT & CLEANUP //
        Map(RenderAgent* agent, const std::filesystem::path& map_path);
        ~Map();
};

inline Map* MAIN_MAP;

#endif
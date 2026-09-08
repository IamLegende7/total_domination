#ifndef ACTORS_HPP
#define ACTORS_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <cstdint>

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

#include "renderring/render_agent.hpp"

#include "utils/quadtree.hpp"

struct ActorFunction{
    std::string function;
    rapidjson::Document arguments = rapidjson::Document(rapidjson::kArrayType);
};

struct Actor {
    std::string id;
    std::string name;
    std::string type;
    std::string sprite;
    int max_hp;
    int movement_speed = 0;
    int attack_value = 0, defence_value = 0;
    std::unordered_map<std::string, std::vector<ActorFunction>> functions = {};
};

struct ActorInstance {
    std::string id;
    Actor* parent;
    RenderAgentEntity* entity;
    int hp, max_hp;
    int movement_speed;
    int attack_value, defence_value;
    int x, y;
    int width = 1, height = 1; // keep Quadtree happy
};

class ActorHandler {
    private:
        RenderAgent* agent = nullptr;
        RenderAgent* map_agent = nullptr;
        std::deque<Actor> actors;
        QuadtreeNode<ActorInstance> instances;
        SDL_Color player_colour = player_colour;
        uint8_t player_num = player_num;
    public:
        ActorHandler(RenderAgent* agent, RenderAgent* map_agent, const SDL_Color& player_colour, uint8_t player_num);
        ~ActorHandler();

        // Actors
        bool load_actor(const std::string& id);
        Actor* get_actor(const std::string& id, bool suppress_logs=false);

        // ActorInstances
        int get_instance_count(const std::string& id);
        bool spawn_actor(const std::string& id, const int& col, const int& row);
        bool delete_instance(const std::string& id);
        ActorInstance* get_instance(const std::string& id, bool suppress_logs=false);
        ActorInstance* get_instance(const int& col, const int& row, bool suppress_logs=false);

        // Actor functions
        bool round();
};

#endif
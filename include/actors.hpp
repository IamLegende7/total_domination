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

struct ActorFunction {
    std::string function;
    rapidjson::Document arguments = rapidjson::Document(rapidjson::kArrayType);

    ActorFunction() = default;
    ActorFunction(const ActorFunction& other) {
        function = other.function;
        arguments.CopyFrom(other.arguments, arguments.GetAllocator());
    }

    ActorFunction& operator=(const ActorFunction& other) {
        if (this != &other) {
            function = other.function;
            arguments.CopyFrom(other.arguments, arguments.GetAllocator());
        }
        return *this;
    }

    ActorFunction(ActorFunction&&) noexcept = default;
    ActorFunction& operator=(ActorFunction&&) noexcept = default;
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
};

class ActorHandler {
    private:
        RenderAgent* agent = nullptr;
        std::deque<Actor> actors;
        std::deque<ActorInstance> instances;
        SDL_Color player_colour = player_colour;
        uint8_t player_num = player_num;

        bool execute_actor_function(ActorInstance* instance, const std::string& key);
    public:
        ActorHandler(RenderAgent* agent, const SDL_Color& player_colour, uint8_t player_num);
        ~ActorHandler();

        // Actors
        bool load_actor(const std::string& id);
        Actor* get_actor(const std::string& id, bool suppress_logs=false);

        // ActorInstances
        int get_instance_count(const std::string& id);
        ActorInstance* spawn_actor(const std::string& id, const int& col, const int& row);
        bool delete_instance(const std::string& id);
        ActorInstance* get_instance(const std::string& id, bool suppress_logs=false);
        int get_instance_index(const std::string& id, bool suppress_logs=true);

        // Actor functions
        bool round();
};

#endif
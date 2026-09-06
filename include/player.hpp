#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <SDL3/SDL_pixels.h>
#include <string>
#include <unordered_map>
#include <cstdint>

#include "actors.hpp"

class Player {
    private:
        std::unordered_map<std::string, int> resources = {};
    public:
        int number;
        std::string name;
        std::string faction;
        SDL_Color colour;
        bool is_comp = false;
        ActorHandler* actor_handler = nullptr;

        Player(const int& number, const std::string& name, const std::string& faction, const SDL_Color& colour = {255, 0, 0, 255});
        ~Player();

        int* get_resource(const std::string& resource_id, bool suppress_logs=false);
        bool set_resource(const std::string& resource_id, const int& count);
};

inline std::vector<Player> PLAYERS;

#endif
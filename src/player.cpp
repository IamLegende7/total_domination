#include <string>

#include "agents.hpp"
#include "player.hpp"
#include "actors.hpp"
#include "registry.hpp"

#include "utils/logger.hpp"

Player::Player(const int& number, const std::string& name, const std::string& faction, const SDL_Color& colour) {
    this->number = number;
    this->name = name;
    this->faction = faction;
    actor_handler = new ActorHandler(ACTORS_RENDER_AGENT, colour, number);
    for (auto& [key, value] : REGISTRY->get_category("resources")) {
        resources[key] = 0;
    }
}

Player::~Player() {}

int* Player::get_resource(const std::string& resource_id, bool suppress_logs) {
    for (auto& [key, value] : resources) {
        if (key == resource_id)
            return &value;
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested nonexistent resource \"%s\".", resource_id.c_str());
    return nullptr;
}

bool Player::set_resource(const std::string& resource_id, const int& count) {
    resources[resource_id] = count;
    LOG(LogLevel::Debug, "Resource \"%s\" is now of count %d", resource_id.c_str(), count);
    return true;
}
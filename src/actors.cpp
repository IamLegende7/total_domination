#include <SDL3/SDL_stdinc.h>
#include <string>
#include <filesystem>
#include <vector>

#include "actors.hpp"
#include "map.hpp"
#include "registry.hpp"

#include "renderring/render_agent.hpp"

#include "mods/mods.hpp"

#include "utils/logger.hpp"
#include "utils/json.hpp"

#include "settings/locations.hpp"

bool ActorHandler::execute_actor_function(ActorInstance* instance, const std::string& key) {
    if (!instance || !instance->parent)
        return false;

    std::vector<ActorFunction> functions_to_execute;
    auto it = instance->parent->functions.find(key);
    if (it == instance->parent->functions.end()) {
        return true;
    }
    functions_to_execute = it->second;
    const std::string instance_id = instance->id;

    for (auto& function : functions_to_execute) {
        rapidjson::Document arguments(rapidjson::kArrayType);
        rapidjson::Document::AllocatorType& allocator = arguments.GetAllocator();
        rapidjson::Value arg0;
        arg0.SetString(instance->id.c_str(), allocator);
        arguments.PushBack(arg0, allocator);
        for (rapidjson::SizeType i = 0; i < function.arguments.Size(); i++) {
            rapidjson::Value arg;
            arg.CopyFrom(function.arguments[i], allocator);
            arguments.PushBack(arg, allocator);
        }
        ModServerRequest response = MOD_SERVER->execute(function.function, arguments);
        if (response.get_status() != 0) {
            LOG(LogLevel::Warning, "Could not execute \"%s\": Code: %d", function.function.c_str(), response.get_status());
        }
    }
    return true;
}

ActorHandler::ActorHandler(RenderAgent* agent, const SDL_Color& player_colour, Uint8 player_num) {
    this->agent = agent;
    this->player_colour = player_colour;
    this->player_num = player_num;
}

ActorHandler::~ActorHandler() {};

bool ActorHandler::load_actor(const std::string& id) {
    if (get_actor(id, true) != nullptr) {
        LOG(LogLevel::Warning, "Could not load actor \"%s\": actor already loaded", id.c_str());
        return false;
    }
    std::filesystem::path path = REGISTRY->get("actors", id, std::filesystem::path("none"));
    if (path == std::filesystem::path("none")) {
        LOG(LogLevel::Error, "Could not load actor \"%s\": id not found in registry", id.c_str());
        return false;
    }
    rapidjson::Document actor_json = open_json(path);
    if (
        !actor_json.HasMember("name") ||
        !actor_json.HasMember("type") ||
        !actor_json.HasMember("hp") ||
        !actor_json.HasMember("code")
    ) {
        LOG(LogLevel::Error, "Could not load actor \"%s\": json file \"%s\" is missing one or more of [\"name\", \"type\", \"hp\", \"code\"]", id.c_str(), path.u8string().c_str());
        return false;
    }
    if (
        !actor_json["name"].IsString() ||
        !actor_json["type"].IsString() ||
        !actor_json["hp"].IsInt() ||
        !actor_json["code"].IsObject()
    ) {
        LOG(LogLevel::Error, "Could not load actor \"%s\": json file \"%s\" has one or more incorrect types. Should be: [\"name\": str, \"type\": str, \"hp\": int, \"code\": Object]", id.c_str(), path.u8string().c_str());
        return false;
    }
    bool atlas_status;
    if (!agent->get_texture("atlas:actors", true))
        atlas_status = agent->bake_atlas("atlas:actors", std::vector<std::string>(1, id), player_colour, player_num);
    else
        atlas_status = agent->add_to_atlas("atlas:actors", id, player_colour, player_num);
    if (!atlas_status) {
        LOG(LogLevel::Error, "Failed to add texture \"%s\", to atlas \"%s\".", id.c_str(), id.c_str());
        return false;
    }
    Actor actor = {
        id,
        actor_json["name"].GetString(),
        actor_json["type"].GetString(),
        id+std::to_string(player_num),
        actor_json["hp"].GetInt()
    };
    if (actor_json.HasMember("movement_speed")) {
        if (!actor_json["movement_speed"].IsInt())
            LOG(LogLevel::Warning, "Movement speed in \"%s\" is not of type \"int\".", path.u8string().c_str());
        else
            actor.movement_speed = actor_json["movement_speed"].GetInt();
    }
    if (actor_json.HasMember("attack_value")) {
        if (!actor_json["attack_value"].IsInt())
            LOG(LogLevel::Warning, "Attack value in \"%s\" is not of type \"int\".", path.u8string().c_str());
        else
            actor.attack_value = actor_json["attack_value"].GetInt();
    }
    if (actor_json.HasMember("defence_value")) {
        if (!actor_json["defence_value"].IsInt())
            LOG(LogLevel::Warning, "Defence value in \"%s\" is not of type \"int\".", path.u8string().c_str());
        else
            actor.defence_value = actor_json["defence_value"].GetInt();
    }
    rapidjson::Value& code_json = actor_json["code"];
    // TOOD: error handling
    for (rapidjson::Value::ConstMemberIterator itr = code_json.MemberBegin(); itr != code_json.MemberEnd(); ++itr) {
        const std::string func_key = itr->name.GetString();

        for (rapidjson::SizeType i = 0; i < itr->value.Size(); i++) {
            ActorFunction code_func;
            code_func.function = itr->value[i]["function"].GetString();
            code_func.arguments.SetArray();

            rapidjson::Document::AllocatorType& allocator = code_func.arguments.GetAllocator();
            for (rapidjson::SizeType args_i = 0; args_i < itr->value[i]["arguments"].Size(); args_i++) {
                const rapidjson::Value& source = itr->value[i]["arguments"][args_i];

                rapidjson::Value argument;
                argument.CopyFrom(source, allocator);

                code_func.arguments.PushBack(
                    std::move(argument),
                    allocator
                );
            }
            //LOG(LogLevel::Debug, "Adding function \"%s\" to actor \"%s\"", code_func.function.c_str(), actor.id.c_str());
            actor.functions[func_key].push_back(std::move(code_func));
        }
    }

    LOG(LogLevel::Debug, "Loaded actor \"%s\"", actor.id.c_str());
    actors.push_back(std::move(actor));
    return true;
}

Actor* ActorHandler::get_actor(const std::string& id, bool suppress_logs) {
    for (auto& actor : actors) {
        if (actor.id == id)
            return &actor;
    }
    if (!suppress_logs)
        LOG(LogLevel::Warning, "Actor \"%s\" not found.", id.c_str());
    return nullptr;
}

ActorInstance* ActorHandler::spawn_actor(const std::string& id, const int& col, const int& row, const bool ignore_occupied) {
    MapTile* tile = MAIN_MAP->get_tile(col, row);
    if (tile == nullptr) {
        LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: tile does not exist.", id.c_str(), col, row);
        return nullptr;
    }
    if ((tile->actors.size() != 0) && !ignore_occupied) {
        LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: an instance already exists at that tile.", id.c_str(), col, row);
        return nullptr;
    }
    if (get_actor(id, true) == nullptr) {
        if (!load_actor(id)) {
            LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: could not add actor.", id.c_str(), col, row);
            return nullptr;
        }
    }
    Actor* parent = get_actor(id);
    if (!parent) {
        LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: parent not loaded.", id.c_str(), col, row);
        return nullptr;
    }
    int instances_same_actor_count = 0;
    for (auto& entry : instances) {
        if (entry.id == id)
            instances_same_actor_count++;
    }
    const std::string entity_id = id + std::string("-") + std::to_string(instances_same_actor_count);

    int x = (16*(col-row));
    int y = (11*(row+col)-(16*(tile->height-1)))-11;
    int layer = ((MAIN_MAP->cols-1)*row) + (col) + 1;
    int quadtree_x, quadtree_y, quadtree_w, quadtree_h;
    int w = agent->get_sprite(parent->sprite)->animations[0].texture_rects[0].w;
    int h = agent->get_sprite(parent->sprite)->animations[0].texture_rects[0].h;
    agent->get_dimensions(quadtree_x, quadtree_y, quadtree_w, quadtree_h);
    if (x <= quadtree_x) {
        LOG(LogLevel::Debug, "Adjusting Quadtree X..");
        agent->set_dimensions(x, quadtree_y, (quadtree_x-x)+quadtree_w, quadtree_h);
        agent->get_dimensions(quadtree_x, quadtree_y, quadtree_w, quadtree_h);
    }
    if (y <= quadtree_y) {
        LOG(LogLevel::Debug, "Adjusting Quadtree Y..");
        agent->set_dimensions(quadtree_x, y, quadtree_w, (quadtree_y-y)+quadtree_h);
        agent->get_dimensions(quadtree_x, quadtree_y, quadtree_w, quadtree_h);
    }
    if (x+w >= quadtree_x+quadtree_w) {
        LOG(LogLevel::Debug, "Adjusting Quadtree W..");
        agent->set_dimensions(quadtree_x, quadtree_y, x+w-quadtree_x, quadtree_h);
        agent->get_dimensions(quadtree_x, quadtree_y, quadtree_w, quadtree_h);
    }
    if (y+h >= quadtree_y+quadtree_h) {
        LOG(LogLevel::Debug, "Adjusting Quadtree H..");
        agent->set_dimensions(quadtree_x, quadtree_y, quadtree_w, y+h-quadtree_y);
        agent->get_dimensions(quadtree_x, quadtree_y, quadtree_w, quadtree_h);
    }
    //LOG(LogLevel::Debug, "Dimensions: x: %d, y: %d, w: %d, h: %d", quadtree_x, quadtree_y, quadtree_w, quadtree_h);
    RenderAgentEntity* entity = agent->add_entity(parent->sprite, 0, x, y, layer, 0, (parent->movement_speed > 0), true);
    ActorInstance instance = {
        entity_id,
        parent,
        entity,
        player_num,
        parent->max_hp,
        parent->max_hp,
        parent->movement_speed,
        parent->attack_value,
        parent->defence_value,
        col,
        row
    };
    instances.push_back(instance);
    tile->actors.push_back(&instances.back());

    execute_actor_function(&instances.back(), "spawn");

    LOG(LogLevel::Debug, "Spawned instance of \"%s\" at %d %d. Length is now: %d; Length of tile->actors: %d", id.c_str(), col, row, instances.size(), tile->actors.size());
    agent->set_dirty();
    return &instances.back();
}

bool ActorHandler::delete_instance(const std::string& id) {
    int index = get_instance_index(id);
    ActorInstance* instance = &instances[index];
    MapTile* tile = MAIN_MAP->get_tile(instance->x, instance->y);
    for (size_t i = 0; i < tile->actors.size(); ++i) {
        if (tile->actors[i]->id == id)
            tile->actors.erase(tile->actors.begin()+i);
    }
    if (index >= 0)
        instances.erase(instances.begin() + index);
    return true;
}

ActorInstance* ActorHandler::get_instance(const std::string& id, bool suppress_logs) {
    for (auto& instance : instances) {
        if (instance.id == id) {
            return &instance;
        }
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent instance with id \"%s\"", id);
    return nullptr;
}

int ActorHandler::get_instance_index(const std::string& id, bool suppress_logs) {
    int index = 0;
    for (auto& instance : instances) {
        if (instance.id == id) {
            return index;
        }
        index++;
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent instance with id \"%s\"", id);
    return -1;
}

bool ActorHandler::round() {
    for (auto& instance : instances) {
        execute_actor_function(&instance, "round");
    }
    return true;
}
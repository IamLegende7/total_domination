#include <string>
#include <filesystem>

#include "actors.hpp"
#include "map.hpp"

#include "renderring/render_agent.hpp"
#include "renderring/textures.hpp"

#include "mods/mods.hpp"

#include "utils/quadtree.hpp"
#include "utils/logger.hpp"
#include "utils/json.hpp"

#include "settings/locations.hpp"

ActorHandler::ActorHandler(RenderAgent* agent, RenderAgent* map_agent) {
    this->agent = agent;
    this->map_agent = map_agent;
    instances.set_capacity(16);
    agent->agent_entitys.set_dimensions(map_agent->agent_entitys.x, map_agent->agent_entitys.y, map_agent->agent_entitys.width, map_agent->agent_entitys.height);
    instances.set_dimensions(map_agent->agent_entitys.x, map_agent->agent_entitys.y, map_agent->agent_entitys.width, map_agent->agent_entitys.height);
}

ActorHandler::~ActorHandler() {};

bool ActorHandler::load_actor(const std::string& id) {
    if (get_actor(id, true) != nullptr) {
        LOG(LogLevel::Warning, "Could not load actor \"%s\": actor already loaded", id.c_str());
        return false;
    }
    std::filesystem::path path = replace_locations("$actor_dir$/buildings/industrial_lumberjack.jsonc"); // TODO: load from registry system
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
    std::string texture_names[] = {id};
    bool atlas_status = bake_atlas(agent, id, texture_names, sizeof(texture_names)/sizeof(texture_names[0])); // TODO: change to add_to_atlas()
    if (!atlas_status) {
        LOG(LogLevel::Error, "Failed to add texture \"%s\", to atlas \"%s\".", id.c_str(), id.c_str());
        return false;
    }
    Actor actor = {
        id,
        actor_json["name"].GetString(),
        actor_json["type"].GetString(),
        id,
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

bool ActorHandler::spawn_actor(const std::string& id, const int& col, const int& row) {
    if (get_instance(col, row, true) != nullptr) {
        LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: an instance already exists at that tile.", id.c_str(), col, row);
        return false;
    }
    if (get_actor(id, true) == nullptr) {
        if (!load_actor(id)) {
            LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: could not add actor.", id.c_str(), col, row);
            return false;
        }
    }
    Actor* parent = get_actor(id);
    if (!parent) {
        LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: parent not loaded.", id.c_str(), col, row);
        return false;
    }
    std::vector<ActorInstance*> all_instances;
    int instances_same_actor_count = 0;
    instances.query_all(all_instances);
    for (auto& entry : all_instances) {
        if (entry->id == id)
            instances_same_actor_count++;
    }
    const std::string entity_id = id + std::string("-") + std::to_string(instances_same_actor_count);
    MapTile* tile = MAIN_MAP->get_tile(row, col);
    if (tile == nullptr) {
        LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: tile does not exist.", id.c_str(), col, row);
        return false;
    }
    std::string map_entity_name = "map:tile:"+std::to_string(col)+"x"+std::to_string(row);
    RenderAgentEntity* map_tile_entity = map_agent->get_entity(map_entity_name+":top", true);
    if (map_tile_entity == nullptr) {
        map_tile_entity = map_agent->get_entity(map_entity_name+":top_tile", true);
        if (map_tile_entity == nullptr) {
            map_tile_entity = map_agent->get_entity(map_entity_name+":base", true);
            if (map_tile_entity == nullptr) {
                LOG(LogLevel::Warning, "Could not spawn instance of actor \"%s\" at %d %d: tile entity does not exist.", id.c_str(), col, row);
                return false;
            }
        }
    }
    int x = (16*(col-row));
    int y = (11*(row+col)-(16*(tile->height-1)))-11;
    agent->add_entity(entity_id, parent->sprite, "default", x, y, map_tile_entity->layer+2);
    ActorInstance instance = {
        id,
        parent,
        entity_id,
        parent->max_hp,
        parent->max_hp,
        parent->movement_speed,
        parent->attack_value,
        parent->defence_value,
        col,
        row
    };
    int max_depth = -1;
    if (instance.movement_speed > 0)
        max_depth = 0;
    instances.insert(entity_id, instance, max_depth, true);
    //LOG(LogLevel::Debug, "Spawned instance of \"%s\" at %d %d. Length is now: %d", id.c_str(), col, row, all_instances.size());
    agent->set_dirty();
    return true;
}

ActorInstance* ActorHandler::get_instance(const int& col, const int& row, bool suppress_logs) {
    std::vector<ActorInstance*> result;
    instances.query_all(result);
    if (result.size() > 0) {
        for (ActorInstance* instance : result) {
            if (
                (instance->x == col) &&
                (instance->y == row)
            )
            return instance;
        }
    }

    if (!suppress_logs)
        LOG(LogLevel::Warning, "Requested non-existent instance at %d %d", col, row);
    return nullptr;
}

bool ActorHandler::round() {
    std::vector<ActorInstance*> all_instances;
    instances.query_all(all_instances);
    for (auto& instance : all_instances) {
        for (auto& [key, value] : instance->parent->functions) {
            if (key == "round") {
                for (auto& function : value) {
                    const std::size_t sep = function.function.find(':');
                    std::string mod = ""; 
                    std::string func = "";
                    if (sep != std::string::npos) {
                        mod = function.function.substr(0, sep);
                        func = function.function.substr(sep+1);
                    }
                    ModServerResponse response = MOD_SERVER->execute(mod, func, function.arguments);
                    if (response.status != 0) {
                        LOG(LogLevel::Warning, "Could not execute \"%s\": Code: %d: \"%s\"", function.function.c_str(), response.status, response.message.c_str());
                    }
                }
                break;
            }
        }
    }
    return true;
}
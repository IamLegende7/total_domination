#include <string>

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

#include "utils/logger.hpp"

#include "settings/locations.hpp"

#include "mods/mods.hpp"

#include "player.hpp"
#include "actors.hpp"
#include "main.hpp"
#include "registry.hpp"
#include "agents.hpp"

// TODO: more and better error handling:
//      nonsensical arguments are often not checked for -> crashes
//      if the arguments are of wrong type / wrong count the return values aren't set

void ModServerFunctions::log(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 3) {
        LOG(LogLevel::Warning, "Could not execute function \"LOG\": args.Size() < 3");
    } else if (!args[0].IsInt() || !args[1].IsString() || !args[2].IsString()) {
        LOG(LogLevel::Warning, "Could not execute function \"LOG\": one or more keys are of incorrect type: [int, str, str]");
    } else {
        int loglevel_int = args[0].GetInt();
        LogLevel loglevel;
        if (loglevel_int == 0)
            loglevel = LogLevel::Debug;
        else if (loglevel_int == 1)
            loglevel = LogLevel::Info;
        else if (loglevel_int == 2)
            loglevel = LogLevel::Warning;
        else if (loglevel_int == 3)
            loglevel = LogLevel::Error;
        else if (loglevel_int == 4)
            loglevel = LogLevel::Critical;
        LOGGER.log("ModServer", args[2].GetString(), loglevel, args[1].GetString());
        output["status"].SetInt(0);
    }
}

void ModServerFunctions::get_resource(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 1) {
        LOG(LogLevel::Warning, "Could not execute function \"get_resource\": args.Size() < 1");
    } else if (!args[0].IsString()) {
        LOG(LogLevel::Warning, "Could not execute function \"get_resource\": one or more keys are of incorrect type: [str]");
    } else {
        const std::string resource_id = args[0].GetString();
        int* resource_count = PLAYERS[0].get_resource(resource_id);
        rapidjson::Value count;
        if (resource_count == nullptr) {
            output["status"].SetInt(1);
            count.SetInt(0);
        } else {
            output["status"].SetInt(0);
            count.SetInt(*resource_count);
        }
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
        output["return"].AddMember("count", count, allocator);
    }
}

void ModServerFunctions::set_resource(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 2) {
        LOG(LogLevel::Warning, "Could not execute function \"set_resource\": args.Size() < 2");
    } else if (!args[0].IsString() || !args[1].IsInt()) {
        LOG(LogLevel::Warning, "Could not execute function \"set_resource\": one or more keys are of incorrect type: [str, int]");
    } else {
        const std::string resource_id = args[0].GetString();
        const int resource_count = args[1].GetInt();
        bool status_bool = PLAYERS[0].set_resource(resource_id, resource_count); // TODO: pass in player number as arg
        if (status_bool) {
            output["status"].SetInt(1);
        } else {
            output["status"].SetInt(0);
        }
    }
}

void ModServerFunctions::get_pos(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 1) {
        LOG(LogLevel::Warning, "Could not execute function \"get_pos\": args.Size() < 1");
    } else if (!args[0].IsString()) {
        LOG(LogLevel::Warning, "Could not execute function \"get_pos\": one or more keys are of incorrect type: [str]");
    } else {
        const std::string actor_id = args[0].GetString();
        ActorInstance* instance = nullptr;
        for (int player_num = 0; player_num < PLAYER_COUNT; ++player_num) {
            instance = PLAYERS[player_num].actor_handler->get_instance(actor_id, true);
            if (instance)
                break;
        }
        rapidjson::Value col;
        rapidjson::Value row;
        if (instance == nullptr) {
            LOG(LogLevel::Warning, "Could not execute function \"get_pos\": instance not found");
            output["status"].SetInt(1);
            col.SetInt(-1);
            row.SetInt(-1);
        } else {
            output["status"].SetInt(0);
            col.SetInt(instance->x);
            row.SetInt(instance->y);
        }
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
        output["return"].AddMember("col", col, allocator);
        output["return"].AddMember("row", row, allocator);
    }
}

void ModServerFunctions::get_owner(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 1) {
        LOG(LogLevel::Warning, "Could not execute function \"get_owner\": args.Size() < 1");
    } else if (!args[0].IsString()) {
        LOG(LogLevel::Warning, "Could not execute function \"get_owner\": one or more keys are of incorrect type: [str]");
    } else {
        const std::string actor_id = args[0].GetString();
        int result = -1;
        for (int i = 0; i < PLAYER_COUNT; ++i) {
            if (PLAYERS[i].actor_handler->get_instance(actor_id, true) != nullptr) {
                result = i;
                break;
            }
        }
        rapidjson::Value player_num;
        if (result == -1) {
            LOG(LogLevel::Warning, "Could not execute function \"get_owner\": instance not found");
            output["status"].SetInt(1);
            player_num.SetInt(-1);
        } else {
            output["status"].SetInt(0);
            player_num.SetInt(result);
        }
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
        output["return"].AddMember("player_num", player_num, allocator);
    }
}

void ModServerFunctions::get_faction(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 1) {
        LOG(LogLevel::Warning, "Could not execute function \"get_faction\": args.Size() < 1");
    } else if (!args[0].IsInt()) {
        LOG(LogLevel::Warning, "Could not execute function \"get_faction\": one or more keys are of incorrect type: [int]");
    } else {
        const int player_num = args[0].GetInt();
        if (player_num < 0 || player_num >= PLAYER_COUNT) {
            LOG(LogLevel::Warning, "Could not execute function \"get_faction\": player with number %d does not exist!", player_num);
        } else {
            std::string faction;
            if (player_num >= PLAYER_COUNT) {
                LOG(LogLevel::Warning, "Could not execute function \"get_faction\": player_num (%d) must be < PLAYER_COUNT (%d)", player_num, PLAYER_COUNT);
                faction = "";
            } else {
                faction = PLAYERS[player_num].faction;
            }
            rapidjson::Value player_faction;
            rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
            if (faction == "") {
                output["status"].SetInt(1);
                player_faction.SetString("", allocator);
            } else {
                output["status"].SetInt(0);
                player_faction.SetString(faction.c_str(), allocator);
            }
            output["return"].AddMember("faction", player_faction, allocator);
        }
    }
}

void ModServerFunctions::spawn_actor(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 4) {
        LOG(LogLevel::Warning, "Could not execute function \"spawn_actor\": args.Size() < 4");
    } else if (
        !args[0].IsString() ||
        !args[1].IsInt() ||
        !args[2].IsInt() ||
        !args[3].IsInt()
    ) {
        LOG(LogLevel::Warning, "Could not execute function \"spawn_actor\": one or more keys are of incorrect type: [str, int, int, int]");
    } else {
        const std::string actor_id = args[0].GetString();
        const int player_num = args[1].GetInt();
        const int col = args[2].GetInt();
        const int row = args[3].GetInt();

        bool spawn_status;
        if (player_num >= PLAYER_COUNT) {
            LOG(LogLevel::Warning, "Could not execute function \"spawn_actor\": player_num (%d) must be < PLAYER_COUNT (%d)", player_num, PLAYER_COUNT);
            spawn_status = false;
        } else {
            spawn_status = PLAYERS[player_num].actor_handler->spawn_actor(actor_id, col, row);
        }
        
        if (!spawn_status) {
            LOG(LogLevel::Warning, "Could not execute function \"spawn_actor\": spawning failed");
            output["status"].SetInt(1);
        } else {
            output["status"].SetInt(0);
        }
    }
}

void ModServerFunctions::delete_actor(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 1) {
        LOG(LogLevel::Warning, "Could not execute function \"delete_actor\": args.Size() < 1");
    } else if (!args[0].IsString()) {
        LOG(LogLevel::Warning, "Could not execute function \"delete_actor\": one or more keys are of incorrect type: [str]");
    } else {
        const std::string actor_id = args[0].GetString();
        ActorInstance* instance = nullptr;
        bool delete_status = false;
        for (int player_num = 0; player_num < PLAYER_COUNT; ++player_num) {
            instance = PLAYERS[player_num].actor_handler->get_instance(actor_id, true);
            if (instance) {
                ACTORS_RENDER_AGENT->delete_entity(instance->entity);
                PLAYERS[player_num].actor_handler->delete_instance(actor_id);
                delete_status = true;
                break;
            }
        }
        if (!delete_status) {
            LOG(LogLevel::Warning, "Could not execute function \"delete_actor\": instance not found");
            output["status"].SetInt(1);
        } else {
            output["status"].SetInt(0);
        }
    }
}

void ModServerFunctions::registry_add(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 3) {
        LOG(LogLevel::Warning, "Could not execute function \"registry_add\": args.Size() < 3");
    } else if (
        !args[0].IsString() ||
        !args[1].IsString() ||
        !args[2].IsString()
    ) {
        LOG(LogLevel::Warning, "Could not execute function \"registry_add\": one or more keys are of incorrect type: [str, str, str]");
    } else {
        const std::string category = args[0].GetString();
        const std::string key = args[1].GetString();
        const std::filesystem::path value = std::filesystem::path(args[2].GetString());

        if (!REGISTRY->add(category, key, value)) {
            LOG(LogLevel::Warning, "Could not execute function \"registry_add\": REGISTRY returned false");
            output["status"].SetInt(1);
        } else {
            output["status"].SetInt(0);
        }
    }
}

void ModServerFunctions::registry_load(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 2) {
        LOG(LogLevel::Warning, "Could not execute function \"registry_load\": args.Size() < 2");
    } else if (
        !args[0].IsString() ||
        !args[1].IsString()
    ) {
        LOG(LogLevel::Warning, "Could not execute function \"registry_load\": one or more keys are of incorrect type: [str, str]");
    } else {
        const std::string category = args[0].GetString();
        const std::filesystem::path file = std::filesystem::path(args[1].GetString());

        if (!REGISTRY->load(category, file)) {
            LOG(LogLevel::Warning, "Could not execute function \"registry_load\": REGISTRY returned false");
            output["status"].SetInt(1);
        } else {
            output["status"].SetInt(0);
        }
    }
}

void ModServerFunctions::registry_get(rapidjson::Value& args, rapidjson::Document& output) {
    if (args.Size() < 3) {
        LOG(LogLevel::Warning, "Could not execute function \"registry_get\": args.Size() < 3");
    } else if (
        !args[0].IsString() ||
        !args[1].IsString() ||
        !args[2].IsString()
    ) {
        LOG(LogLevel::Warning, "Could not execute function \"registry_get\": one or more keys are of incorrect type: [str, str, str]");
    } else {
        const std::string category = args[0].GetString();
        const std::string key = args[1].GetString();
        const std::filesystem::path default_value = std::filesystem::path(args[2].GetString());

        const std::filesystem::path result = REGISTRY->get(category, key, default_value);

        rapidjson::Value value;
        if (result == default_value) {
            LOG(LogLevel::Warning, "Could not execute function \"registry_get\": REGISTRY returned false");
            output["status"].SetInt(1);
        } else {
            output["status"].SetInt(0);
        }
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
        value.SetString(result.u8string().c_str(), allocator);
        output["return"].AddMember("value", value, allocator);
    }
}
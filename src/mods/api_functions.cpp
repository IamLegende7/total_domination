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
        rapidjson::Value status;
        status.SetInt(0);
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
        output.AddMember("status", status, allocator);
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
        rapidjson::Value status;
        rapidjson::Value count;
        if (resource_count == nullptr) {
            status.SetInt(1);
            count.SetInt(0);
        } else {
            status.SetInt(0);
            count.SetInt(*resource_count);
        }
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
        output.AddMember("status", status, allocator);
        output.AddMember("count", count, allocator);
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
        rapidjson::Value status;
        if (status_bool) {
            status.SetInt(1);
        } else {
            status.SetInt(0);
        }
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
        output.AddMember("status", status, allocator);
    }
}
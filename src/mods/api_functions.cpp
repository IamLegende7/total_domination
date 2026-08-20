#include <string>

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

#include "utils/logger.hpp"

#include "settings/locations.hpp"

#include "mods/mods.hpp"

void ModServerFunctions::log(rapidjson::Value& args,rapidjson::Document& output) {
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
    }
}
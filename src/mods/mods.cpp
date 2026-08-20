#include <SDL3/SDL_process.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_timer.h>
#include <string>
#include <filesystem>

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

#include "mods/mods.hpp"

#include "utils/logger.hpp"

#include "settings/locations.hpp"

ModServer::ModServer() {
    const std::filesystem::path server_path = LOCATIONS["mod_server_path"].get<std::filesystem::path>();
    const std::string server_path_str = server_path.u8string();
    const std::string server_log_file_str = (LOCATIONS["log_dir"].get<std::filesystem::path>() / std::filesystem::path("mods.log")).u8string();
    const char* args[] = {"python3", server_path_str.c_str(), server_log_file_str.c_str(), nullptr};
    process = SDL_CreateProcess(args, true);
    if (!process) {
        LOG(LogLevel::Error, "Could not create mod server process: %s", SDL_GetError());
        return;
    }
    output = SDL_GetProcessOutput(process);
    if (!output) {
        LOG(LogLevel::Error, "Could not create mod server output stream: %s", SDL_GetError());
        return;
    }
    input = SDL_GetProcessInput(process);
    if (!input) {
        LOG(LogLevel::Error, "Could not create mod server input stream: %s", SDL_GetError());
        return;
    }
}

ModServer::~ModServer() {
    if (process)
        SDL_DestroyProcess(process);
}

// Calls //
void ModServer::make_request(ModServerRequest& request) {
    rapidjson::Document request_json;
    request_json.SetObject();
    rapidjson::Value type;
    type.SetString(request.type.c_str(), request_json.GetAllocator());
    request_json.AddMember("type", type, request_json.GetAllocator());
    request_json.AddMember("data", request.data, request_json.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    request_json.Accept(writer);

    std::string request_str = std::string(buffer.GetString())+"\n";

    SDL_WriteIO(input, request_str.data(), request_str.size());
    SDL_FlushIO(input);
}

void ModServer::handle_function_request(ModServerResponse& function_request) {
    rapidjson::Value& data = function_request.data;
    rapidjson::Value args(rapidjson::kArrayType);
    
    ModServerRequest result = {"requested_function_response"};
    result.data.SetObject();
    rapidjson::Value status;
    status.SetInt(1);

    if (!data.IsObject()) {
        LOG(LogLevel::Error, "Could not process function: data is not an object.");
    } else if (
        !data.HasMember("function") ||
        !data.HasMember("args")
    ) {
        LOG(LogLevel::Error, "Could not process function: is missing one or more of: [\"function\", \"args\"].");
    } else if (
        !data["function"].IsString() ||
        !data["args"].IsArray()
    ) {
        LOG(LogLevel::Error, "Could not process function: one or more keys are of incorrect type: [\"function\": str, \"args\": list]");
    } else {
        std::string function_str = data["function"].GetString();
        args = data["args"];
        bool found = false;
        for (auto& [func_str, func] : ModServerFunctions::functions) {
            if (func_str == function_str) {
                func(args, result.data);
                found = true;
                break;
            }
        }
        if (!found)
            status.SetInt(2);
        else
            status.SetInt(0);
    }

    result.data.AddMember("status", status, result.data.GetAllocator());

    rapidjson::Document request_json;
    request_json.SetObject();
    rapidjson::Value type;
    type.SetString(result.type.c_str(), request_json.GetAllocator());
    request_json.AddMember("type", type, request_json.GetAllocator());
    request_json.AddMember("data", result.data, request_json.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    request_json.Accept(writer);

    std::string request_str = std::string(buffer.GetString())+"\n";

    SDL_WriteIO(input, request_str.data(), request_str.size());
    SDL_FlushIO(input);
}

ModServerResponse ModServer::get_response() {
    bool stop_loop = false;
    ModServerResponse response;
    while (!stop_loop) {    
        std::string output_str;
        char character = 0;
        while (character != '\n') {
            size_t n = SDL_ReadIO(output, &character, 1);
            if (n == 0) {
                SDL_Delay(1);
                continue;
            }
            output_str.push_back(character);
        }

        rapidjson::Document response_json;
        try {
            response_json.Parse<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(output_str.c_str(), strlen(output_str.c_str()));
            if (response_json.HasParseError()) {
                int error_offset = response_json.GetErrorOffset();
                const char* error_message = rapidjson::GetParseError_En(response_json.GetParseError());
                LOG(LogLevel::Error, "Error parsing json response: %s (offset: %d)", error_message, error_offset);
                response_json = rapidjson::Document();
                response_json.SetObject();
            }
        } catch (const std::exception& e) {
            LOG(LogLevel::Error, "Could not load json response: %s", e.what());
            response_json = rapidjson::Document();
            response_json.SetObject();
        }
        
        response = {3, "Error loading response.", "TDModServerComunicator"};
        if (!response_json.IsObject()) {
            LOG(LogLevel::Error, "Could not load response: root is not an object.");
            stop_loop = true;
        } else if (
            !response_json.HasMember("status") ||
            !response_json.HasMember("message") ||
            !response_json.HasMember("sender") ||
            !response_json.HasMember("data")
        ) {
            LOG(LogLevel::Error, "Could not load response: is missing one or more of: [\"status\", \"message\", \"sender\", \"data\"].");
            stop_loop = true;
        } else if (
            !response_json["status"].IsInt() ||
            !response_json["message"].IsString() ||
            !response_json["sender"].IsString() ||
            !response_json["data"].IsObject()
        ) {
            LOG(LogLevel::Error, "Could not load response: one or more keys are of incorrect type: [\"status\": int, \"message\": str, \"sender\": str, \"data\": dict].");
            stop_loop = true;
        } else {
            response.status = response_json["status"].GetInt();
            response.message = response_json["message"].GetString();
            response.sender = response_json["sender"].GetString();
            response.data = response_json["data"];
            if (response.status == -1) {
                handle_function_request(response);
            } else {
                stop_loop = true;
            }
        }
    }
    return response;
}

// API //
ModServerResponse ModServer::status() {
    ModServerRequest request = {
        "status"
    };
    MOD_SERVER->make_request(request);
    return MOD_SERVER->get_response();
}

ModServerResponse ModServer::execute(const std::string& mod, const std::string& function, rapidjson::Document& args) {
    ModServerRequest request = {
        "execute"
    };
    rapidjson::Value json_mod;
    rapidjson::Value json_function;

    request.data.SetObject();
    json_mod.SetString(mod.c_str(), request.data.GetAllocator());
    json_function.SetString(function.c_str(), request.data.GetAllocator());

    request.data.AddMember("mod", json_mod, request.data.GetAllocator());
    request.data.AddMember("function", json_function, request.data.GetAllocator());
    request.data.AddMember("args", args, request.data.GetAllocator());
    
    MOD_SERVER->make_request(request);
    return MOD_SERVER->get_response();
}

ModServerResponse ModServer::load_mod(const std::filesystem::path& path) {
    ModServerRequest request = {
        "load_mod"
    };
    rapidjson::Value json_mod_path;

    request.data.SetObject();
    json_mod_path.SetString(path.u8string().c_str(), request.data.GetAllocator());

    request.data.AddMember("mod_path", json_mod_path, request.data.GetAllocator());
    
    MOD_SERVER->make_request(request);
    return MOD_SERVER->get_response();
}
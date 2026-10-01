#include <SDL3/SDL_process.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_stdinc.h>
#include <string>
#include <filesystem>
#include <cstdint>

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

#include "mods/mods.hpp"

#include "utils/logger.hpp"

#include "settings/locations.hpp"
#include "settings/main.hpp"

int ModServerRequest::get_status() {
    if (!data.IsObject())
        return -200;
    if (!data.HasMember("status"))
        return -200;
    if (!data["status"].IsInt())
        return -200;
    return data["status"].GetInt();
}

std::string ModServer::type_to_string(ModServerRequestType& type) {
    switch (type) {
        case ModServerRequestType::execute:  return "execute";
        case ModServerRequestType::load_mod: return "load_mod";
        case ModServerRequestType::status:   return "status";
        case ModServerRequestType::response: return "response";
        case ModServerRequestType::error:    return "error";
        default:                             return "[Unknown ModServerRequestType]";
    }
}

ModServerRequestType ModServer::string_to_type(std::string string) {
    if (string == "execute")       return ModServerRequestType::execute;
    else if (string == "load_mod") return ModServerRequestType::load_mod;
    else if (string == "status")   return ModServerRequestType::status;
    else if (string == "response") return ModServerRequestType::response;
    else if (string == "error")    return ModServerRequestType::error;
    else                           return ModServerRequestType::error;
}

ModServer::ModServer() {
    const std::filesystem::path server_path = SETTINGS["mod_server_path"].get<std::filesystem::path>();
    const std::string server_path_str = server_path.u8string();
    const std::string server_log_file_str = (LOCATIONS["log_dir"].get<std::filesystem::path>() / std::filesystem::path("mods.log")).u8string();
    std::filesystem::path python_executable_path = SETTINGS["python_executable"].get<std::filesystem::path>();
    if (python_executable_path == std::filesystem::path("default") || !std::filesystem::exists(python_executable_path))
        #ifdef WIN32
            python_executable_path = LOCATIONS["shipped_python"].get<std::filesystem::path>() / std::filesystem::path("python.exe");
        #else
            python_executable_path = LOCATIONS["shipped_python"].get<std::filesystem::path>() / std::filesystem::path("bin/python");
        #endif
    if (!std::filesystem::exists(python_executable_path)) {
        LOG(LogLevel::Critical, "Python path is not valid: \"%s\"", python_executable_path.u8string().c_str());
        return;
    }
    const std::string python_executable_path_str = python_executable_path.u8string();
    const char* args[] = {python_executable_path_str.c_str(), server_path_str.c_str(), server_log_file_str.c_str(), nullptr};
    LOG(LogLevel::Debug, "Executing %s %s %s", args[0], args[1], args[2]);
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

// Tools //
std::string ModServer::uuid4(){
    uint8_t bytes[12];

    for (size_t i = 0; i < 12; i += 4) {
        uint32_t value = SDL_rand_bits();

        bytes[i + 0] = static_cast<uint8_t>(value >> 24);
        bytes[i + 1] = static_cast<uint8_t>(value >> 16);
        bytes[i + 2] = static_cast<uint8_t>(value >> 8);
        bytes[i + 3] = static_cast<uint8_t>(value);
    }
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;
    char buffer[37];

    std::snprintf(
        buffer,
        sizeof(buffer),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0],  bytes[1],  bytes[2],  bytes[3],
        bytes[4],  bytes[5],
        bytes[6],  bytes[7],
        bytes[8],  bytes[9],
        bytes[10], bytes[11], bytes[12], bytes[13],
        bytes[14], bytes[15]
    );

    return std::string(buffer);
}

// Calls //
void ModServer::send_request(ModServerRequest& request) {
    rapidjson::Document request_json;
    request_json.SetObject();

    rapidjson::Value uuid;
    uuid.SetString(request.uuid.c_str(), request_json.GetAllocator());
    rapidjson::Value type;
    type.SetString(type_to_string(request.type).c_str(), request_json.GetAllocator());
    rapidjson::Value origin;
    if (request.origin)
        origin.SetString("TDModServer", request_json.GetAllocator());
    else
        origin.SetString("TD", request_json.GetAllocator());

    request_json.AddMember("id", uuid, request_json.GetAllocator());
    request_json.AddMember("type", type, request_json.GetAllocator());
    request_json.AddMember("origin", origin, request_json.GetAllocator());
    request_json.AddMember("data", request.data, request_json.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    request_json.Accept(writer);

    std::string request_str = std::string(buffer.GetString())+"\n";

    //LOG(LogLevel::Debug, "Sending request: %s", request_str.c_str());
    SDL_WriteIO(input, request_str.data(), request_str.size());
    SDL_FlushIO(input);
}

void ModServer::handle_function_request(ModServerRequest& request) {
    rapidjson::Value& data = request.data;
    rapidjson::Value args(rapidjson::kArrayType);
    
    ModServerRequest result = {request.uuid, ModServerRequestType::response};
    result.data.SetObject();
    rapidjson::Value status;
    rapidjson::Value return_val;
    status.SetInt(1);
    return_val.SetObject();
    rapidjson::Document::AllocatorType allocator = result.data.GetAllocator();
    result.data.AddMember("status", status, allocator);
    result.data.AddMember("return", return_val, allocator);

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
        if (!found) {
            LOG(LogLevel::Warning, "Requested function \"%s\" not found.", function_str.c_str());
            result.data["status"].SetInt(2);
        }
    }

    send_request(result);
}

ModServerRequest ModServer::get_response() {
    bool stop_loop = false;
    ModServerRequest response;
    while (!stop_loop) {    
        std::string output_str;
        char character = 0;
        while (character != '\n') {
            size_t n = SDL_ReadIO(output, &character, 1);
            if (SDL_GetIOStatus(output) == SDL_IO_STATUS_EOF) {
                LOG(LogLevel::Error, "Mod server closed its output stream.");
                response.type = ModServerRequestType::error;
                response.data.SetObject();
                return response;
            }
            if (n == 0) {
                SDL_Delay(1);
                continue;
            }
            if (character != '\n')
                output_str.push_back(character);
        }
        //LOG(LogLevel::Debug, "Recieved: \"%s\"", output_str.c_str());

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
        
        response = {"00000000-0000-4000-0000-000000000000", ModServerRequestType::error, true};
        if (!response_json.IsObject()) {
            LOG(LogLevel::Error, "Could not load response: root is not an object.");
            stop_loop = true;
        } else if (
            !response_json.HasMember("id") ||
            !response_json.HasMember("type") ||
            !response_json.HasMember("origin") ||
            !response_json.HasMember("data")
        ) {
            LOG(LogLevel::Error, "Could not load response: is missing one or more of: [\"id\", \"type\", \"origin\", \"data\"].");
            stop_loop = true;
        } else if (
            !response_json["id"].IsString() ||
            !response_json["type"].IsString() ||
            !response_json["origin"].IsString() ||
            !response_json["data"].IsObject()
        ) {
            LOG(LogLevel::Error, "Could not load response: one or more keys are of incorrect type: [\"id\": str, \"type\": str, \"origin\": str, \"data\": dict].");
            stop_loop = true;
        } else {
            response.uuid = response_json["id"].GetString();
            response.type = string_to_type(std::string(response_json["type"].GetString()));
            response.origin = (std::string(response_json["origin"].GetString()) == "TDModServer");
            response.data.CopyFrom(response_json["data"], response_json.GetAllocator());
            if (response.type == ModServerRequestType::execute) {
                handle_function_request(response);
            } else {
                stop_loop = true;
            }
        }
    }
    return response;
}

// API //
ModServerRequest ModServer::status() {
    ModServerRequest request = {uuid4(), ModServerRequestType::status};
    send_request(request);
    return get_response();
}

ModServerRequest ModServer::execute(const std::string& function, rapidjson::Document& args) {
    ModServerRequest request = {uuid4(), ModServerRequestType::execute};
    rapidjson::Value json_function;
    rapidjson::Value json_args;

    rapidjson::Document::AllocatorType& allocator = request.data.GetAllocator();

    request.data.SetObject();
    json_function.SetString(function.c_str(), allocator);
    json_args.CopyFrom(args, allocator);

    request.data.AddMember("function", json_function, allocator);
    request.data.AddMember("args", json_args, allocator);
    
    send_request(request);
    return get_response();
}

ModServerRequest ModServer::load_mod(const std::filesystem::path& path) {
    ModServerRequest request = {uuid4(), ModServerRequestType::load_mod};
    rapidjson::Value json_path;

    request.data.SetObject();
    json_path.SetString(path.u8string().c_str(), request.data.GetAllocator());

    request.data.AddMember("path", json_path, request.data.GetAllocator());
    
    send_request(request);
    return get_response();
}
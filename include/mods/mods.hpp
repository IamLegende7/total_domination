#ifndef MODS_HPP
#define MODS_HPP

#include <SDL3/SDL_process.h>
#include <SDL3/SDL_iostream.h>
#include "rapidjson/rapidjson.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <filesystem>

struct ModServerResponse {
    int status;
    std::string message;
    std::string sender;
    rapidjson::Value data = rapidjson::Value();
};

struct ModServerRequest {
    std::string type;
    rapidjson::Document data = rapidjson::Document();
};

namespace ModServerFunctions {
    void log(rapidjson::Value& args,rapidjson::Document& output);
    void get_resource(rapidjson::Value& args, rapidjson::Document& output);
    void set_resource(rapidjson::Value& args, rapidjson::Document& output);
    void get_pos(rapidjson::Value& args, rapidjson::Document& output);
    void get_owner(rapidjson::Value& args, rapidjson::Document& output);
    void get_faction(rapidjson::Value& args, rapidjson::Document& output);
    void delete_actor(rapidjson::Value& args, rapidjson::Document& output);
    void spawn_actor(rapidjson::Value& args, rapidjson::Document& output);
    void registry_add(rapidjson::Value& args, rapidjson::Document& output);
    void registry_load(rapidjson::Value& args, rapidjson::Document& output);
    void registry_get(rapidjson::Value& args, rapidjson::Document& output);

    inline std::unordered_map<std::string, std::function<void(rapidjson::Value&,rapidjson::Document&)>> functions = {
        {"LOG", ModServerFunctions::log},
        {"get_resource", ModServerFunctions::get_resource},
        {"set_resource", ModServerFunctions::set_resource},
        {"get_pos", ModServerFunctions::get_pos},
        {"get_owner", ModServerFunctions::get_owner},
        {"get_faction", ModServerFunctions::get_faction},
        {"spawn_actor", ModServerFunctions::spawn_actor},
        {"delete_actor", ModServerFunctions::delete_actor},
        {"registry_add", ModServerFunctions::registry_add},
        {"registry_load", ModServerFunctions::registry_load},
        {"registry_get", ModServerFunctions::registry_get}
    };
};

class ModServer {
    private:
        SDL_Process* process;
        SDL_IOStream* output;
        SDL_IOStream* input;
    public:
        ModServer();
        ~ModServer();

        // Calls //
        void make_request(ModServerRequest& request);
        void handle_function_request(ModServerResponse& function_request);
        ModServerResponse get_response();

        // API //
        ModServerResponse status();
        ModServerResponse execute(const std::string& mod, const std::string& function, rapidjson::Document& args);
        ModServerResponse load_mod(const std::filesystem::path& path);
};

inline ModServer* MOD_SERVER = nullptr;

#endif
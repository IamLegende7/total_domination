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

    inline std::unordered_map<std::string, std::function<void(rapidjson::Value&,rapidjson::Document&)>> functions = {
        {"LOG", ModServerFunctions::log}
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
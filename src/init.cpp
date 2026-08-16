#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3/SDL_stdinc.h>

#include <string>
#include <filesystem>

#include "callback_functions.hpp"

#include "main.hpp"
#include "map.hpp"
#include "inputs/inputs.hpp"
#include "renderring/render_agents.hpp"
#include "renderring/textures.hpp"
#include "utils/logger.hpp"

#include "settings/info.h"
#include "settings/locations.hpp"
#include "settings/debug.hpp"
#include "settings/main.hpp"
#include "settings/render.hpp"
#include "renderring/shaders.hpp"

bool load_settings() {
    init_locations_settings(std::filesystem::path(SDL_GetCurrentDirectory()) / std::filesystem::path("data/config/locations.ini")); /* Not really ideal */
    init_debug_settings(LOCATIONS["config_dir"].get<std::filesystem::path>() / "debug.ini");
    init_main_settings(LOCATIONS["config_dir"].get<std::filesystem::path>() / "main.ini");
    init_render_settings(LOCATIONS["config_dir"].get<std::filesystem::path>() / "render.ini");
    return true;
}

SDL_AppResult init(void** appState, int argc, char** argv) {
    // APP METADATA //
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, INFO_NAME.c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, INFO_VERSION.toString().c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, INFO_DOMAIN.c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, INFO_AUTHOR.c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, INFO_COPYRIGHT.c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, INFO_URL.c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

    // SETTINGS //
    LOG(LogLevel::Info, "Loading Settings");
    load_settings();

    // LOGGER //
    LOGGER.set_logfile(LOCATIONS["log_file"].get<std::filesystem::path>()); // Logger function has its own error handling

    // SDL //
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG(LogLevel::Critical, "SDL could not initialize: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG(LogLevel::Info, "SDL initialized successfully.");
    }

    // SDL_ttf init //
    if (!TTF_Init()) {
        LOG(LogLevel::Critical, "SDL_ttf could not initialize: %s", SDL_GetError());
        STATUS_TTF_LOADED = false;
    }

    // SDL_shadercross init //
    if (RENDER_SETTINGS["online_shaders"].get<bool>()) {
        LOG(LogLevel::Info, "Initializing SDL_shadercross..");
        if (!SDL_ShaderCross_Init()) {
            LOG(LogLevel::Error, "SDL_ShaderCross_Init failed: %s", SDL_GetError());
            LOG(LogLevel::Warning, "Online shader compilation disabled.");
            STATUS_SHADERCROSS_LOADED = false;
        } else {
            STATUS_SHADERCROSS_LOADED = true;
        }
    }


    // WINDOW //
    std::string windowTitle = INFO_NAME + " - " + INFO_VERSION.toString();
    WINDOW = SDL_CreateWindow(windowTitle.c_str(), SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (WINDOW == NULL) {
        LOG(LogLevel::Critical, "Window could not be created: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if ((RENDER_SETTINGS["render_mode"].get<int>() > 2) || (RENDER_SETTINGS["render_mode"].get<int>() < 1)) {
        LOG(LogLevel::Critical, "Invalid render_mode \"%d\".", RENDER_SETTINGS["render_mode"].get<int>());
        return SDL_APP_FAILURE;
    }

    if (RENDER_SETTINGS["render_mode"].get<int>() == 1) {
        // RENDERER //
        RENDERER = SDL_CreateRenderer(WINDOW, SDL_GPU_RENDERER);
        if (RENDERER == NULL) {
            LOG(LogLevel::Error, "Renderer could not be created: %s", SDL_GetError());
            LOG(LogLevel::Warning, "Falling back to software renderer..");
            STATUS_FORCED_RENDERMODE = 2;
            RENDER_SETTINGS["render_mode"].set<int>(2);
        }

        // VSYNC //
        if (RENDER_SETTINGS["vsync"].get<bool>()) {
            if (!SDL_SetRenderVSync(RENDERER, 1)) {
                LOG(LogLevel::Error, "VSync could not be enabled: %s", SDL_GetError());
            }
        }
    }
    if (RENDER_SETTINGS["render_mode"].get<int>() == 2) { // TODO
        LOG(LogLevel::Critical, "Software Renderer not implemented!");
        return SDL_APP_FAILURE;
    }

    // SOFTWARE RENDERRING WARNING //
    if (RENDER_SETTINGS["render_mode"].get<int>() == 2) {
        LOG(LogLevel::Warning, "Using software renderring");
    }

    // Render Agents //
    MAIN_RENDER_AGENT = new RenderAgent(RENDERER);
    UI_RENDER_AGENT = new RenderAgent(RENDERER, true);

    // Window icon //
    std::filesystem::path icon_path = LOCATIONS["resource_dir"].get<std::filesystem::path>() / "icons/icon.png";
    const std::string icon_path_str = icon_path.u8string();
    LOG(LogLevel::Debug, "icon_path: \"%s\"", icon_path_str.c_str());
    SDL_Surface* icon = IMG_Load(icon_path_str.c_str());
    if ( !SDL_SetWindowIcon(WINDOW, icon) ) {
        LOG(LogLevel::Warning, "Windowicon could not be set: %s", SDL_GetError());
    }

    // Inputs //
    INPUTS = new InputHandler();

    // Shader loading //
    CURRENT_RENDER_STATE = set_render_state(RENDERER, RENDER_SETTINGS["shader"].get<std::filesystem::path>(), RENDER_STATES);

    // TTF font loading //
    UI_RENDER_AGENT->add_font("def_font", SETTINGS["font"].get<std::filesystem::path>(), (float)(SETTINGS["font_size"].get<int>()/10));

    // Seeding SDL_rand
    SDL_srand(0);

    LOG(LogLevel::Info, "Setup all done!");

    // TESTS //
    LOG(LogLevel::Info, "Running tests..");
    if (DEBUG["test_logger"].get<bool>()) {
        LOG(LogLevel::Debug,    "Testing Logger");
        LOG(LogLevel::Info,     "Testing Logger");
        LOG(LogLevel::Warning,  "Testing Logger");
        LOG(LogLevel::Error,    "Testing Logger");
        LOG(LogLevel::Critical, "Testing Logger");
    }

    if (DEBUG["print_locations"].get<bool>()) {
        for (auto& [key, setting] : LOCATIONS) {
            std::string current_value_str = setting.get<std::filesystem::path>().u8string();
            LOG(LogLevel::Debug, "%s: \"%s\"", key.c_str(), current_value_str.c_str());
        }
    }

    LOG(LogLevel::Info, "All good; have fun!");
    return SDL_APP_CONTINUE;
}



std::filesystem::path crash_handler() {
    SDL_Time* current_time = new SDL_Time();
    if (!SDL_GetCurrentTime(current_time)) {
        LOG(LogLevel::Error, "Could not get local time: %s", SDL_GetError());
    }
    SDL_DateTime* current_date_time = new SDL_DateTime();
    if (!SDL_TimeToDateTime(*current_time, current_date_time, true)) {
        LOG(LogLevel::Error, "Could not convert time to date time: %s", SDL_GetError());
    }

    std::ostringstream crash_file_name_stream;
        crash_file_name_stream << "crash-" << current_date_time->day << "." << current_date_time->month << "." << current_date_time->year << "-" << current_date_time->hour << ":" << current_date_time->minute << ".log";
    std::string crash_file_name = crash_file_name_stream.str();
    delete current_time;
    delete current_date_time;

    std::string log_crash_dir_str = LOCATIONS["log_crash_dir"].get<std::filesystem::path>().u8string();
    if (!SDL_GetPathInfo(log_crash_dir_str.c_str(), NULL)) {
        if (!SDL_CreateDirectory(log_crash_dir_str.c_str())) {
            LOG(LogLevel::Error, "Could not create the crash-log directory %s: %s", log_crash_dir_str.c_str(), SDL_GetError());
            return LOCATIONS["log_file"].get<std::filesystem::path>();
        }
    }
    std::filesystem::path new_file_path = LOCATIONS["log_crash_dir"].get<std::filesystem::path>() / crash_file_name;
    std::string new_file_path_str = new_file_path.u8string();
    std::string log_file_path_str = LOCATIONS["log_file"].get<std::filesystem::path>().u8string();
    if (!SDL_CopyFile(log_file_path_str.c_str(), new_file_path_str.c_str())) {
        LOG(LogLevel::Error, "Could not copy %s to %s: %s", log_file_path_str.c_str(), new_file_path_str.c_str(), SDL_GetError());
        return log_file_path_str;
    }
    SDL_RemovePath(log_file_path_str.c_str());

    return new_file_path;
}

void quit(void *appstate, SDL_AppResult result) {
    if (result == SDL_APP_FAILURE) {
        std::filesystem::path crash_log_file = crash_handler();
        const std::string crash_log_file_str = crash_log_file.u8string();
        const std::string crash_message = "It seems this application has crashed! See \""+crash_log_file_str+"\" for more details";
        LOG(LogLevel::Critical, "%s", crash_message.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "TD Chrash-Handler", crash_message.c_str(), NULL);
    }

    LOG(LogLevel::Info, "Cleaning up");

    delete MAIN_MAP;
    delete MAIN_RENDER_AGENT;
    delete INPUTS;

    if (RENDERER != NULL) {
        SDL_DestroyRenderer(RENDERER);
    }
    if (WINDOW != NULL) {
        SDL_DestroyWindow(WINDOW);
    }

    if (STATUS_SHADERCROSS_LOADED) {
        SDL_ShaderCross_Quit();
    }
    if (STATUS_TTF_LOADED) {
        TTF_Quit();
    }
    SDL_Quit();
}


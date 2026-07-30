#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <string>

#include "callback_functions.hpp"

#include "main.hpp"
#include "map.hpp"
#include "inputs.hpp"
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
    init_locations_settings("data/config/locations.ini"); /* Not really ideal */
    init_debug_settings(std::string(LOCATIONS["config_dir"]) + "/debug.ini");
    init_main_settings(std::string(LOCATIONS["config_dir"]) + "/main.ini");
    init_render_settings(std::string(LOCATIONS["config_dir"]) + "/render.ini");
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
    LOG(LogLevel::INFO, "Loading Settings");
    load_settings();

    // LOGGER //
    LOGGER.set_logfile(LOCATIONS["log_file"]); // Logger function has its own error handling

    // SDL //
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG(LogLevel::CRITICAL, "SDL could not initialize: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG(LogLevel::INFO, "SDL initialized successfully.");
    }

    // SDL_ttf init //
    if (!TTF_Init()) {
        LOG(LogLevel::CRITICAL, "SDL_ttf could not initialize: %s", SDL_GetError());
        STATUS_TTF_LOADED = false;
    }

    // SDL_shadercross init //
    if (RENDER_SETTINGS["online_shaders"]) {
        LOG(LogLevel::INFO, "Initializing SDL_shadercross..");
        if (!SDL_ShaderCross_Init()) {
            LOG(LogLevel::ERROR, "SDL_ShaderCross_Init failed: %s", SDL_GetError());
            LOG(LogLevel::WARNING, "Online shader compilation disabled.");
            STATUS_SHADERCROSS_LOADED = false;
        } else {
            STATUS_SHADERCROSS_LOADED = true;
        }
    }


    // WINDOW //
    std::string windowTitle = INFO_NAME + " - " + INFO_VERSION.toString();
    WINDOW = SDL_CreateWindow(windowTitle.c_str(), SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (WINDOW == NULL) {
        LOG(LogLevel::CRITICAL, "Window could not be created: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if ((int(RENDER_SETTINGS["render_mode"]) > 2) || (int(RENDER_SETTINGS["render_mode"]) < 1)) {
        LOG(LogLevel::CRITICAL, "Invalid render_mode \"%d\".", int(RENDER_SETTINGS["render_mode"]));
        return SDL_APP_FAILURE;
    }

    if (int(RENDER_SETTINGS["render_mode"]) == 1) {
        // RENDERER //
        RENDERER = SDL_CreateRenderer(WINDOW, SDL_GPU_RENDERER);
        if (RENDERER == NULL) {
            LOG(LogLevel::ERROR, "Renderer could not be created: %s", SDL_GetError());
            LOG(LogLevel::WARNING, "Falling back to software renderer..");
            STATUS_FORCED_RENDERMODE = 2;
            RENDER_SETTINGS["render_mode"].set(2);
        }

        // VSYNC //
        if (!SDL_SetRenderVSync(RENDERER, 1)) {
            LOG(LogLevel::ERROR, "VSync could not be enabled: %s", SDL_GetError());
        }
    }
    if (int(RENDER_SETTINGS["render_mode"]) == 2) { // TODO
        LOG(LogLevel::CRITICAL, "Software Renderer not implemented!");
        return SDL_APP_FAILURE;
    }

    // SOFTWARE RENDERRING WARNING //
    if (int(RENDER_SETTINGS["render_mode"]) == 2) {
        LOG(LogLevel::WARNING, "Using software renderring");
    }

    // Render Agents //
    MAIN_RENDER_AGENT = new RenderAgent();

    // Window icon //
    std::string icon_path = std::string(LOCATIONS["resource_dir"]) + "/icons/icon.png";
    SDL_Surface* icon = IMG_Load(icon_path.c_str());
    if ( !SDL_SetWindowIcon(WINDOW, icon) ) {
        LOG(LogLevel::WARNING, "Windowicon could not be set: %s", SDL_GetError());
    }

    // Inputs //
    INPUTS = new InputHandler();

    // Shader loading //
    CURRENT_RENDER_STATE = set_render_state(RENDERER, RENDER_SETTINGS["shader"].get_str(), RENDER_STATES);

    LOG(LogLevel::INFO, "Setup all done!");

    // TESTS //
    LOG(LogLevel::INFO, "Running tests..");
    if (DEBUG["test_logger"]) {
        LOG(LogLevel::DEBUG,    "Testing Logger");
        LOG(LogLevel::INFO,     "Testing Logger");
        LOG(LogLevel::WARNING,  "Testing Logger");
        LOG(LogLevel::ERROR,    "Testing Logger");
        LOG(LogLevel::CRITICAL, "Testing Logger");
    }

    // Test.png //
    //LOG(LogLevel::DEBUG, "Loading test.png...");
    //MAIN_RENDER_AGENT->add_texture("test_texture", LOCATIONS["texture_dir"].get_str()+"/test.png");
    //MAIN_RENDER_AGENT->add_sprite("test_sprite", "test_texture", 0, 0, 32, 32);
    //MAIN_RENDER_AGENT->add_entity("test_entity", "test_sprite", 0, 0, 4);

    LOG(LogLevel::INFO, "All good; have fun!");
    return SDL_APP_CONTINUE;
}



std::string crash_handler() {
    SDL_Time* current_time = new SDL_Time();
    if (!SDL_GetCurrentTime(current_time)) {
        LOG(LogLevel::ERROR, "Could not get local time: %s", SDL_GetError());
    }
    SDL_DateTime* current_date_time = new SDL_DateTime();
    if (!SDL_TimeToDateTime(*current_time, current_date_time, true)) {
        LOG(LogLevel::ERROR, "Could not convert time to date time: %s", SDL_GetError());
    }

    std::ostringstream crash_file_name_stream;
        crash_file_name_stream << "crash-" << current_date_time->day << "." << current_date_time->month << "." << current_date_time->year << "-" << current_date_time->hour << ":" << current_date_time->minute << ".log";
    std::string crash_file_name = crash_file_name_stream.str();
    delete current_time;
    delete current_date_time;

    if (!SDL_GetPathInfo(LOCATIONS["log_crash_dir"].get_c_str(), NULL)) {
        if (!SDL_CreateDirectory(LOCATIONS["log_crash_dir"].get_c_str())) {
            LOG(LogLevel::ERROR, "Could not create the crash-log directory %s: %s", LOCATIONS["log_crash_dir"].get_c_str(), SDL_GetError());
            return LOCATIONS["log_file"].get_str();
        }
    }
    std::string new_file_path = LOCATIONS["log_crash_dir"].get_str() + "/" + crash_file_name;
    if (!SDL_CopyFile(LOCATIONS["log_file"].get_c_str(), new_file_path.c_str())) {
        LOG(LogLevel::ERROR, "Could not copy %s to %s: %s", LOCATIONS["log_file"].get_c_str(), new_file_path.c_str(), SDL_GetError());
        return LOCATIONS["log_file"].get_str();
    }
    SDL_RemovePath(LOCATIONS["log_file"].get_c_str());

    return new_file_path;
}

void quit(void *appstate, SDL_AppResult result) {
    if (result == SDL_APP_FAILURE) {
        std::string crash_log_file = crash_handler();
        const std::string crash_message = "It seems this application has crashed! See \""+crash_log_file+"\" for more details";
        LOG(LogLevel::CRITICAL, "%s", crash_message.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "TD Chrash-Handler", crash_message.c_str(), NULL);
    }

    LOG(LogLevel::INFO, "Cleaning up");

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


#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>

#include "main.hpp"
#include "renderring/render_agents.hpp"
#include "renderring/textures.hpp"
#include "utils/logger.hpp"

#include "settings/info.h"
#include "settings/locations.hpp"
#include "settings/debug.hpp"
#include "settings/main.hpp"
#include "settings/render.hpp"

float accumulator = 0.0f;
Uint64 previous;

// ██╗████████╗███████╗██████╗  █████╗ ████████╗███████╗
// ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██╔════╝
// ██║   ██║   █████╗  ██████╔╝███████║   ██║   █████╗  
// ██║   ██║   ██╔══╝  ██╔══██╗██╔══██║   ██║   ██╔══╝  
// ██║   ██║   ███████╗██║  ██║██║  ██║   ██║   ███████╗
// ╚═╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚══════╝
SDL_AppResult SDL_AppIterate(void* appState) {
    Uint64 current = SDL_GetTicks();
    float elapsed = (current - previous) / 1000.0f;
    previous = current;
    accumulator += elapsed;

    if (MODE == 0) {
        //TextureConstructor* terrain_tiles[2];
        //    terrain_tiles[0] = new TextureConstructor(LOCATIONS["texture_dir"].get_str()+"/tiles/dirt.png", 0, 0);
        //    terrain_tiles[1] = new TextureConstructor(LOCATIONS["texture_dir"].get_str()+"/tiles/stone.png", 32, 0);
        //MAIN_RENDER_AGENT->insert_texture("terrain_atlas", MAIN_RENDER_AGENT->bake_texture_from_file(terrain_tiles, (sizeof(terrain_tiles) / sizeof(terrain_tiles[0]))));
        //MAIN_RENDER_AGENT->add_sprite("dirt", "terrain_atlas", 0, 0, 32, 37);
        //MAIN_RENDER_AGENT->add_sprite("stone", "terrain_atlas", 32, 0, 32, 37);

        std::string texture_names[3] = {
            "td:dirt",
            "td:stone",
            "td:dry_dirt"
        };

        bake_atlas(MAIN_RENDER_AGENT, "terrain_atlas", texture_names, (sizeof(texture_names) / sizeof(texture_names[0])));

        LOG(LogLevel::DEBUG, "What is the Problem?");
        MAIN_RENDER_AGENT->add_sprite("terrain_atlas", "terrain_atlas", NULL, NULL, NULL, NULL);
        MAIN_RENDER_AGENT->add_entity("terrain_entity", "terrain_atlas", 0, 0, 4);
        MAIN_RENDER_AGENT->add_entity("tile1", "td:dirt", (int)SCREEN_WIDTH/10, (int)SCREEN_HEIGHT/10, 4);
        MAIN_RENDER_AGENT->add_entity("tile2", "td:stone", (int)SCREEN_WIDTH/10+16, (int)SCREEN_HEIGHT/10+11, 4);
        LOG(LogLevel::DEBUG, "Hewwo?");
        MODE = 1;

    } else if (MODE == 1) {
        if (DIRTY_SCREEN) {
            MAIN_RENDER_AGENT->render();
            DIRTY_SCREEN = false;
        }
    }

    float sleepTime = (1.0f / TARGET_FPS) - accumulator;
    if (sleepTime > 0) {
        SDL_Delay((int)(sleepTime * 1000));
    }

    return SDL_APP_CONTINUE;
}


// ███████╗██╗   ██╗███████╗███╗   ██╗████████╗
// ██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝
// █████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   
// ██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   
// ███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   
// ╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   
SDL_AppResult SDL_AppEvent(void* appState, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        SCREEN_WIDTH = event->window.data1;
        SCREEN_HEIGHT = event->window.data2;
        if (int(RENDER_SETTINGS["render_mode"]) == 1) SDL_SetRenderViewport(RENDERER, NULL);
        //DIRTY_SCREEN = true;
    }

    return SDL_APP_CONTINUE;
}


// ██╗███╗   ██╗██╗████████╗
// ██║████╗  ██║██║╚══██╔══╝
// ██║██╔██╗ ██║██║   ██║   
// ██║██║╚██╗██║██║   ██║   
// ██║██║ ╚████║██║   ██║   
// ╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝   
SDL_AppResult SDL_AppInit(void** appState, int argc, char** argv) {
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
    init_locations_settings("data/config/locations.ini"); /* Not really ideal */
    init_debug_settings(std::string(LOCATIONS["config_dir"]) + "/debug.ini");
    init_main_settings(std::string(LOCATIONS["config_dir"]) + "/main.ini");
    init_render_settings(std::string(LOCATIONS["config_dir"]) + "/render.ini");

    // LOGGER //
    LOGGER.set_logfile(LOCATIONS["log_file"]); // Logger function has its own error handeling

    // SDL //
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG(LogLevel::CRITICAL, "SDL could not initialize: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    } else {
        LOG(LogLevel::INFO, "SDL initialized successfully.");
    }

    // GPU DEVICE //
    if (int(RENDER_SETTINGS["render_mode"]) == 0) {
        GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, std::string(RENDER_SETTINGS["gpu_driver"]).c_str());
        if (GPU == NULL) {
            LOG(LogLevel::ERROR, "Could not create GPU device: %s", SDL_GetError());
            LOG(LogLevel::WARNING, "Falling back to renderer..");
            RENDER_SETTINGS["render_mode"].set(1);
        } else {
            LOG(LogLevel::INFO, "Using GPU driver %s", SDL_GetGPUDeviceDriver(GPU));
        }
    }

    // SOFTWARE RENDERRING WARNING //
    if (int(RENDER_SETTINGS["render_mode"]) == 2) {
        LOG(LogLevel::WARNING, "Using software renderring");
    }

    // WINDOW //
    std::string windowTitle = INFO_NAME + " - " + INFO_VERSION.toString();
    WINDOW = SDL_CreateWindow(windowTitle.c_str(), SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (WINDOW == NULL) {
        LOG(LogLevel::CRITICAL, "Window could not be created: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // CLAIM WINDOW //
    if ((int(RENDER_SETTINGS["render_mode"]) > 2) || (int(RENDER_SETTINGS["render_mode"]) < 0)) return SDL_APP_FAILURE;
    if (int(RENDER_SETTINGS["render_mode"]) == 0) {
        if (!SDL_ClaimWindowForGPUDevice(GPU, WINDOW)) {
            LOG(LogLevel::ERROR, "Could not claim Main Window for GPU: %s", SDL_GetError());
            RENDER_SETTINGS["render_mode"].set(1);
            LOG(LogLevel::WARNING, "Using software renderring");
            // GPU device cleanup
            SDL_DestroyGPUDevice(GPU);
        }

        // VSYNC (GPU) //
        // https://discourse.libsdl.org/t/how-to-enable-disable-vsync-with-sdl3-gpu-no-sdl-renderer/61735/2
        bool supports_mailbox = SDL_WindowSupportsGPUPresentMode(GPU, WINDOW, SDL_GPU_PRESENTMODE_MAILBOX);
        SDL_SetGPUSwapchainParameters(GPU, WINDOW, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, supports_mailbox ? SDL_GPU_PRESENTMODE_MAILBOX : SDL_GPU_PRESENTMODE_VSYNC);
    }

    if (int(RENDER_SETTINGS["render_mode"]) == 1) {
        // RENDERER //
        RENDERER = SDL_CreateRenderer(WINDOW, NULL);
        if (RENDERER == NULL) {
            LOG(LogLevel::ERROR, "Renderrer could not be created: %s", SDL_GetError());
            LOG(LogLevel::WARNING, "Falling back to software renderer..");
            RENDER_SETTINGS["render_mode"].set(2);
        }

        // VSYNC //
        if (!SDL_SetRenderVSync(RENDERER, 1)) {
            LOG(LogLevel::ERROR, "VSync could not be enabled: %s", SDL_GetError());
        }
    } else if (int(RENDER_SETTINGS["render_mode"]) == 2) {
        SDL_Surface* window_surface = SDL_GetWindowSurface(WINDOW);
        RENDERER = SDL_CreateSoftwareRenderer(window_surface);
        if (RENDERER == NULL) {
            LOG(LogLevel::ERROR, "Software renderer could not be created: %s", SDL_GetError());
            LOG(LogLevel::CRITICAL, "Nothing to fall back to.. aboarding!");
            return SDL_APP_FAILURE;
        }
    }

    // Render Agents //
    MAIN_RENDER_AGENT = new RenderAgent();

    // Test.png //
    LOG(LogLevel::DEBUG, "Loading Test.png...");
    MAIN_RENDER_AGENT->add_texture("test_texture", LOCATIONS["texture_dir"].get_str()+"/Test.png");
    MAIN_RENDER_AGENT->add_sprite("test_sprite", "test_texture", 0, 0, 32, 32);
    //MAIN_RENDER_AGENT->add_entity("test_entity", "test_sprite", 0, 0, 4);

    std::string icon_path = std::string(LOCATIONS["resource_dir"]) + "/icons/icon.png";
    SDL_Surface* icon = IMG_Load(icon_path.c_str());
    if ( !SDL_SetWindowIcon(WINDOW, icon) ) {
        LOG(LogLevel::WARNING, "Windowicon could not be set: %s", SDL_GetError());
    }
    LOG(LogLevel::INFO, "Setup all done!");

    // Frame Limiting //
    previous = SDL_GetTicks();

    // TESTS //
    if (DEBUG["test_logger"]) {
        LOG(LogLevel::DEBUG,    "Testing Logger");
        LOG(LogLevel::INFO,     "Testing Logger");
        LOG(LogLevel::WARNING,  "Testing Logger");
        LOG(LogLevel::ERROR,    "Testing Logger");
        LOG(LogLevel::CRITICAL, "Testing Logger");
    }
    return SDL_APP_CONTINUE;
}


//  ██████╗ ██╗   ██╗██╗████████╗
// ██╔═══██╗██║   ██║██║╚══██╔══╝
// ██║   ██║██║   ██║██║   ██║   
// ██║▄▄ ██║██║   ██║██║   ██║   
// ╚██████╔╝╚██████╔╝██║   ██║   
//  ╚══▀▀═╝  ╚═════╝ ╚═╝   ╚═╝   
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

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if (result == SDL_APP_FAILURE) {
        std::string crash_log_file = crash_handler();
        LOG(LogLevel::CRITICAL, "It seems this application has crashed! See %s for more details", crash_log_file.c_str());
    }

    delete MAIN_RENDER_AGENT;
    LOG(LogLevel::INFO, "Cleaning up");
    //SDL_DestroyGPUDevice(GPU);
    //SDL_DestroyRenderer(RENDERER);
    //SDL_DestroyWindow(WINDOW);
    if (GPU != NULL) {
        if (WINDOW != NULL) {
            SDL_ReleaseWindowFromGPUDevice(GPU, WINDOW);
            SDL_DestroyWindow(WINDOW);
        }

        SDL_DestroyGPUDevice(GPU);
    }

    SDL_Quit();
}


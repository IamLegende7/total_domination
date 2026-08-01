#include <SDL3/SDL.h>  
#include <SDL3_shadercross/SDL_shadercross.h>
#include <string>
#include <vector>
#include <tuple>
#include <filesystem>
#include "rapidjson/document.h"

#include "renderring/shaders.hpp"
#include "settings/render.hpp"
#include "status.hpp"
#include "main.hpp"
#include "utils/json.hpp"
#include "utils/logger.hpp"

// Local
std::tuple<std::string, std::string> get_compiled_path(const std::string& hlsl_path, const SDL_GPUShaderFormat format) {
    const std::string initial_suffix = ".hlsl";
    std::string compiled_suffix;
    std::string compiled_dir;
    if (format == SDL_GPU_SHADERFORMAT_SPIRV) {
        compiled_suffix = ".spv";
        compiled_dir = "SPIRV";
    } else if (format == SDL_GPU_SHADERFORMAT_DXIL) {
        compiled_suffix = ".dxil";
        compiled_dir = "DXIL";
    } else if (format == SDL_GPU_SHADERFORMAT_MSL) {
        compiled_suffix = ".msl";
        compiled_dir = "MSL";
    }

    std::filesystem::path p(hlsl_path);
    if (p.extension() != ".hlsl") {
        LOG(LogLevel::WARNING, "Path \"%s\" does not point to a .hlsl file.", hlsl_path.c_str());
        return {"", ""};
    }
    std::string base = p.stem().string();
    return {"bin/shaders/"+compiled_dir+"/"+base+compiled_suffix, "bin/shaders/reflection_info/"+base+".json"};
}

SDL_GPUShader* load_shader(SDL_GPUDevice *device, const std::string& filename, SDL_ShaderCross_ShaderStage stage) {
    LOG(LogLevel::INFO, "Loading shader \"%s\"..", filename.c_str());
    SDL_GPUShader* shader;
    if (RENDER_SETTINGS["online_shaders"]) { 
        if (!STATUS_SHADERCROSS_LOADED) {
            LOG(LogLevel::WARNING, "Could not load shader %s: SDL_shadercross not loaded.", filename.c_str());
            return nullptr;
        }

        size_t sz = 0;
        char *source = (char *)SDL_LoadFile(filename.c_str(), &sz);
        if (source == nullptr) {
            LOG(LogLevel::ERROR, "Could not load shader source from file \"%s\": %s", filename.c_str(), SDL_GetError());
            return nullptr;
        }
    
        // 1: HLSL -> SPIRV
        SDL_ShaderCross_HLSL_Info hlsl_info;
        SDL_zero(hlsl_info);
        hlsl_info.source = source;
        hlsl_info.entrypoint = "main";
        hlsl_info.shader_stage = stage;
    
        size_t spirv_size = 0;
        void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &spirv_size);
        SDL_free(source);
        if (!spirv) {
            LOG(LogLevel::ERROR, "Compiling SPIRV from HLSL failed: %s", SDL_GetError());
            return nullptr;
        }
    
        // 2: reflect to get resource_info
        SDL_ShaderCross_GraphicsShaderMetadata *metadata = SDL_ShaderCross_ReflectGraphicsSPIRV((Uint8 *)spirv, spirv_size, 0);
        if (!metadata) {
            LOG(LogLevel::ERROR, "Failed to reflect SPIRV shader: %s", SDL_GetError());
            SDL_free(spirv);
            return nullptr;
        }
    
        // 3: SPIRV -> SDL_GPUShader
        SDL_ShaderCross_SPIRV_Info spirv_info;
        SDL_zero(spirv_info);
        spirv_info.bytecode = (const Uint8 *)spirv;
        spirv_info.bytecode_size = spirv_size;
        spirv_info.entrypoint = "main";
        spirv_info.shader_stage = stage;
    
        shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &spirv_info, &metadata->resource_info, 0);
    
        SDL_free(metadata);
        SDL_free(spirv);

    } else {
        SDL_GPUShaderCreateInfo info;
        SDL_zero(info);
        info.entrypoint = "main";
        info.stage = (SDL_GPUShaderStage)stage;
        
        
        SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
        if (formats == SDL_GPU_SHADERFORMAT_INVALID) {
            LOG(LogLevel::ERROR, "Couldn't get supported shader formats: %s", SDL_GetError());
            return nullptr;
        }
        if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
            info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
            info.format = SDL_GPU_SHADERFORMAT_DXIL;
        } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
            info.format = SDL_GPU_SHADERFORMAT_MSL;
        } else {
            LOG(LogLevel::ERROR, "No supported shader format found");
            return nullptr;
        }
        auto [compiled_shader_file, reflection_info_file] = get_compiled_path(filename, info.format);

        size_t size = 0;
        void *code = SDL_LoadFile(compiled_shader_file.c_str(), &size);
        if (!code) {
            LOG(LogLevel::ERROR, "Could not load compiled shader from file \"%s\": %s", compiled_shader_file.c_str(), SDL_GetError());
            return nullptr;
        }
        info.code = (const Uint8 *)code;
        info.code_size = size;

        rapidjson::Document reflection_info_json = open_json(reflection_info_file);
        if (!reflection_info_json.HasMember("samplers") || !reflection_info_json.HasMember("uniform_buffers")) {
            LOG(LogLevel::ERROR, "Reflection info file \"%s\" holds invalid data: member \"samplers\" and/or \"uniform_buffers\" is missing.", compiled_shader_file.c_str());
            return nullptr;
        }
        info.num_samplers = reflection_info_json["samplers"].GetInt();
        info.num_uniform_buffers = reflection_info_json["uniform_buffers"].GetInt();

        shader = SDL_CreateGPUShader(device, &info);
        SDL_free(code);
    }
    return shader;
}

bool add_renderer_render_state(SDL_Renderer* renderer, const std::string& filename,  std::vector<RenderState>& render_states) {
    SDL_GPUDevice* device = SDL_GetGPURendererDevice(renderer);
    if (!device) { // TODO: Better logging
        LOG(LogLevel::ERROR, "Couldn't get GPU device: %s", SDL_GetError());
        return false;
    }

    render_states.push_back(RenderState());
    render_states.back().name = filename;
    render_states.back().shader = load_shader(device, filename, SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    if (render_states.back().shader == nullptr) {
        LOG(LogLevel::WARNING, "Couldn't create shader: %s", SDL_GetError());
        return false;
    }

    SDL_GPURenderStateCreateInfo createinfo;
    SDL_zero(createinfo);
    createinfo.fragment_shader = render_states.back().shader;
    render_states.back().state = SDL_CreateGPURenderState(renderer, &createinfo);
    if (!render_states.back().state) {
        LOG(LogLevel::WARNING, "Couldn't create render state: %s", SDL_GetError());
        return false;
    }

    if (filename == "resources/shaders/basic_CRT.frag.hlsl") { // TODO: Make automatic somehow
        CRTEffectUniforms uniforms;
        SDL_zero(uniforms);
        uniforms.texture_width = SCREEN_WIDTH;
        uniforms.texture_height = SCREEN_HEIGHT;
        if (!SDL_SetGPURenderStateFragmentUniforms(render_states.back().state, 0, &uniforms, sizeof(uniforms))) {
            LOG(LogLevel::ERROR, "Couldn't set uniform data: %s", SDL_GetError());
            return false;
        }
    }
    return true;
}

int get_render_state(const std::vector<RenderState>& render_states, const std::string& name) {
    int i = 0;
    for (const RenderState& state : render_states) {
        if (state.name == name) {
            return i;
        }
        i++;
    }
    return -1;
}

int set_render_state(SDL_Renderer* renderer, const std::string& filename, std::vector<RenderState>& render_states) {
    if (filename == "none") {
        return 0;
    }
    const int existing_shader_state_index = get_render_state(render_states, filename);
    if (existing_shader_state_index != -1) {
        //LOG(LogLevel::WARNING, "Tried to add render state \"%s\" but it already exists.", filename);
        return existing_shader_state_index;
    }

    if (!add_renderer_render_state(renderer, filename, render_states)) {
        LOG(LogLevel::WARNING, "Failed to setup render state.");
        return 0;
    }

    return render_states.size()-1;
}
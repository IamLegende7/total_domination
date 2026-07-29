#include <SDL3/SDL.h>  
#include <SDL3_shadercross/SDL_shadercross.h>
#include <string>
#include <vector>
#include "renderring/shaders.hpp"
#include "settings/render.hpp"
#include "status.hpp"
#include "main.hpp"

SDL_GPUShader* load_shader(SDL_GPUDevice *device, const std::string& filename, SDL_ShaderCross_ShaderStage stage) { // TODO: more error logging // TODO: add DXIL & MSL support depending on device support
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
            SDL_Log("CompileSPIRVFromHLSL failed: %s", SDL_GetError());
            return nullptr;
        }
    
        // 2: reflect to get resource_info
        SDL_ShaderCross_GraphicsShaderMetadata *metadata = SDL_ShaderCross_ReflectGraphicsSPIRV((Uint8 *)spirv, spirv_size, 0);
        if (!metadata) {
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
        LOG(LogLevel::WARNING, "Offline shader loading not yet implemented!");
        return nullptr;
    }
    return shader;
}

bool add_renderer_shader_state(SDL_Renderer* renderer, const std::string& filename,  std::vector<RenderState>& out_render_states) {
    SDL_GPUDevice* device = SDL_GetGPURendererDevice(renderer);
    if (!device) { // TODO: Better logging
        SDL_Log("Couldn't get GPU device: %s", SDL_GetError());
        return false;
    }

    out_render_states.push_back(RenderState());
    out_render_states.back().name = filename;
    out_render_states.back().shader = load_shader(device, filename, SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    if (out_render_states.back().shader == nullptr) {
        SDL_Log("Couldn't create shader: %s", SDL_GetError());
        return false;
    }

    SDL_GPURenderStateCreateInfo createinfo;
    SDL_zero(createinfo);
    createinfo.fragment_shader = out_render_states.back().shader;
    out_render_states.back().state = SDL_CreateGPURenderState(renderer, &createinfo);
    if (!out_render_states.back().state) {
        SDL_Log("Couldn't create render state: %s", SDL_GetError());
        return false;
    }

    if (filename == "resources/shaders/basic_CRT.frag.hlsl") { // Make automatic somehow
        CRTEffectUniforms uniforms;
        SDL_zero(uniforms);
        uniforms.texture_width = SCREEN_WIDTH;
        uniforms.texture_height = SCREEN_HEIGHT;
        if (!SDL_SetGPURenderStateFragmentUniforms(out_render_states.back().state, 0, &uniforms, sizeof(uniforms))) {
            SDL_Log("Couldn't set uniform data: %s", SDL_GetError());
            return false;
        }
    }
    return true;
}
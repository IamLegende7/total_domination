#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <vector>

struct CRTEffectUniforms {
    float texture_width;
    float texture_height;
};

struct RenderState {
    std::string name = "NONE";
    SDL_GPUShader* shader = NULL;
    SDL_GPURenderState* state = NULL;
};

SDL_GPUShader* load_shader(SDL_GPUDevice *device, const std::string& filename, SDL_ShaderCross_ShaderStage stage);
bool add_renderer_shader_state(SDL_Renderer* renderer, const std::string& filename,  std::vector<RenderState>& out_render_states);

#endif
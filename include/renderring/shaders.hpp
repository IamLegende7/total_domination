#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include <vector>
#include <filesystem>

struct CRTEffectUniforms {
    float texture_width;
    float texture_height;
};

struct RenderState {
    std::string name = "NONE";
    SDL_GPUShader* shader = NULL;
    SDL_GPURenderState* state = NULL;
};

SDL_GPUShader* load_shader(SDL_GPUDevice *device, const std::filesystem::path& filename, SDL_ShaderCross_ShaderStage stage);
bool add_renderer_render_state(SDL_Renderer* renderer, const std::filesystem::path& filename,  std::vector<RenderState>& render_states);
int get_render_state(const std::vector<RenderState>& render_states, const std::string& name);
int set_render_state(SDL_Renderer* renderer, const std::filesystem::path& filename, std::vector<RenderState>& render_states); // this is the one you should use outside of shaders.cpp, usually

#endif
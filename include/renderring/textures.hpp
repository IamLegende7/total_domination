#ifndef TEXTURES_HPP
#define TEXTURES_HPP

#include <SDL3/SDL.h>
#include <string>
#include "rapidjson/document.h"

#include "renderring/render_agent.hpp"

std::string get_texture_path(const std::string& name);

bool add_texture(RenderAgent* agent, const std::string& texture_name);
bool bake_atlas(RenderAgent* agent, const std::string& atlas_name, const std::string texture_names[], const int& texture_names_length, const bool force_file_loading=false);

#endif
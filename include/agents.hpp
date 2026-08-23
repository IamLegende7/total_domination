#ifndef RENDER_AGENTS_HPP
#define RENDER_AGENTS_HPP

#include <unordered_map>

#include "renderring/render_agent.hpp"
#include "actors.hpp"

inline RenderAgent* MAIN_RENDER_AGENT = nullptr;
inline RenderAgent* UI_RENDER_AGENT = nullptr;

// TODO: put into player class
inline std::unordered_map<int, ActorHandler*> ACTOR_HANDLERS = {};

#endif
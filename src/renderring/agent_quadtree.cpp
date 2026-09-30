#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "renderring/render_agent.hpp"
#include "utils/logger.hpp"

RenderAgentQuadtreeNode::~RenderAgentQuadtreeNode() {
    if (children[0]) {
        for (int i = 0; i < 4; ++i) {
            delete children[i];
            children[i] = nullptr;
        }
    }
}

void RenderAgentQuadtreeNode::set_dimensions(int x, int y, int width, int height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
}

void RenderAgentQuadtreeNode::set_capacity(const int node_capacity) {
    this->node_capacity = node_capacity;
}

bool RenderAgentQuadtreeNode::subdivide() {
    if (node_capacity == 0) {
        LOG(LogLevel::Error, "Node capacity unset!");
        return false;
    }
    if ((children[0] != nullptr) || ((int)contents.size() <= node_capacity))
        return false;
    if ((width < 1) || (height < 1))
        return false;
    
    int child_width  = width / 2;
    int child_height = height / 2;

    int mid_x = x + child_width;
    int mid_y = y + child_height;

    children[0] = new RenderAgentQuadtreeNode(x,     y,     child_width, child_height, node_capacity, depth+1);
    children[1] = new RenderAgentQuadtreeNode(mid_x, y,     width - child_width, child_height, node_capacity, depth+1);
    children[2] = new RenderAgentQuadtreeNode(x,     mid_y, child_width, height - child_height, node_capacity, depth+1);
    children[3] = new RenderAgentQuadtreeNode(mid_x, mid_y, width - child_width, height - child_height, node_capacity, depth+1);

    std::vector<RenderAgentEntity*> old_contents = contents;
    contents.clear();
    for (RenderAgentEntity* current_entry : old_contents) {
        insert(current_entry, true);
    }
    old_contents.clear();

    return true;
}

bool RenderAgentQuadtreeNode::insert(RenderAgentEntity* entity, const bool allow_subdivision) {
    if (!(
        entity->x >= x &&
        entity->y >= y &&
        entity->x+entity->sprite->animations[0].texture_rects[0].w <= x+width &&
        entity->y+entity->sprite->animations[0].texture_rects[0].h <= y+height
    ))
        return false;

    if (children[0] == nullptr) {
        contents.push_back(entity);
        if (allow_subdivision)
            subdivide();
    } else if (entity->movable && (depth == 0)) {
        contents.push_back(entity);
    } else {
        bool taken = false;
        for (int i = 0; i < 4; ++i) {
            if (children[i]->insert(entity, allow_subdivision)) {
                taken = true;
                break;
            }
        }
        if (!taken)
            contents.push_back(entity);
    }
    return true;
}

bool RenderAgentQuadtreeNode::remove(RenderAgentEntity* entity) {
    if (entity == nullptr)
        return false;

    bool removed = false;
    size_t old_size = contents.size();

    contents.erase(
        std::remove(contents.begin(), contents.end(), entity),
        contents.end()
    );
    removed = contents.size() != old_size;

    if (children[0]) {
        for (int i = 0; i < 4; ++i) {
            removed = children[i]->remove(entity) || removed;
        }
    }

    return removed;
}

bool RenderAgentQuadtreeNode::query(const int target_x, const int target_y, const int target_width, const int target_height, std::vector<RenderAgentEntity*>& result) {
    if (!(
        x <= target_x+target_width &&
        x+width >= target_x &&
        y <= target_y+target_height &&
        y+height >= target_y
    ))
        return false;

    for (const auto& current_entry : contents) {
        /*if (current_entry == nullptr ||
            current_entry->sprite == nullptr ||
            current_entry->sprite->animations.empty() ||
            current_entry->sprite->animations[0].texture_rects.empty()) {
            LOG(LogLevel::Error, "Invalid entity in quadtree");
            continue;
        }*/

        if (
            current_entry->x <= target_x+target_width &&
            current_entry->x+current_entry->sprite->animations[0].texture_rects[0].w >= target_x &&
            current_entry->y <= target_y+target_height &&
            current_entry->y+current_entry->sprite->animations[0].texture_rects[0].h >= target_y
        )
            result.push_back(current_entry);
    }
    if (children[0] != nullptr) {
        for (int i = 0; i < 4; ++i) {
            children[i]->query(target_x, target_y, target_width, target_height, result);
        }
    }
    return true;
};

bool RenderAgentQuadtreeNode::render(SDL_Renderer* renderer, const int x_offset, const int y_offset, const int zoom, const int resolution, const SDL_Color& colour) { // TODO: don't render outside of view
    SDL_FRect rect = {
        (float)std::ceil(((x - x_offset) * zoom) / resolution),
        (float)std::ceil(((y - y_offset) * zoom) / resolution),
        (float)std::ceil(width * zoom),
        (float)std::ceil(height * zoom)
    };
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
    SDL_RenderRect(renderer, &rect);

    if (children[0] != nullptr) {
        for (int i = 0; i < 4; ++i) {
            children[i]->render(renderer, x_offset, y_offset, zoom, resolution, colour);
        }
    }
    return true;
};
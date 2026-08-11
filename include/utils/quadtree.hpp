#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>

#include "utils/logger.hpp"


// !! "T" needs to have x, y, width and height attributes !!
template<typename T>
class QuadtreeNode {
    private:
        std::vector<T> contents;
        std::vector<std::string> ids;
        QuadtreeNode* children[4] = {nullptr, nullptr, nullptr, nullptr};

        bool subdivide() {
            if (node_capacity == 0) {
                LOG(LogLevel::Error, "Node capacity unset!");
                return false;
            }
            if ((width < 1) || (height < 1))
                return false;
            //LOG(LogLevel::Debug, "Subdividing");
            int child_width  = width / 2;
            int child_height = height / 2;

            int mid_x = x + child_width;
            int mid_y = y + child_height;

            children[0] = new QuadtreeNode(x,     y,     child_width, child_height, node_capacity, depth+1);
            children[1] = new QuadtreeNode(mid_x, y,     width - child_width, child_height, node_capacity, depth+1);
            children[2] = new QuadtreeNode(x,     mid_y, child_width, height - child_height, node_capacity, depth+1);
            children[3] = new QuadtreeNode(mid_x, mid_y, width - child_width, height - child_height, node_capacity, depth+1);

            std::vector<T> old_contents = contents;
            std::vector<std::string> old_ids = ids;
            contents.clear();
            ids.clear();
            for (size_t i = 0; i < old_contents.size(); ++i) {
                insert(old_ids[i], old_contents[i]);
            }
            old_contents.clear();
            old_ids.clear();

            return true;
        };

    public:
        int x, y;
        int width, height;
        int node_capacity = 0;
        int depth = 0;

        QuadtreeNode(): x(0), y(0), width(0), height(0), node_capacity(0), depth(0) {};
        QuadtreeNode(const int x, const int y, const int width, const int height, const int node_capacity, const int depth=0): x(x), y(y), width(width), height(height), node_capacity(node_capacity), depth(depth) {
            contents.reserve(node_capacity);
            ids.reserve(node_capacity);
        };
        ~QuadtreeNode() {};
        void set_dimensions(int x, int y, int width, int height) {
            this->x = x;
            this->y = y;
            this->width = width;
            this->height = height;
        }
        void set_capacity(const int node_capacity) {
            this->node_capacity = node_capacity;
            contents.reserve(node_capacity);
            ids.reserve(node_capacity);
        };

        bool insert(const std::string& id, T entry) {
            if (depth == 0) {
                if (!(
                    x <= entry.x+entry.width &&
                    x+width >= entry.x &&
                    y <= entry.y+entry.height &&
                    y+height >= entry.y
                ))
                    return false;
            } else if (!(
                x >= entry.x &&
                x+width <= entry.x+entry.width &&
                y >= entry.y &&
                y+height <= entry.y+entry.height
            )) {
                return false;
            }

            if (children[0] == nullptr) {
                contents.push_back(entry);
                ids.push_back(id);

                if ((int)contents.size() > node_capacity)
                    subdivide();
                return true;
            } else {
                bool taken = false;
                for (int i = 0; i < 4; ++i) {
                    if (children[i]->insert(id, entry)) {
                        taken = true;
                        if (std::find(ids.begin(), ids.end(), id) == ids.end())
                            ids.push_back(id);
                        break;
                    }
                }

                if (!taken) {
                    contents.push_back(entry);
                    ids.push_back(id);
                }
                return true;
            }
            return false;
        };

        bool query(const int target_x, const int target_y, const int target_width, const int target_height, std::vector<T*>& result) {
            if (!(
                x <= target_x+target_width &&
                x+width >= target_x &&
                y <= target_y+target_height &&
                y+height >= target_y
            ))
                return false;

            for (auto& entry : contents) {
                if (
                    entry.x <= target_x+target_width &&
                    entry.x+entry.width >= target_x &&
                    entry.y <= target_y+target_height &&
                    entry.y+entry.height >= target_y
                )
                    result.push_back(&entry);
            }
            if (children[0] != nullptr) {
                for (int i = 0; i < 4; ++i) {
                    children[i]->query(target_x, target_y, target_width, target_height, result);
                }
            }
            return true;
        };

        bool query_by_id(const std::string& id, std::vector<T*>& result) {
            if (std::find(ids.begin(), ids.end(), id) == ids.end())
                return false;

            for (size_t i = 0; i < contents.size(); ++i) {
                if (ids[i] == id)
                    result.push_back(&contents[i]);
            }
            if (children[0] != nullptr) {
                for (int i = 0; i < 4; ++i) {
                    children[i]->query_by_id(id, result);
                }
            }
            return true;
        };

        bool render(SDL_Renderer* renderer, const int x_offset, const int y_offset, const int zoom, const SDL_Color& colour={200, 30, 210, 255}) { // TODO: don't render outside of view
            SDL_FRect rect = {
                (float)(x - x_offset) * zoom,
                (float)(y - y_offset) * zoom,
                (float)width * zoom,
                (float)height * zoom
            };
            SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
            SDL_RenderRect(renderer, &rect);

            if (children[0] != nullptr) {
                for (int i = 0; i < 4; ++i) {
                    children[i]->render(renderer, x_offset, y_offset, zoom, colour);
                }
            }
            return true;
        };
};

#endif
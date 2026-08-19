#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include <deque>
#include <vector>
#include <unordered_map>
#include <string>
#include <SDL3/SDL.h>
#include <cmath>

#include "utils/logger.hpp"

template<typename T>
struct QuadtreeEntry {
    T* content = nullptr;
    int max_depth = -1;
};

// !! "T" needs to have x, y, width and height attributes !!
template<typename T>
class QuadtreeNode {
    private:
        std::unordered_map<std::string, QuadtreeEntry<T>> contents;
        std::deque<T> contents_storage;
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

            std::unordered_map<std::string, QuadtreeEntry<T>> old_contents = contents;
            contents.clear();
            for (const auto& [id, current_entry] : old_contents) {
                insert_pointer(id, current_entry.content, current_entry.max_depth);
            }
            old_contents.clear();

            return true;
        };

    public:
        int x, y;
        int width, height;
        int node_capacity = 0;
        int depth = 0;

        QuadtreeNode(): x(0), y(0), width(0), height(0), node_capacity(0), depth(0) {};
        QuadtreeNode(const int x, const int y, const int width, const int height, const int node_capacity, const int depth=0): x(x), y(y), width(width), height(height), node_capacity(node_capacity), depth(depth) {};
        ~QuadtreeNode() {};
        void set_dimensions(int x, int y, int width, int height) {
            this->x = x;
            this->y = y;
            this->width = width;
            this->height = height;
        }
        void set_capacity(const int node_capacity) {
            this->node_capacity = node_capacity;
        };

        bool insert(const std::string& id, T entry, const int& max_depth=-1, const bool allow_subdivision=true) {
            if (depth == 0) {
                if (!(
                    entry.x <= x+width &&
                    entry.y <= y+height &&
                    entry.x+entry.width >= x &&
                    entry.y+entry.height >= y
                ))
                    return false;
            } else if (!(
                entry.x >= x &&
                entry.y >= y &&
                entry.x+entry.width <= x+width &&
                entry.y+entry.height <= y+height
            )) {
                return false;
            }

            if (children[0] == nullptr) {
                contents_storage.push_back(entry);
                contents[id] = {&contents_storage.back(), max_depth};

                if (allow_subdivision && ((int)contents.size() > node_capacity))
                    subdivide();
                return true;
            } else if ((max_depth != -1) && (max_depth <= depth)) {
                contents_storage.push_back(entry);
                contents[id] = {&contents_storage.back(), max_depth};
                return true;
            } else {
                bool taken = false;
                for (int i = 0; i < 4; ++i) {
                    if (children[i]->insert(id, entry, max_depth)) {
                        taken = true;
                        contents[id] = {nullptr, depth};
                        break;
                    }
                }

                if (!taken) {
                    contents_storage.push_back(entry);
                    contents[id] = {&contents_storage.back(), max_depth};
                }
                return true;
            }
            return false;
        };

        bool insert_pointer(const std::string& id, T* entry, const int& max_depth=-1, const bool allow_subdivision=true) {
            if (depth == 0) {
                if (!(
                    entry->x <= x+width &&
                    entry->y <= y+height &&
                    entry->x+entry->width >= x &&
                    entry->y+entry->height >= y
                ))
                    return false;
            } else if (!(
                entry->x >= x &&
                entry->y >= y &&
                entry->x+entry->width <= x+width &&
                entry->y+entry->height <= y+height
            )) {
                return false;
            }

            if (children[0] == nullptr) {
                contents[id] = {entry, max_depth};

                if (allow_subdivision && ((int)contents.size() > node_capacity))
                    subdivide();
                return true;
            } else if ((max_depth != -1) && (max_depth <= depth)) {
                contents[id] = {entry, max_depth};
                return true;
            } else {
                bool taken = false;
                for (int i = 0; i < 4; ++i) {
                    if (children[i]->insert_pointer(id, entry, max_depth)) {
                        taken = true;
                        contents[id] = {nullptr, depth};
                        break;
                    }
                }

                if (!taken)
                    contents[id] = {entry, max_depth};
                return true;
            }
            return false;
        };

        bool trigger_subdivision() {
            if ((children[0] == nullptr) && ((int)contents.size() > node_capacity))
                subdivide();
            return true;
        };

        bool query(const int target_x, const int target_y, const int target_width, const int target_height, std::vector<T*>& result) {
            if (!(
                x <= target_x+target_width &&
                x+width >= target_x &&
                y <= target_y+target_height &&
                y+height >= target_y
            ))
                return false;

            for (const auto& [id, current_entry] : contents) {
                if (current_entry.content == nullptr)
                    continue;
                if (
                    current_entry.content->x <= target_x+target_width &&
                    current_entry.content->x+current_entry.content->width >= target_x &&
                    current_entry.content->y <= target_y+target_height &&
                    current_entry.content->y+current_entry.content->height >= target_y
                )
                    result.push_back(current_entry.content);
            }
            if (children[0] != nullptr) {
                for (int i = 0; i < 4; ++i) {
                    children[i]->query(target_x, target_y, target_width, target_height, result);
                }
            }
            return true;
        };

        bool query_by_id(const std::string& id, std::vector<T*>& result) {
            bool is_present = false;
            for (const auto& [current_id, current_entry] : contents) {
                if (current_id == id) {
                    is_present = true;
                    break;
                }
            }
            if (!is_present)
                return false;

            for (const auto& [current_id, current_entry] : contents) {
                if ((current_id == id) && (current_entry.content != nullptr)) {
                    result.push_back(current_entry.content);
                }
            }
            if (children[0] != nullptr) {
                for (int i = 0; i < 4; ++i) {
                    children[i]->query_by_id(id, result);
                }
            }
            return true;
        };

        bool render(SDL_Renderer* renderer, const int x_offset, const int y_offset, const int zoom, const int resolution, const SDL_Color& colour={200, 30, 210, 255}) { // TODO: don't render outside of view
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
};

#endif
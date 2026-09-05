#ifndef RENDER_AGENT_HPP
#define RENDER_AGENT_HPP

#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdint>
#include <deque>

inline constexpr int animation_frames_count = 12;

struct RenderAgentTexture {
    SDL_Texture* texture = nullptr;
    int size = -1; // If the texture is an atlas, this will be positive (see RenderAgent::_find_atlas_pos())
    std::vector<int> rows_x;
    std::vector<int> rows_h;
};

struct SpriteAnimation {
    std::vector<SDL_Rect> texture_rects = std::vector<SDL_Rect>(animation_frames_count, {0, 0, 16, 16});
};

struct RenderAgentSprite {
    std::string name;
    RenderAgentTexture* texture = nullptr;
    std::vector<SpriteAnimation> animations = std::vector<SpriteAnimation>(1, SpriteAnimation());
};

struct RenderAgentEntity {
    int x, y;
    int layer;
    RenderAgentSprite* sprite = nullptr;
    uint8_t animation = 0;
    uint8_t animation_frame = 0;
    uint16_t rotation = 0;
    bool movable = false;
};

struct TextureConstructor {
    std::string texture; // file path or texture id
    int x, y;
};

struct Text {
    std::string id;
    TTF_Text* text;
    int x = 0;
    int y = 0;
    SDL_Color colour = {255, 255, 255, 255};
};

class RenderAgentQuadtreeNode {
    private:
        std::vector<RenderAgentEntity*> contents;
        RenderAgentQuadtreeNode* children[4] = {nullptr, nullptr, nullptr, nullptr};
    
    public:
        int x, y;
        int width, height;
        int node_capacity = 16;
        uint8_t depth = 0;

        RenderAgentQuadtreeNode(): x(0), y(0), width(0), height(0), node_capacity(0), depth(0) {};
        RenderAgentQuadtreeNode(const int x, const int y, const int width, const int height, const int node_capacity, const int depth=0): x(x), y(y), width(width), height(height), node_capacity(node_capacity), depth(depth) {};
        ~RenderAgentQuadtreeNode() {};

        void set_dimensions(int x, int y, int width, int height);
        void set_capacity(const int node_capacity);

        bool subdivide();
        bool insert(RenderAgentEntity* entity, const bool allow_subdivision = true);
        bool query(const int target_x, const int target_y, const int target_width, const int target_height, std::vector<RenderAgentEntity*>& result);
        bool render(SDL_Renderer* renderer, const int x_offset, const int y_offset, const int zoom, const int resolution, const SDL_Color& colour={200, 30, 210, 255});
};

class RenderAgent {
    private:
        SDL_Renderer* renderer;
        SDL_Texture* target[animation_frames_count] = {nullptr};

        std::unordered_map<std::string, RenderAgentTexture> agent_textures;
        std::unordered_map<std::string, RenderAgentSprite> agent_sprites;
        std::deque<RenderAgentEntity> agent_entitys;

        TTF_TextEngine* text_engine = nullptr;
        std::unordered_map<std::string, TTF_Font*> fonts;
        std::vector<Text> texts;
    
        bool dirty[animation_frames_count] = {true};
    public:
        RenderAgentQuadtreeNode agent_quadtree;
        int heighest_layer = -1;

        RenderAgent(SDL_Renderer* renderer, bool allow_text = false);
        ~RenderAgent();

        RenderAgent(const RenderAgent&) = delete;
        RenderAgent& operator=(const RenderAgent&) = delete;
        RenderAgent(RenderAgent&&) = delete;
        RenderAgent& operator=(RenderAgent&&) = delete;

        void set_dimensions(const int& x, const int& y, const int& width, const int& height);
        
        bool render(const int zoom=0, const int x_offset=0, const int y_offset=0, const bool clear_renderer=true, const int resolution=1, SDL_Color clear_colour={26, 26, 26, 255});
        void render_target();
        void set_dirty(bool value = true);

        // Textures
        SDL_Texture* load_texture(const std::string& texture); // registry key (= id) or path
        bool insert_texture(const std::string& id, RenderAgentTexture texture);
        RenderAgentTexture* get_texture(const std::string& id, bool suppress_logs = false);
        void drop_texture(const std::string& id);
        bool add_texture(const std::string& id, const std::string& texture_path);
        SDL_Texture* bake_texture(std::vector<TextureConstructor*> constructors);
        void list_textures(); // For debugging
        bool check_texture_ptr(RenderAgentTexture* ptr); // For debugging
        
        // Atlas
        std::filesystem::path get_png_path(const std::string& name);
        bool find_atlas_pos(RenderAgentTexture* atlas, SDL_Texture* texture, TextureConstructor* constructor);
        bool bake_atlas(const std::string& atlas_name, const std::vector<std::string> texture_names);
        bool add_to_atlas(const std::string& atlas_name, const std::string& texture_name);

        // Sprites
        RenderAgentSprite* get_sprite(const std::string& id, bool suppress_logs = false);
        bool add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& w, const int& h, const std::filesystem::path& sprite_sheet_path);
        void list_sprites(); // For debugging
        bool check_sprite_ptr(RenderAgentSprite* ptr); // For debugging

        // Entity
        RenderAgentEntity* add_entity(const std::string& sprite_id, const uint8_t& animation, const int& x, const int& y, const int& layer = -1, const int& rotation = 0, bool movable = false, bool allow_subdivision = true);
        RenderAgentEntity* insert_entity(RenderAgentEntity& entity, bool allow_subdivision = true);
        bool trigger_subdivision();
        RenderAgentEntity* get_entity(const int& x, const int& y, bool suppress_logs = false);

        // Text
        TTF_Font* get_font(const std::string& font_name, bool suppress_logs = false);
        Text* get_text(const std::string& id, bool suppress_logs = false);
        bool add_font(const std::string& font_name, const std::filesystem::path& font_path, const float font_size);
        bool add_text(const std::string& id, const std::string& content, const std::string& font_name, const int x, const int y, const SDL_Color colour={255, 255, 255, 255});
};

#endif
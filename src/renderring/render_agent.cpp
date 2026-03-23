#include "renderring/render_agent.hpp"

// A lot of the basics of SDL3_GPU taken from https://glusoft.com/sdl3-tutorials/display-texture-sdl3_gpu/ and https://glusoft.com/sdl3-tutorials/sprite-batching-sdl3-gpu/

struct SpriteInstance {
    float x, y, z;
    float rotation;
    float w, h, padding_a, padding_b;
    float tex_u, tex_v, tex_w, tex_h;
    float r, g, b, a;
};

RenderAgent::RenderAgent(SDL_Window* window) {
    if (int(RENDER_SETTINGS["render_mode"]) == 0) {
        // SWAPCHAIN PARAMETERS //
        SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
        if (SDL_WindowSupportsGPUPresentMode(
            GPU,
            WINDOW,
            SDL_GPU_PRESENTMODE_IMMEDIATE
        )) {
            presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        } else if (SDL_WindowSupportsGPUPresentMode(
            GPU,
            WINDOW,
            SDL_GPU_PRESENTMODE_MAILBOX
        )) {
            presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
        }

        SDL_SetGPUSwapchainParameters(
            GPU,
            WINDOW,
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            presentMode
        );

        // Load Shaders //
        SDL_GPUShader* vertex_shader = LoadShader(GPU, "simple_texture.vert", 0, 0, 0, 0);
        if (vertex_shader == nullptr)
        {
            LOG(LogLevel::ERROR,"Failed to create vertex shader: %s", SDL_GetError());
            //RENDER_SETTINGS["render_mode"].set(1);
            //SDL_DestroyGPUDevice(GPU);
            return;
        }

        SDL_GPUShader* fragment_shader = LoadShader(GPU, "simple_texture.frag", 0, 0, 0, 0);
        if (fragment_shader == nullptr)
        {
            LOG(LogLevel::ERROR, "Failed to create fragment shader: %s", SDL_GetError());
            //RENDER_SETTINGS["render_mode"].set(1);
            //SDL_DestroyGPUDevice(GPU);
            return;
        } 

        // Create Pipeline //
        SDL_GPUColorTargetDescription target_desc[] {{
            .format = SDL_GetGPUSwapchainTextureFormat(GPU, window),
            .blend_state = {
                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .color_blend_op = SDL_GPU_BLENDOP_ADD,
                .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                .enable_blend = true
            }
        }};
            
        SDL_GPUGraphicsPipelineTargetInfo pipeline_target_info{
            .color_target_descriptions = target_desc,
            .num_color_targets = 1
        };

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info{
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .target_info = pipeline_target_info

        };
        
        pipeline = SDL_CreateGPUGraphicsPipeline(GPU, &pipeline_info);
        if (pipeline == NULL)
        {
            LOG(LogLevel::CRITICAL, "Failed to create pipeline!");
            return;
        }
        SDL_ReleaseGPUShader(GPU, vertex_shader);
        SDL_ReleaseGPUShader(GPU, fragment_shader);

        // Create Sampler //
        SDL_GPUSamplerCreateInfo sampler_info{
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        };
        sampler = SDL_CreateGPUSampler(GPU, &sampler_info);

        /*
        // Sprite Data Transfer Buffer //
        SDL_GPUTransferBufferCreateInfo transfer_buffer_info {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = SPRITE_COUNT * sizeof(SpriteInstance)
        };

        sprite_data_transfer_buffer = SDL_CreateGPUTransferBuffer(
            GPU,
            &transfer_buffer_info
        );z

        // Sprite Data Buffer //
        SDL_GPUBufferCreateInfo buffer_info_read {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = SPRITE_COUNT * sizeof(SpriteInstance)
        };

        sprite_data_buffer = SDL_CreateGPUBuffer(
            GPU,
            &buffer_info_read
        );
        */

    }
};

RenderAgentTexture RenderAgent::load_texture(const std::string& id, const std::string& texture_path) {
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Loading Texture %s", texture_path.c_str());
    SDL_Surface* image_surface = IMG_Load(texture_path.c_str());
    if (image_surface == NULL) {
        LOG(LogLevel::WARNING, "Could not load Texture '%s': %s", texture_path.c_str(), SDL_GetError());
        return RenderAgentTexture();
    }

    if (int(RENDER_SETTINGS["render_mode"]) == 1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(RENDERER, image_surface);
        if (texture == NULL) {
            LOG(LogLevel::WARNING, "Could not create Texture from Surface: %s", SDL_GetError());
        }
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        RenderAgentTexture agent_texture = RenderAgentTexture(texture, image_surface->w, image_surface->h);
        SDL_DestroySurface(image_surface);
        return agent_texture;
    } else if (int(RENDER_SETTINGS["render_mode"]) == 0) {
        // Sprite Data Transfer Buffer //
        SDL_GPUTransferBufferCreateInfo transfer_buffer_info {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(SpriteInstance)
        };

        SDL_GPUTransferBuffer* sprite_data_transfer_buffer = SDL_CreateGPUTransferBuffer(
            GPU,
            &transfer_buffer_info
        );

        // Sprite Data Buffer //
        SDL_GPUBufferCreateInfo buffer_info_read {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(SpriteInstance)
        };

        SDL_GPUBuffer* sprite_data_buffer = SDL_CreateGPUBuffer(
            GPU,
            &buffer_info_read
        );


        // Transfer Buffer //
        SDL_GPUTransferBufferCreateInfo buffer_info {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = static_cast<Uint32>(image_surface->w * image_surface->h * 4)
        };

        SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(
            GPU,
            &buffer_info
        );

        Uint8 *textureTransferPtr = (Uint8*) SDL_MapGPUTransferBuffer(
            GPU,
            texture_transfer_buffer,
            false
        );
        SDL_memcpy(textureTransferPtr, image_surface->pixels, image_surface->w * image_surface->h * 4);
        SDL_UnmapGPUTransferBuffer(GPU, texture_transfer_buffer);

        // Create GPU Texture //
        SDL_GPUTextureCreateInfo texture_info{
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = static_cast<Uint32>(image_surface->w),
            .height = static_cast<Uint32>(image_surface->h),
            .layer_count_or_depth = 1,
            .num_levels = 1
        };

        SDL_GPUTexture* texture = SDL_CreateGPUTexture(GPU, &texture_info);
        SDL_SetGPUTextureName(
            GPU,
            texture,
            id.c_str()
        );

        // Upload //        
        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(GPU);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_GPUTextureTransferInfo transfer_info {
            .transfer_buffer = texture_transfer_buffer,
            .offset = 0, // Zeroes out the rest
        };

        SDL_GPUTextureRegion texture_region {
            .texture = texture,
            .w = static_cast<Uint32>(image_surface->w),
            .h = static_cast<Uint32>(image_surface->h),
            .d = 1
        };

        SDL_UploadToGPUTexture(
            copyPass,
            &transfer_info,
            &texture_region,
            false
        );

        SDL_GPUTextureSamplerBinding sampler_binding {
            .texture = texture,
            .sampler = sampler
        };

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);

        RenderAgentTexture dst_texture = RenderAgentTexture(texture, image_surface->w, image_surface->h);
        return dst_texture;

        SDL_DestroySurface(image_surface);
        SDL_ReleaseGPUTransferBuffer(GPU, texture_transfer_buffer);

        SDL_ReleaseGPUTransferBuffer(GPU, sprite_data_transfer_buffer);
        SDL_ReleaseGPUBuffer(GPU, sprite_data_buffer);
    }


    return RenderAgentTexture();
};

bool RenderAgent::insert_texture(const std::string& id, RenderAgentTexture texture) {
    agent_textures[id] = texture;
    return true;
}

bool RenderAgent::texture_exists(const std::string& id) {
    return agent_textures.find(id) != agent_textures.end();
}

RenderAgentTexture RenderAgent::get_texture(const std::string& id) {
    if (texture_exists(id)) {
        return agent_textures[id];
    }
    return RenderAgentTexture();
}

bool RenderAgent::add_texture(const std::string& id, const std::string& texture_path) {
    return insert_texture(id, load_texture(id, texture_path));
};

bool RenderAgent::add_sprite(const std::string& id, const std::string& texture_id, const int& x, const int& y, const int& width, const int& height) {
    int sprite_x = x;
    int sprite_y = y;
    int sprite_width = (width < 0) ? agent_textures[texture_id].width : width;
    int sprite_height = (height < 0) ? agent_textures[texture_id].height: height;

    SDL_FRect srcrect = {static_cast<float>(sprite_x), static_cast<float>(sprite_y), static_cast<float>(sprite_width), static_cast<float>(sprite_height)};
    agent_sprites[id] = RenderAgentSprite(texture_id, srcrect);
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Added new Sprite %s: srcrect: %f, %f, %f, %f", id.c_str(), agent_sprites[id].texture_rect.x, agent_sprites[id].texture_rect.y, agent_sprites[id].texture_rect.w, agent_sprites[id].texture_rect.h);
    return true;
};

bool RenderAgent::add_entity(const std::string& id, const std::string& sprite_id, const int& x, const int& y, const int& size, const int& rotation) {
    agent_entitys.push_back(RenderAgentEntity(id, sprite_id, x, y, size, rotation));
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Added new Entity %s: X: %d, Y: %d, Size: %d, Rotation: %d", id.c_str(), x, y, size, rotation);
    return true;
};

void RenderAgent::render(bool clear_renderer) {
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Starting Renderpass");
    if ((int(RENDER_SETTINGS["render_mode"]) == 1) || (int(RENDER_SETTINGS["render_mode"]) == 2)) {
        if (clear_renderer) {
            SDL_SetRenderDrawColor(RENDERER, 26, 26, 26, 255);
            SDL_RenderClear(RENDERER);
        }

        for (const RenderAgentEntity& entity : agent_entitys) {
            if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Renderring Entity %s at %d;%d", entity.name.c_str(), (entity.x-CAMERA.x)*CAMERA.zoom, (entity.y-CAMERA.y)*CAMERA.zoom);
            // Sprite
            if (!(agent_sprites.find(entity.sprite) != agent_sprites.end())) {
                LOG(LogLevel::WARNING, "Sprite '%s' not found", entity.sprite.c_str());
            }
            const RenderAgentSprite sprite = agent_sprites[entity.sprite];
            // Texture
            if (!(agent_textures.find(sprite.texture) != agent_textures.end())) {
                LOG(LogLevel::WARNING, "Texture '%s' not found", sprite.texture.c_str());
            }
            SDL_Texture* texture = agent_textures[sprite.texture].get_texture();
            SDL_FRect dstrect = {(float)((entity.x-CAMERA.x)*CAMERA.zoom), (float)((entity.y-CAMERA.y)*CAMERA.zoom), (sprite.texture_rect.w*CAMERA.zoom), (sprite.texture_rect.h*CAMERA.zoom)};
            SDL_RenderTexture(RENDERER, texture, &sprite.texture_rect, &dstrect);
        }

        SDL_RenderPresent(RENDERER);

        if (int(RENDER_SETTINGS["render_mode"]) == 2) {
            SDL_UpdateWindowSurface(WINDOW);
        }

    } else if (int(RENDER_SETTINGS["render_mode"]) == 0) {
        SDL_GPUCommandBuffer* cmd_buffer;
        cmd_buffer = SDL_AcquireGPUCommandBuffer(GPU);
        if (cmd_buffer == NULL) {
            SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
            return;
        }

        SDL_GPUTexture* swapchain_texture;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buffer, WINDOW, &swapchain_texture, NULL, NULL)) {
            SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
            return;
        }

        if (swapchain_texture == NULL) {
            LOG(LogLevel::CRITICAL, "Aquiering Swapchain Texture Failed: %s", SDL_GetError());
            return;
        }

        SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.cycle = true;
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

        SDL_GPURenderPass* render_pass;
        render_pass = SDL_BeginGPURenderPass(cmd_buffer, &target_info, 1, NULL);
        SDL_EndGPURenderPass(render_pass);

        SDL_SubmitGPUCommandBuffer(cmd_buffer);
    }
}

RenderAgentTexture RenderAgent::bake_texture(TextureConstructor* texture_constructors[], const int array_size, const bool force_file_loading) {
    int surface_width = 0;
    int surface_height = 0;
    for (int i = 0; i < array_size; ++i) {
        TextureConstructor* constructor = texture_constructors[i];
        if (texture_exists(constructor->name) & !force_file_loading) constructor->texture = &(agent_textures[constructor->name]);
        else                                                         constructor->texture = new RenderAgentTexture(load_texture(constructor->file, constructor->file));
        surface_width = std::max(surface_width, (constructor->texture->width+constructor->x)*constructor->size);
        surface_height = std::max(surface_height, (constructor->texture->height+constructor->y)*constructor->size);
    }
    if (DEBUG["all_debug_logs"]) LOG(LogLevel::DEBUG, "Bake Surface is %dx%d", surface_width, surface_height);
    if (RENDER_SETTINGS["render_mode"] == 1) {
        SDL_Texture* bake_texture = SDL_CreateTexture(RENDERER, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, surface_width, surface_height);
        SDL_SetTextureScaleMode(bake_texture, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderTarget(RENDERER, bake_texture);
        for (int i = 0; i < array_size; ++i) {
            TextureConstructor* constructor = texture_constructors[i];
            SDL_FRect dstrect = {(float)(constructor->x*constructor->size), (float)(constructor->y*constructor->size), (float)(constructor->texture->width*constructor->size), (float)(constructor->texture->height*constructor->size)};
            SDL_RenderTexture(RENDERER, constructor->texture->get_texture(), NULL, &dstrect);
        }
        SDL_SetRenderTarget(RENDERER, nullptr);
        return RenderAgentTexture(bake_texture, surface_width, surface_height);
    }
    return RenderAgentTexture();
}

void RenderAgent::drop_texture(const std::string& id) {
    agent_textures.erase(id.c_str());
}

RenderAgentEntity RenderAgent::get_entity(const std::string& id) {
    for (auto& entity : agent_entitys) {
        if (entity.name == id) {
            //LOG(LogLevel::DEBUG, "Found entity %s!", id.c_str());
            return entity;
        }
    }
    return RenderAgentEntity();
}

bool RenderAgent::update_entity(RenderAgentEntity update_entity) {
    for (auto& entity : agent_entitys) {
        if (entity.name == update_entity.name) {
            entity = update_entity;
            return true;
        }
    }
    return false;
}
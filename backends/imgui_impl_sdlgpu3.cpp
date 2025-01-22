// dear imgui: Renderer Backend for SDL_GPU
// This needs to be used along with the SDL3 Platform Backend

// Implemented features:
//  [X] Renderer: User texture binding. Use simply cast a reference to your SDL_GPUTextureSamplerBinding to ImTextureID.
//  [X] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.

// The aim of imgui_impl_sdlgpu3.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important note to the reader who wish to integrate imgui_impl_sdlgpu3.cpp/.h in their own engine/app.
// - Unlike other backends, the user must call the function Imgui_ImplSDLGPU3_PrepareDrawData() BEFORE issuing a SDL_GPURenderPass containing ImGui_ImplSDLGPU3_RenderDrawData.
//   Calling the function is MANDATORY, otherwise the ImGui will not upload neither the vertex nor the index buffer for the GPU. See imgui_impl_sdlgpu3.cpp for more info.

// CHANGELOG
//  2025-01-16: Renamed ImGui_ImplSDLGPU3_InitInfo::GpuDevice to Device.
//  2025-01-09: SDL_GPU: Added the SDL_GPU3 backend.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_sdlgpu3.h"
#include "imgui_impl_sdlgpu3_shaders.h"

// SDL_GPU Data

// Reusable buffers used for rendering 1 current in-flight frame, for ImGui_ImplSDLGPU3_RenderDrawData()
struct ImGui_ImplSDLGPU3_FrameData
{
    SDL_GPUBuffer*      VertexBuffer     = nullptr;
    SDL_GPUBuffer*      IndexBuffer      = nullptr;
    uint32_t            VertexBufferSize = 0;
    uint32_t            IndexBufferSize  = 0;
};

struct ImGui_ImplSDLGPU3_Data
{
    ImGui_ImplSDLGPU3_InitInfo   InitInfo;

    // Graphics pipeline & shaders
    SDL_GPUShader*               VertexShader   = nullptr;
    SDL_GPUShader*               FragmentShader = nullptr;
    SDL_GPUGraphicsPipeline*     Pipeline       = nullptr;

    // Font data
    SDL_GPUSampler*              FontSampler = nullptr;
    SDL_GPUTexture*              FontTexture = nullptr;
    SDL_GPUTextureSamplerBinding FontBinding = { nullptr, nullptr };

    // Frame data for main window
    ImGui_ImplSDLGPU3_FrameData  MainWindowFrameData;
};

// Forward Declarations
static void ImGui_ImplSDLGPU3_DestroyFrameData();

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support has never been tested.
static ImGui_ImplSDLGPU3_Data* ImGui_ImplSDLGPU3_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplSDLGPU3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static void ImGui_ImplSDLGPU3_SetupRenderState(ImDrawData* draw_data, SDL_GPUGraphicsPipeline* pipeline, SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass * render_pass, ImGui_ImplSDLGPU3_FrameData* fd, uint32_t fb_width, uint32_t fb_height)
{
    //ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();

    // Bind graphics pipeline
    SDL_BindGPUGraphicsPipeline(render_pass,pipeline);

    // Bind Vertex And Index Buffers
    if (draw_data->TotalVtxCount > 0)
    {
        SDL_GPUBufferBinding vertex_buffer_binding = {};
        vertex_buffer_binding.buffer = fd->VertexBuffer;
        vertex_buffer_binding.offset = 0;
        SDL_GPUBufferBinding index_buffer_binding = {};
        index_buffer_binding.buffer = fd->IndexBuffer;
        index_buffer_binding.offset = 0;
        SDL_BindGPUVertexBuffers(render_pass,0, &vertex_buffer_binding, 1);
        SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, sizeof(ImDrawIdx) == 2 ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
    }

    // Setup viewport
    SDL_GPUViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.w = (float)fb_width;
    viewport.h = (float)fb_height;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(render_pass, &viewport);

    // Setup scale and translation
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    struct UBO { float scale[2]; float translation[2]; } ubo;
    ubo.scale[0] = 2.0f / draw_data->DisplaySize.x;
    ubo.scale[1] = 2.0f / draw_data->DisplaySize.y;
    ubo.translation[0] = -1.0f - draw_data->DisplayPos.x * ubo.scale[0];
    ubo.translation[1] = -1.0f - draw_data->DisplayPos.y * ubo.scale[1];
    SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
}

static void CreateOrResizeBuffer(SDL_GPUBuffer** buffer, uint32_t* old_size, uint32_t new_size, SDL_GPUBufferUsageFlags usage)
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    // Even though this is fairly rarely called.
    SDL_WaitForGPUIdle(v->Device);
    SDL_ReleaseGPUBuffer(v->Device, *buffer);

    SDL_GPUBufferCreateInfo buffer_info = {};
    buffer_info.usage = usage;
    buffer_info.size = new_size;
    buffer_info.props = 0;
    *buffer = SDL_CreateGPUBuffer(v->Device, &buffer_info);
    *old_size = new_size;
    IM_ASSERT(*buffer != nullptr && "Failed to create GPU Buffer, call SDL_GetError() for more information");
}

// SDL_GPU doesn't allow copy passes to occur while a render or compute pass is bound!
// The only way to allow a user to supply their own RenderPass (to render to a texture instead of the window for example),
// is to split the upload part of ImGui_ImplSDLGPU3_RenderDrawData() to another function that needs to be called by the user before rendering.
void Imgui_ImplSDLGPU3_PrepareDrawData(ImDrawData* draw_data, SDL_GPUCommandBuffer* command_buffer)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount <= 0)
        return;

    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;
    ImGui_ImplSDLGPU3_FrameData* fd = &bd->MainWindowFrameData;

    uint32_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    uint32_t index_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    if (fd->VertexBuffer == nullptr || fd->VertexBufferSize < vertex_size)
        CreateOrResizeBuffer(&fd->VertexBuffer, &fd->VertexBufferSize, vertex_size, SDL_GPU_BUFFERUSAGE_VERTEX);
    if (fd->IndexBuffer == nullptr || fd->IndexBufferSize < index_size)
        CreateOrResizeBuffer(&fd->IndexBuffer, &fd->IndexBufferSize, index_size, SDL_GPU_BUFFERUSAGE_INDEX);

    // FIXME: It feels like more code could be shared there.
    SDL_GPUTransferBufferCreateInfo vertex_transferbuffer_info = {};
    vertex_transferbuffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vertex_transferbuffer_info.size = vertex_size;
    SDL_GPUTransferBufferCreateInfo index_transferbuffer_info = {};
    index_transferbuffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    index_transferbuffer_info.size = index_size;

    SDL_GPUTransferBuffer* vertex_transferbuffer = SDL_CreateGPUTransferBuffer(v->Device, &vertex_transferbuffer_info);
    IM_ASSERT(vertex_transferbuffer != nullptr && "Failed to create the vertex transfer buffer, call SDL_GetError() for more information");
    SDL_GPUTransferBuffer* index_transferbuffer = SDL_CreateGPUTransferBuffer(v->Device, &index_transferbuffer_info);
    IM_ASSERT(index_transferbuffer != nullptr && "Failed to create the index transfer buffer, call SDL_GetError() for more information");

    ImDrawVert* vtx_dst = (ImDrawVert*)SDL_MapGPUTransferBuffer(v->Device, vertex_transferbuffer, true);
    ImDrawIdx*  idx_dst = (ImDrawIdx*)SDL_MapGPUTransferBuffer(v->Device, index_transferbuffer, true);
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += draw_list->VtxBuffer.Size;
        idx_dst += draw_list->IdxBuffer.Size;
    }
    SDL_UnmapGPUTransferBuffer(v->Device, vertex_transferbuffer);
    SDL_UnmapGPUTransferBuffer(v->Device, index_transferbuffer);

    SDL_GPUTransferBufferLocation vertex_buffer_location = {};
    vertex_buffer_location.offset = 0;
    vertex_buffer_location.transfer_buffer = vertex_transferbuffer;
    SDL_GPUTransferBufferLocation index_buffer_location = {};
    index_buffer_location.offset = 0;
    index_buffer_location.transfer_buffer = index_transferbuffer;

    SDL_GPUBufferRegion vertex_buffer_region = {};
    vertex_buffer_region.buffer = fd->VertexBuffer;
    vertex_buffer_region.offset = 0;
    vertex_buffer_region.size = vertex_size;

    SDL_GPUBufferRegion index_buffer_region = {};
    index_buffer_region.buffer = fd->IndexBuffer;
    index_buffer_region.offset = 0;
    index_buffer_region.size = index_size;

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    SDL_UploadToGPUBuffer(copy_pass, &vertex_buffer_location, &vertex_buffer_region, true);
    SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_buffer_region, true);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(v->Device, index_transferbuffer);
    SDL_ReleaseGPUTransferBuffer(v->Device, vertex_transferbuffer);
}

void ImGui_ImplSDLGPU3_RenderDrawData(ImDrawData* draw_data, SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* render_pass, SDL_GPUGraphicsPipeline* pipeline)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_FrameData* fd = &bd->MainWindowFrameData;

    if (pipeline == nullptr)
        pipeline = bd->Pipeline;

    ImGui_ImplSDLGPU3_SetupRenderState(draw_data, pipeline, command_buffer, render_pass, fd, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as SDL_SetGPUScissor() won't accept values that are off bounds
                if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
                if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
                if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
                if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                SDL_Rect scissor_rect = {};
                scissor_rect.x = (int)clip_min.x;
                scissor_rect.y = (int)clip_min.y;
                scissor_rect.w = (int)(clip_max.x - clip_min.x);
                scissor_rect.h = (int)(clip_max.y - clip_min.y);
                SDL_SetGPUScissor(render_pass,&scissor_rect);

                // Bind DescriptorSet with font or user texture
                SDL_BindGPUFragmentSamplers(render_pass, 0, (SDL_GPUTextureSamplerBinding*)pcmd->GetTexID(), 1);

                // Draw
                SDL_DrawGPUIndexedPrimitives(render_pass, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += draw_list->IdxBuffer.Size;
        global_vtx_offset += draw_list->VtxBuffer.Size;
    }

    // Note: at this point both SDL_SetGPUViewport() and SDL_SetGPUScissor() have been called.
    // Our last values will leak into user/application rendering if you forgot to call SDL_SetGPUViewport() and SDL_SetGPUScissor() yourself to explicitly set that state
    // In theory we should aim to backup/restore those values but I am not sure this is possible.
    // We perform a call to SDL_SetGPUScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)
    SDL_Rect scissor_rect { 0, 0, fb_width, fb_height };
    SDL_SetGPUScissor(render_pass, &scissor_rect);
}

void ImGui_ImplSDLGPU3_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    // Destroy existing texture (if any)
    if (bd->FontTexture)
    {
        SDL_WaitForGPUIdle(v->Device);
        ImGui_ImplSDLGPU3_DestroyFontsTexture();
    }

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    uint32_t upload_size = width * height * 4 * sizeof(char);

    // Create the Image:
    {
        SDL_GPUTextureCreateInfo texture_info = {};
        texture_info.type   = SDL_GPU_TEXTURETYPE_2D;
        texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texture_info.width  = width;
        texture_info.height = height;
        texture_info.layer_count_or_depth = 1;
        texture_info.num_levels = 1;
        texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;

        bd->FontTexture = SDL_CreateGPUTexture(v->Device, &texture_info);
        IM_ASSERT(bd->FontTexture && "Failed to create font texture, call SDL_GetError() for more info");
    }

    // Assign the texture to the TextureSamplerBinding
    bd->FontBinding.texture = bd->FontTexture;

    // Create all the upload structures and upload:
    {
        SDL_GPUTransferBufferCreateInfo transferbuffer_info = {};
        transferbuffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferbuffer_info.size = upload_size;

        SDL_GPUTransferBuffer* transferbuffer = SDL_CreateGPUTransferBuffer(v->Device, &transferbuffer_info);
        IM_ASSERT(transferbuffer != nullptr && "Failed to create font transfer buffer, call SDL_GetError() for more information");

        void* texture_ptr = SDL_MapGPUTransferBuffer(v->Device, transferbuffer, false);
        memcpy(texture_ptr, pixels, upload_size);
        SDL_UnmapGPUTransferBuffer(v->Device, transferbuffer);

        SDL_GPUTextureTransferInfo transfer_info = {};
        transfer_info.offset = 0;
        transfer_info.transfer_buffer = transferbuffer;

        SDL_GPUTextureRegion texture_region = {};
        texture_region.texture = bd->FontTexture;
        texture_region.w = width;
        texture_region.h = height;
        texture_region.d = 1;

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(v->Device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
        SDL_UploadToGPUTexture(copy_pass, &transfer_info, &texture_region, false);
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(v->Device, transferbuffer);
    }

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)&bd->FontBinding);
}

// You probably never need to call this, as it is called by ImGui_ImplSDLGPU3_CreateFontsTexture() and ImGui_ImplSDLGPU3_Shutdown().
void ImGui_ImplSDLGPU3_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;
    if (bd->FontTexture)
    {
        SDL_ReleaseGPUTexture(v->Device, bd->FontTexture);
        bd->FontBinding.texture = nullptr;
        bd->FontTexture = nullptr;
    }
    io.Fonts->SetTexID(0);
}

static void Imgui_ImplSDLGPU3_CreateShaders()
{
    // Create the shader modules
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    const char* driver = SDL_GetGPUDeviceDriver(v->Device);

    SDL_GPUShaderCreateInfo vertex_shader_info = {};
    vertex_shader_info.entrypoint = "main";
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertex_shader_info.num_uniform_buffers  = 1;
    vertex_shader_info.num_storage_buffers = 0;
    vertex_shader_info.num_storage_textures = 0;
    vertex_shader_info.num_samplers = 0;

    SDL_GPUShaderCreateInfo fragment_shader_info = {};
    fragment_shader_info.entrypoint = "main";
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_shader_info.num_samplers = 1;
    fragment_shader_info.num_storage_buffers  = 0;
    fragment_shader_info.num_storage_textures = 0;
    fragment_shader_info.num_uniform_buffers  = 0;

    if (strcmp(driver, "vulkan") == 0)
    {
        vertex_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        vertex_shader_info.code = spirv_vertex;
        vertex_shader_info.code_size = sizeof(spirv_vertex);
        fragment_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        fragment_shader_info.code = spirv_fragment;
        fragment_shader_info.code_size = sizeof(spirv_fragment);
    }
    else if (strcmp(driver, "direct3d12") == 0)
    {
        vertex_shader_info.format   = SDL_GPU_SHADERFORMAT_DXBC;
        vertex_shader_info.code = dxbc_vertex;
        vertex_shader_info.code_size = sizeof(dxbc_vertex);
        fragment_shader_info.format = SDL_GPU_SHADERFORMAT_DXBC;
        fragment_shader_info.code = dxbc_fragment;
        fragment_shader_info.code_size = sizeof(dxbc_fragment);
    }
#ifdef __APPLE__
    else
    {
        vertex_shader_info.entrypoint = "main0";
        vertex_shader_info.format = SDL_GPU_SHADERFORMAT_METALLIB;
        vertex_shader_info.code = metallib_vertex;
        vertex_shader_info.code_size = sizeof(metallib_vertex);
        fragment_shader_info.entrypoint = "main0";
        fragment_shader_info.format = SDL_GPU_SHADERFORMAT_METALLIB;
        fragment_shader_info.code = metallib_fragment;
        fragment_shader_info.code_size = sizeof(metallib_fragment);
    }
#endif
    bd->VertexShader = SDL_CreateGPUShader(v->Device, &vertex_shader_info);
    bd->FragmentShader = SDL_CreateGPUShader(v->Device, &fragment_shader_info);
    IM_ASSERT(bd->VertexShader != nullptr && "Failed to create vertex shader, call SDL_GetError() for more information");
    IM_ASSERT(bd->FragmentShader != nullptr && "Failed to create fragment shader, call SDL_GetError() for more information");
}

static void ImGui_ImplSDLGPU3_CreateGraphicsPipeline()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;
    Imgui_ImplSDLGPU3_CreateShaders();

    SDL_GPUVertexBufferDescription vertex_buffer_desc[1];
    vertex_buffer_desc[0].slot = 0;
    vertex_buffer_desc[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_desc[0].instance_step_rate = 0;
    vertex_buffer_desc[0].pitch = sizeof(ImDrawVert);

    SDL_GPUVertexAttribute vertex_attributes[3];
    vertex_attributes[0].buffer_slot = 0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertex_attributes[0].location = 0;
    vertex_attributes[0].offset = offsetof(ImDrawVert,pos);

    vertex_attributes[1].buffer_slot = 0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertex_attributes[1].location = 1;
    vertex_attributes[1].offset = offsetof(ImDrawVert, uv);

    vertex_attributes[2].buffer_slot = 0;
    vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    vertex_attributes[2].location = 2;
    vertex_attributes[2].offset = offsetof(ImDrawVert, col);

    SDL_GPUVertexInputState vertex_input_state = {};
    vertex_input_state.num_vertex_attributes = 3;
    vertex_input_state.vertex_attributes = vertex_attributes;
    vertex_input_state.num_vertex_buffers = 1;
    vertex_input_state.vertex_buffer_descriptions = vertex_buffer_desc;

    SDL_GPURasterizerState rasterizer_state = {};
    rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    rasterizer_state.enable_depth_bias = false;
    rasterizer_state.enable_depth_clip = false;

    SDL_GPUMultisampleState multisample_state = {};
    multisample_state.sample_count = v->MSAASamples;
    multisample_state.enable_mask = false;

    SDL_GPUDepthStencilState depth_stencil_state = {};
    depth_stencil_state.enable_depth_test = false;
    depth_stencil_state.enable_depth_write = false;
    depth_stencil_state.enable_stencil_test = false;

    SDL_GPUColorTargetBlendState blend_state = {};
    blend_state.enable_blend = true;
    blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    blend_state.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;

    SDL_GPUColorTargetDescription color_target_desc[1];
    color_target_desc[0].format = v->ColorTargetFormat;
    color_target_desc[0].blend_state = blend_state;

    SDL_GPUGraphicsPipelineTargetInfo target_info = {};
    target_info.num_color_targets = 1;
    target_info.color_target_descriptions = color_target_desc;
    target_info.has_depth_stencil_target = false;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = bd->VertexShader;
    pipeline_info.fragment_shader = bd->FragmentShader;
    pipeline_info.vertex_input_state = vertex_input_state;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state = rasterizer_state;
    pipeline_info.multisample_state = multisample_state;
    pipeline_info.depth_stencil_state = depth_stencil_state;
    pipeline_info.target_info = target_info;

    bd->Pipeline = SDL_CreateGPUGraphicsPipeline(v->Device, &pipeline_info);
    IM_ASSERT(bd->Pipeline != nullptr && "Failed to create graphics pipeline, call SDL_GetError() for more information");
}

void ImGui_ImplSDLGPU3_CreateDeviceObjects()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    if (!bd->FontSampler)
    {
        // Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling.
        SDL_GPUSamplerCreateInfo sampler_info = {};
        sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
        sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
        sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_info.mip_lod_bias = 0.0f;
        sampler_info.min_lod = -1000.0f;
        sampler_info.max_lod = 1000.0f;
        sampler_info.enable_anisotropy = false;
        sampler_info.max_anisotropy = 1.0f;
        sampler_info.enable_compare = false;

        bd->FontSampler = SDL_CreateGPUSampler(v->Device, &sampler_info);
        bd->FontBinding.sampler = bd->FontSampler;
        IM_ASSERT(bd->FontSampler != nullptr && "Failed to create font sampler, call SDL_GetError() for more information");
    }

    ImGui_ImplSDLGPU3_CreateGraphicsPipeline();
    ImGui_ImplSDLGPU3_CreateFontsTexture();
}

void ImGui_ImplSDLGPU3_DestroyFrameData()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    SDL_ReleaseGPUBuffer(v->Device, bd->MainWindowFrameData.VertexBuffer);
    SDL_ReleaseGPUBuffer(v->Device, bd->MainWindowFrameData.IndexBuffer);
    bd->MainWindowFrameData.VertexBuffer = nullptr;
    bd->MainWindowFrameData.IndexBuffer = nullptr;
    bd->MainWindowFrameData.VertexBufferSize = 0;
    bd->MainWindowFrameData.IndexBufferSize = 0;
}

void ImGui_ImplSDLGPU3_DestroyDeviceObjects()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    ImGui_ImplSDLGPU3_DestroyFrameData();
    ImGui_ImplSDLGPU3_DestroyFontsTexture();

    if (bd->VertexShader)   { SDL_ReleaseGPUShader(v->Device, bd->VertexShader); bd->VertexShader = nullptr;}
    if (bd->FragmentShader) { SDL_ReleaseGPUShader(v->Device, bd->FragmentShader); bd->FragmentShader = nullptr;}
    if (bd->FontSampler)    { SDL_ReleaseGPUSampler(v->Device, bd->FontSampler); bd->FontSampler = nullptr;}
    if (bd->Pipeline)       { SDL_ReleaseGPUGraphicsPipeline(v->Device, bd->Pipeline); bd->Pipeline = nullptr;}
}

bool ImGui_ImplSDLGPU3_Init(ImGui_ImplSDLGPU3_InitInfo* info)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplSDLGPU3_Data* bd = IM_NEW(ImGui_ImplSDLGPU3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_sdlgpu3";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    IM_ASSERT(info->Device != nullptr);
    IM_ASSERT(info->ColorTargetFormat != SDL_GPU_TEXTUREFORMAT_INVALID);

    bd->InitInfo = *info;

    ImGui_ImplSDLGPU3_CreateDeviceObjects();

    return true;
}

void ImGui_ImplSDLGPU3_Shutdown()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDLGPU3_DestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

void ImGui_ImplSDLGPU3_NewFrame()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplSDLGPU3_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplSDLGPU3_CreateFontsTexture();
}

#endif // #ifndef IMGUI_DISABLE

// dear imgui: Renderer Backend for SDL_GPU
// This needs to be used along with the SDL3 Platform Backend

// Implemented features:
//  [X] Renderer: User texture binding. Use 'SDL_GPUTexture*' as texture identifier. Read the FAQ about ImTextureID/ImTextureRef! **IMPORTANT** Before 2025/08/08, ImTextureID was a reference to a SDL_GPUTextureSamplerBinding struct.
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Texture updates support for dynamic font atlas (ImGuiBackendFlags_RendererHasTextures).
//  [X] Renderer: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.

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
// - Unlike other backends, the user must call the function ImGui_ImplSDLGPU3_PrepareDrawData() BEFORE issuing a SDL_GPURenderPass containing ImGui_ImplSDLGPU3_RenderDrawData.
//   Calling the function is MANDATORY, otherwise the ImGui will not upload neither the vertex nor the index buffer for the GPU. See imgui_impl_sdlgpu3.cpp for more info.

// CHANGELOG
//  2025-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2025-09-18: Call platform_io.ClearRendererHandlers() on shutdown.
//  2025-08-20: Added ImGui_ImplSDLGPU3_InitInfo::SwapchainComposition and ImGui_ImplSDLGPU3_InitInfo::PresentMode to configure how secondary viewports are created.
//  2025-08-08: *BREAKING* Changed ImTextureID type from SDL_GPUTextureSamplerBinding* to SDL_GPUTexture*, which is more natural and easier for user to manage. If you need to change the current sampler, you can access the ImGui_ImplSDLGPU3_RenderState struct. (#8866, #8163, #7998, #7988)
//  2025-08-08: Expose SamplerDefault and SamplerCurrent in ImGui_ImplSDLGPU3_RenderState. Allow callback to change sampler.
//  2025-06-25: Mapping transfer buffer for texture update use cycle=true. Fixes artifacts e.g. on Metal backend.
//  2025-06-11: Added support for ImGuiBackendFlags_RendererHasTextures, for dynamic font atlas. Removed ImGui_ImplSDLGPU3_CreateFontsTexture() and ImGui_ImplSDLGPU3_DestroyFontsTexture().
//  2025-04-28: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2025-03-30: Made ImGui_ImplSDLGPU3_PrepareDrawData() reuse GPU Transfer Buffers which were unusually slow to recreate every frame. Much faster now.
//  2025-03-21: Fixed typo in function name Imgui_ImplSDLGPU3_PrepareDrawData() -> ImGui_ImplSDLGPU3_PrepareDrawData().
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
    SDL_GPUBuffer*          VertexBuffer            = nullptr;
    SDL_GPUTransferBuffer*  VertexTransferBuffer    = nullptr;
    uint32_t                VertexBufferSize        = 0;
    SDL_GPUBuffer*          IndexBuffer             = nullptr;
    SDL_GPUTransferBuffer*  IndexTransferBuffer     = nullptr;
    uint32_t                IndexBufferSize         = 0;
};

struct ImGui_ImplSDLGPU3_Data
{
    ImGui_ImplSDLGPU3_InitInfo   InitInfo;

    // Graphics pipeline & shaders
    SDL_GPUShader*               VertexShader           = nullptr;
    SDL_GPUShader*               FragmentShader         = nullptr;
    SDL_GPUGraphicsPipeline*     Pipeline               = nullptr;
    SDL_GPUSampler*              TexSamplerLinear       = nullptr;
    SDL_GPUTransferBuffer*       TexTransferBuffer      = nullptr;
    uint32_t                     TexTransferBufferSize  = 0;

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

static void ImGui_ImplSDLGPU3_SetupRenderState(ImDrawData* draw_data, ImGui_ImplSDLGPU3_RenderState* render_state, SDL_GPUGraphicsPipeline* pipeline, SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* render_pass, ImGui_ImplSDLGPU3_FrameData* fd, uint32_t fb_width, uint32_t fb_height)
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    render_state->SamplerCurrent = bd->TexSamplerLinear;

    // Bind graphics pipeline
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

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

static void CreateOrResizeBuffers(SDL_GPUBuffer** buffer, SDL_GPUTransferBuffer** transferbuffer, uint32_t* old_size, uint32_t new_size, SDL_GPUBufferUsageFlags usage)
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    // FIXME-OPT: Not optimal, but this is fairly rarely called.
    SDL_WaitForGPUIdle(v->Device);
    SDL_ReleaseGPUBuffer(v->Device, *buffer);
    SDL_ReleaseGPUTransferBuffer(v->Device, *transferbuffer);

    SDL_GPUBufferCreateInfo buffer_info = {};
    buffer_info.usage = usage;
    buffer_info.size = new_size;
    buffer_info.props = 0;
    *buffer = SDL_CreateGPUBuffer(v->Device, &buffer_info);
    *old_size = new_size;
    IM_ASSERT(*buffer != nullptr && "Failed to create GPU Buffer, call SDL_GetError() for more information");

    SDL_GPUTransferBufferCreateInfo transferbuffer_info = {};
    transferbuffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferbuffer_info.size = new_size;
    *transferbuffer = SDL_CreateGPUTransferBuffer(v->Device, &transferbuffer_info);
    IM_ASSERT(*transferbuffer != nullptr && "Failed to create GPU Transfer Buffer, call SDL_GetError() for more information");
}

// SDL_GPU doesn't allow copy passes to occur while a render or compute pass is bound!
// The only way to allow a user to supply their own RenderPass (to render to a texture instead of the window for example),
// is to split the upload part of ImGui_ImplSDLGPU3_RenderDrawData() to another function that needs to be called by the user before rendering.
void ImGui_ImplSDLGPU3_PrepareDrawData(ImDrawData* draw_data, SDL_GPUCommandBuffer* command_buffer)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount <= 0)
        return;

    // Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplSDLGPU3_UpdateTexture(tex);

    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;
    ImGui_ImplSDLGPU3_FrameData* fd = &bd->MainWindowFrameData;

    uint32_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    uint32_t index_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    if (fd->VertexBuffer == nullptr || fd->VertexBufferSize < vertex_size)
        CreateOrResizeBuffers(&fd->VertexBuffer, &fd->VertexTransferBuffer, &fd->VertexBufferSize, vertex_size, SDL_GPU_BUFFERUSAGE_VERTEX);
    if (fd->IndexBuffer == nullptr || fd->IndexBufferSize < index_size)
        CreateOrResizeBuffers(&fd->IndexBuffer, &fd->IndexTransferBuffer, &fd->IndexBufferSize, index_size, SDL_GPU_BUFFERUSAGE_INDEX);

    ImDrawVert* vtx_dst = (ImDrawVert*)SDL_MapGPUTransferBuffer(v->Device, fd->VertexTransferBuffer, true);
    ImDrawIdx* idx_dst = (ImDrawIdx*)SDL_MapGPUTransferBuffer(v->Device, fd->IndexTransferBuffer, true);
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        memcpy(vtx_dst, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += draw_list->VtxBuffer.Size;
        idx_dst += draw_list->IdxBuffer.Size;
    }
    SDL_UnmapGPUTransferBuffer(v->Device, fd->VertexTransferBuffer);
    SDL_UnmapGPUTransferBuffer(v->Device, fd->IndexTransferBuffer);

    SDL_GPUTransferBufferLocation vertex_buffer_location = {};
    vertex_buffer_location.offset = 0;
    vertex_buffer_location.transfer_buffer = fd->VertexTransferBuffer;
    SDL_GPUTransferBufferLocation index_buffer_location = {};
    index_buffer_location.offset = 0;
    index_buffer_location.transfer_buffer = fd->IndexTransferBuffer;

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

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Setup render state structure (for callbacks and custom texture bindings)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    ImGui_ImplSDLGPU3_RenderState render_state;
    render_state.Device = bd->InitInfo.Device;
    render_state.SamplerDefault = render_state.SamplerCurrent = bd->TexSamplerLinear;
    platform_io.Renderer_RenderState = &render_state;

    ImGui_ImplSDLGPU3_SetupRenderState(draw_data, &render_state, pipeline, command_buffer, render_pass, fd, fb_width, fb_height);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplSDLGPU3_SetupRenderState(draw_data, &render_state, pipeline, command_buffer, render_pass, fd, fb_width, fb_height);
                else
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
                SDL_GPUTextureSamplerBinding texture_sampler_binding;
                texture_sampler_binding.texture = (SDL_GPUTexture*)(intptr_t)pcmd->GetTexID();
                texture_sampler_binding.sampler = render_state.SamplerCurrent;
                SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);

                // Draw
                // **IF YOU GET A CRASH HERE** In 1.92.2 on 2025/08/08 we have changed ImTextureID to store 'SDL_GPUTexture*' instead of storing 'SDL_GPUTextureSamplerBinding'.
                // Any code loading custom texture using this backend needs to be updated.
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

static void ImGui_ImplSDLGPU3_DestroyTexture(ImTextureData* tex)
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    if (SDL_GPUTexture* raw_tex = (SDL_GPUTexture*)(intptr_t)tex->GetTexID())
        SDL_ReleaseGPUTexture(bd->InitInfo.Device, raw_tex);

    // Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
    tex->SetTexID(ImTextureID_Invalid);
    tex->SetStatus(ImTextureStatus_Destroyed);
}

void ImGui_ImplSDLGPU3_UpdateTexture(ImTextureData* tex)
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create and upload new texture to graphics system
        //IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

        // Create texture
        SDL_GPUTextureCreateInfo texture_info = {};
        texture_info.type = SDL_GPU_TEXTURETYPE_2D;
        texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texture_info.width = tex->Width;
        texture_info.height = tex->Height;
        texture_info.layer_count_or_depth = 1;
        texture_info.num_levels = 1;
        texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;

        SDL_GPUTexture* raw_tex = SDL_CreateGPUTexture(v->Device, &texture_info);
        IM_ASSERT(raw_tex != nullptr && "Failed to create texture, call SDL_GetError() for more info");

        // Store identifiers
        tex->SetTexID((ImTextureID)(intptr_t)raw_tex);
    }

    if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates)
    {
        SDL_GPUTexture* raw_tex = (SDL_GPUTexture*)(intptr_t)tex->GetTexID();
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

        // Update full texture or selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->UpdateRect but you can use tex->Updates[] to upload individual regions.
        // We could use the smaller rect on _WantCreate but using the full rect allows us to clear the texture.
        const int upload_x = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.x;
        const int upload_y = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.y;
        const int upload_w = (tex->Status == ImTextureStatus_WantCreate) ? tex->Width : tex->UpdateRect.w;
        const int upload_h = (tex->Status == ImTextureStatus_WantCreate) ? tex->Height : tex->UpdateRect.h;
        uint32_t upload_pitch = upload_w * tex->BytesPerPixel;
        uint32_t upload_size = upload_w * upload_h * tex->BytesPerPixel;

        // Create transfer buffer
        if (bd->TexTransferBufferSize < upload_size)
        {
            SDL_ReleaseGPUTransferBuffer(v->Device, bd->TexTransferBuffer);
            SDL_GPUTransferBufferCreateInfo transferbuffer_info = {};
            transferbuffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferbuffer_info.size = upload_size + 1024;
            bd->TexTransferBufferSize = upload_size + 1024;
            bd->TexTransferBuffer = SDL_CreateGPUTransferBuffer(v->Device, &transferbuffer_info);
            IM_ASSERT(bd->TexTransferBuffer != nullptr && "Failed to create transfer buffer, call SDL_GetError() for more information");
        }

        // Copy to transfer buffer
        {
            void* texture_ptr = SDL_MapGPUTransferBuffer(v->Device, bd->TexTransferBuffer, true);
            for (int y = 0; y < upload_h; y++)
                memcpy((void*)((uintptr_t)texture_ptr + y * upload_pitch), tex->GetPixelsAt(upload_x, upload_y + y), upload_pitch);
            SDL_UnmapGPUTransferBuffer(v->Device, bd->TexTransferBuffer);
        }

        SDL_GPUTextureTransferInfo transfer_info = {};
        transfer_info.offset = 0;
        transfer_info.transfer_buffer = bd->TexTransferBuffer;

        SDL_GPUTextureRegion texture_region = {};
        texture_region.texture = raw_tex;
        texture_region.x = (Uint32)upload_x;
        texture_region.y = (Uint32)upload_y;
        texture_region.w = (Uint32)upload_w;
        texture_region.h = (Uint32)upload_h;
        texture_region.d = 1;

        // Upload
        {
            SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(v->Device);
            SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
            SDL_UploadToGPUTexture(copy_pass, &transfer_info, &texture_region, false);
            SDL_EndGPUCopyPass(copy_pass);
            SDL_SubmitGPUCommandBuffer(cmd);
        }

        tex->SetStatus(ImTextureStatus_OK);
    }
    if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0)
        ImGui_ImplSDLGPU3_DestroyTexture(tex);
}

static void ImGui_ImplSDLGPU3_CreateShaders()
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
    ImGui_ImplSDLGPU3_CreateShaders();

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

    ImGui_ImplSDLGPU3_DestroyDeviceObjects();

    if (bd->TexSamplerLinear == nullptr)
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

        bd->TexSamplerLinear = SDL_CreateGPUSampler(v->Device, &sampler_info);
        IM_ASSERT(bd->TexSamplerLinear != nullptr && "Failed to create sampler, call SDL_GetError() for more information");
    }

    ImGui_ImplSDLGPU3_CreateGraphicsPipeline();
}

void ImGui_ImplSDLGPU3_DestroyFrameData()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    ImGui_ImplSDLGPU3_FrameData* fd = &bd->MainWindowFrameData;
    SDL_ReleaseGPUBuffer(v->Device, fd->VertexBuffer);
    SDL_ReleaseGPUBuffer(v->Device, fd->IndexBuffer);
    SDL_ReleaseGPUTransferBuffer(v->Device, fd->VertexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(v->Device, fd->IndexTransferBuffer);
    fd->VertexBuffer = fd->IndexBuffer = nullptr;
    fd->VertexTransferBuffer = fd->IndexTransferBuffer = nullptr;
    fd->VertexBufferSize = fd->IndexBufferSize = 0;
}

void ImGui_ImplSDLGPU3_DestroyDeviceObjects()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    ImGui_ImplSDLGPU3_InitInfo* v = &bd->InitInfo;

    ImGui_ImplSDLGPU3_DestroyFrameData();

    // Destroy all textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1)
            ImGui_ImplSDLGPU3_DestroyTexture(tex);
    if (bd->TexTransferBuffer)  { SDL_ReleaseGPUTransferBuffer(v->Device, bd->TexTransferBuffer); bd->TexTransferBuffer = nullptr; }
    if (bd->VertexShader)       { SDL_ReleaseGPUShader(v->Device, bd->VertexShader); bd->VertexShader = nullptr; }
    if (bd->FragmentShader)     { SDL_ReleaseGPUShader(v->Device, bd->FragmentShader); bd->FragmentShader = nullptr; }
    if (bd->TexSamplerLinear)   { SDL_ReleaseGPUSampler(v->Device, bd->TexSamplerLinear); bd->TexSamplerLinear = nullptr; }
    if (bd->Pipeline)           { SDL_ReleaseGPUGraphicsPipeline(v->Device, bd->Pipeline); bd->Pipeline = nullptr; }
}

static void ImGui_ImplSDLGPU3_InitMultiViewportSupport();
static void ImGui_ImplSDLGPU3_ShutdownMultiViewportSupport();

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
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // We can honor ImGuiPlatformIO::Textures[] requests during render.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    IM_ASSERT(info->Device != nullptr);
    IM_ASSERT(info->ColorTargetFormat != SDL_GPU_TEXTUREFORMAT_INVALID);

    bd->InitInfo = *info;

    ImGui_ImplSDLGPU3_InitMultiViewportSupport();

    return true;
}

void ImGui_ImplSDLGPU3_Shutdown()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGui_ImplSDLGPU3_ShutdownMultiViewportSupport();
    ImGui_ImplSDLGPU3_DestroyDeviceObjects();

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasViewports);
    platform_io.ClearRendererHandlers();
    IM_DELETE(bd);
}

void ImGui_ImplSDLGPU3_NewFrame()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplSDLGPU3_Init()?");

    if (!bd->TexSamplerLinear)
        ImGui_ImplSDLGPU3_CreateDeviceObjects();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

static void ImGui_ImplSDLGPU3_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplSDLGPU3_Data* data = ImGui_ImplSDLGPU3_GetBackendData();
    SDL_Window* window = SDL_GetWindowFromID((SDL_WindowID)(intptr_t)viewport->PlatformHandle);
    SDL_ClaimWindowForGPUDevice(data->InitInfo.Device, window);
    SDL_SetGPUSwapchainParameters(data->InitInfo.Device, window, data->InitInfo.SwapchainComposition, data->InitInfo.PresentMode);
    viewport->RendererUserData = (void*)1;
}

static void ImGui_ImplSDLGPU3_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGui_ImplSDLGPU3_Data* data = ImGui_ImplSDLGPU3_GetBackendData();
    SDL_Window* window = SDL_GetWindowFromID((SDL_WindowID)(intptr_t)viewport->PlatformHandle);

    ImDrawData* draw_data = viewport->DrawData;

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(data->InitInfo.Device);

    SDL_GPUTexture* swapchain_texture;
    SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr);

    if (swapchain_texture != nullptr)
    {
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer); // FIXME-OPT: Not optimal, may this be done earlier?
        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture;
        target_info.clear_color = SDL_FColor{ 0.0f,0.0f,0.0f,1.0f };
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

static void ImGui_ImplSDLGPU3_DestroyWindow(ImGuiViewport* viewport)
{
    ImGui_ImplSDLGPU3_Data* data = ImGui_ImplSDLGPU3_GetBackendData();
    if (viewport->RendererUserData)
    {
        SDL_Window* window = SDL_GetWindowFromID((SDL_WindowID)(intptr_t)viewport->PlatformHandle);
        SDL_ReleaseWindowFromGPUDevice(data->InitInfo.Device, window);
    }
    viewport->RendererUserData = nullptr;
}

static void ImGui_ImplSDLGPU3_InitMultiViewportSupport()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_RenderWindow = ImGui_ImplSDLGPU3_RenderWindow;
    platform_io.Renderer_CreateWindow = ImGui_ImplSDLGPU3_CreateWindow;
    platform_io.Renderer_DestroyWindow = ImGui_ImplSDLGPU3_DestroyWindow;
}

static void ImGui_ImplSDLGPU3_ShutdownMultiViewportSupport()
{
    ImGui::DestroyPlatformWindows();
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE

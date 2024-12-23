// dear imgui: Renderer Backend for SDL_GPU for SDL3
// (Requires: SDL 3.0.0+)

// (**IMPORTANT: SDL 3.0.0 is NOT YET RELEASED AND CURRENTLY HAS A FAST CHANGING API. THIS CODE BREAKS OFTEN AS SDL3 CHANGES.**)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'SDL_GPUTexture*' as ImTextureID. Read the FAQ about ImTextureID!
//  [ ] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.
// Missing features:
//  [ ] Renderer: Multi-viewport support (multiple windows).

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
//  2024-09-19: Initial version.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_sdlgpu3.h"
#include <stdint.h>  // intptr_t

// Clang warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"  // warning: implicit conversion changes signedness
#endif

// SDL
#include <SDL3/SDL.h>
#if !SDL_VERSION_ATLEAST(3, 0, 0)
#error This backend requires SDL 3.0.0+
#endif
#include <SDL3/SDL_gpu.h>

// SDL_GPUDevice data
struct ImGui_ImplSDLGPU3_Data
{
    SDL_GPUDevice* Device;  // Main viewport's renderer
    SDL_Window* Window;
    SDL_GPUTexture* FontTexture;
    SDL_GPUSampler* FontSampler;

    SDL_GPUGraphicsPipeline* Pipeline;
    SDL_GPUShader* ShaderModuleVert;
    SDL_GPUShader* ShaderModuleFrag;

    size_t VertexBufferSize;
    size_t IndexBufferSize;
    SDL_GPUBuffer* VertexBuffer;
    SDL_GPUBuffer* IndexBuffer;

    ImGui_ImplSDLGPU3_Data() { memset((void*)this, 0, sizeof(*this)); }
};

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// backends/vulkan/glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(set = 1, binding = 0) uniform UBO { vec2 uScale; vec2 uTranslate; };

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * uScale + uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x000a000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000000f, 0x00000015,
    0x0000001b, 0x0000001c, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00030005, 0x00000009, 0x00000000,
    0x00050006, 0x00000009, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x00000009, 0x00000001, 0x00005655, 0x00030005, 0x0000000b, 0x0074754f,
    0x00040005, 0x0000000f, 0x6c6f4361, 0x0000726f, 0x00030005, 0x00000015, 0x00565561, 0x00060005, 0x00000019, 0x505f6c67, 0x65567265, 0x78657472,
    0x00000000, 0x00060006, 0x00000019, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000001b, 0x00000000, 0x00040005, 0x0000001c,
    0x736f5061, 0x00000000, 0x00030005, 0x0000001e, 0x004f4255, 0x00050006, 0x0000001e, 0x00000000, 0x61635375, 0x0000656c, 0x00060006, 0x0000001e,
    0x00000001, 0x61725475, 0x616c736e, 0x00006574, 0x00030005, 0x00000020, 0x00000000, 0x00040047, 0x0000000b, 0x0000001e, 0x00000000, 0x00040047,
    0x0000000f, 0x0000001e, 0x00000002, 0x00040047, 0x00000015, 0x0000001e, 0x00000001, 0x00050048, 0x00000019, 0x00000000, 0x0000000b, 0x00000000,
    0x00030047, 0x00000019, 0x00000002, 0x00040047, 0x0000001c, 0x0000001e, 0x00000000, 0x00050048, 0x0000001e, 0x00000000, 0x00000023, 0x00000000,
    0x00050048, 0x0000001e, 0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x0000001e, 0x00000002, 0x00040047, 0x00000020, 0x00000022, 0x00000001,
    0x00040047, 0x00000020, 0x00000021, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020,
    0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040017, 0x00000008, 0x00000006, 0x00000002, 0x0004001e, 0x00000009, 0x00000007, 0x00000008,
    0x00040020, 0x0000000a, 0x00000003, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00040015, 0x0000000c, 0x00000020, 0x00000001,
    0x0004002b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040020, 0x0000000e, 0x00000001, 0x00000007, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000001,
    0x00040020, 0x00000011, 0x00000003, 0x00000007, 0x0004002b, 0x0000000c, 0x00000013, 0x00000001, 0x00040020, 0x00000014, 0x00000001, 0x00000008,
    0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00040020, 0x00000017, 0x00000003, 0x00000008, 0x0003001e, 0x00000019, 0x00000007, 0x00040020,
    0x0000001a, 0x00000003, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b, 0x00000003, 0x0004003b, 0x00000014, 0x0000001c, 0x00000001, 0x0004001e,
    0x0000001e, 0x00000008, 0x00000008, 0x00040020, 0x0000001f, 0x00000002, 0x0000001e, 0x0004003b, 0x0000001f, 0x00000020, 0x00000002, 0x00040020,
    0x00000021, 0x00000002, 0x00000008, 0x0004002b, 0x00000006, 0x00000028, 0x00000000, 0x0004002b, 0x00000006, 0x00000029, 0x3f800000, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000007, 0x00000010, 0x0000000f, 0x00050041, 0x00000011,
    0x00000012, 0x0000000b, 0x0000000d, 0x0003003e, 0x00000012, 0x00000010, 0x0004003d, 0x00000008, 0x00000016, 0x00000015, 0x00050041, 0x00000017,
    0x00000018, 0x0000000b, 0x00000013, 0x0003003e, 0x00000018, 0x00000016, 0x0004003d, 0x00000008, 0x0000001d, 0x0000001c, 0x00050041, 0x00000021,
    0x00000022, 0x00000020, 0x0000000d, 0x0004003d, 0x00000008, 0x00000023, 0x00000022, 0x00050085, 0x00000008, 0x00000024, 0x0000001d, 0x00000023,
    0x00050041, 0x00000021, 0x00000025, 0x00000020, 0x00000013, 0x0004003d, 0x00000008, 0x00000026, 0x00000025, 0x00050081, 0x00000008, 0x00000027,
    0x00000024, 0x00000026, 0x00050051, 0x00000006, 0x0000002a, 0x00000027, 0x00000000, 0x00050051, 0x00000006, 0x0000002b, 0x00000027, 0x00000001,
    0x00070050, 0x00000007, 0x0000002c, 0x0000002a, 0x0000002b, 0x00000028, 0x00000029, 0x00050041, 0x00000011, 0x0000002d, 0x0000001b, 0x0000000d,
    0x0003003e, 0x0000002d, 0x0000002c, 0x000100fd, 0x00010038};

// backends/vulkan/glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=2, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x00000009, 0x6c6f4366,
    0x0000726f, 0x00030005, 0x0000000b, 0x00000000, 0x00050006, 0x0000000b, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x0000000b, 0x00000001,
    0x00005655, 0x00030005, 0x0000000d, 0x00006e49, 0x00050005, 0x00000016, 0x78655473, 0x65727574, 0x00000000, 0x00040047, 0x00000009, 0x0000001e,
    0x00000000, 0x00040047, 0x0000000d, 0x0000001e, 0x00000000, 0x00040047, 0x00000016, 0x00000022, 0x00000002, 0x00040047, 0x00000016, 0x00000021,
    0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006,
    0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006,
    0x00000002, 0x0004001e, 0x0000000b, 0x00000007, 0x0000000a, 0x00040020, 0x0000000c, 0x00000001, 0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d,
    0x00000001, 0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040020, 0x00000010, 0x00000001,
    0x00000007, 0x00090019, 0x00000013, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001b, 0x00000014,
    0x00000013, 0x00040020, 0x00000015, 0x00000000, 0x00000014, 0x0004003b, 0x00000015, 0x00000016, 0x00000000, 0x0004002b, 0x0000000e, 0x00000018,
    0x00000001, 0x00040020, 0x00000019, 0x00000001, 0x0000000a, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005,
    0x00050041, 0x00000010, 0x00000011, 0x0000000d, 0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0004003d, 0x00000014, 0x00000017,
    0x00000016, 0x00050041, 0x00000019, 0x0000001a, 0x0000000d, 0x00000018, 0x0004003d, 0x0000000a, 0x0000001b, 0x0000001a, 0x00050057, 0x00000007,
    0x0000001c, 0x00000017, 0x0000001b, 0x00050085, 0x00000007, 0x0000001d, 0x00000012, 0x0000001c, 0x0003003e, 0x00000009, 0x0000001d, 0x000100fd,
    0x00010038};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple
// Dear ImGui contexts.
static ImGui_ImplSDLGPU3_Data* ImGui_ImplSDLGPU3_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplSDLGPU3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static void ImGui_ImplSDLGPU3_CreateShaderModules()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();

    const char* driver = SDL_GetGPUDeviceDriver(bd->Device);
    if(bd->ShaderModuleVert == nullptr)
    {
        SDL_GPUShaderCreateInfo vert_info{};
        vert_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        vert_info.entrypoint = "main";
        vert_info.num_uniform_buffers = 1;  // UBO set 1 binding 0

        if(strcmp(driver, "vulkan") == 0)
        {
            vert_info.code_size = sizeof(__glsl_shader_vert_spv);
            vert_info.code = (uint8_t*)__glsl_shader_vert_spv;
            vert_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        }
        // TODO: Add other shader drivers here

        bd->ShaderModuleVert = SDL_CreateGPUShader(bd->Device, &vert_info);
        if(bd->ShaderModuleVert == nullptr)
        {
            SDL_Log("error creating vertex shader module: %s", SDL_GetError());
        }
    }

    if(bd->ShaderModuleFrag == nullptr)
    {
        SDL_GPUShaderCreateInfo frag_info{};
        frag_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        frag_info.entrypoint = "main";
        frag_info.num_samplers = 1;  // sTexture set 0 binding 0

        if(strcmp(driver, "vulkan") == 0)
        {
            frag_info.code_size = sizeof(__glsl_shader_frag_spv);
            frag_info.code = (uint8_t*)__glsl_shader_frag_spv;
            frag_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        }
        // TODO: Add other shader drivers here

        bd->ShaderModuleFrag = SDL_CreateGPUShader(bd->Device, &frag_info);
        if(bd->ShaderModuleFrag == nullptr)
        {
            SDL_Log("error creating fragment shader module: %s", SDL_GetError());
        }
    }
}

static void ImGui_ImplSDLGPU3_DestroyRenderPipeline()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    if(bd->Pipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(bd->Device, bd->Pipeline);
        bd->Pipeline = nullptr;
    }
}

static void ImGui_ImplSDLGPU3_CreateRenderPipeline()
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();

    ImGui_ImplSDLGPU3_CreateShaderModules();
    ImGui_ImplSDLGPU3_DestroyRenderPipeline();

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};

    SDL_GPUColorTargetDescription color_attachment_desc{};
    color_attachment_desc.blend_state.color_write_mask =
        SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
    color_attachment_desc.blend_state.enable_blend = true;
    color_attachment_desc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_attachment_desc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment_desc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color_attachment_desc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_attachment_desc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment_desc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    color_attachment_desc.format = SDL_GetGPUSwapchainTextureFormat(bd->Device, bd->Window);

    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_attachment_desc;
    pipeline_info.target_info.has_depth_stencil_target = false;

    pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipeline_info.multisample_state.sample_mask = 0xf;

    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pipeline_info.vertex_shader = bd->ShaderModuleVert;
    pipeline_info.fragment_shader = bd->ShaderModuleFrag;

    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;

    SDL_GPUVertexBufferDescription binding_desc[1]{};
    binding_desc[0].slot = 0;
    binding_desc[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    binding_desc[0].instance_step_rate = 0;
    binding_desc[0].pitch = sizeof(ImDrawVert);

    SDL_GPUVertexAttribute attribute_desc[3] = {};
    attribute_desc[0].location = 0;
    attribute_desc[0].buffer_slot = binding_desc[0].slot;
    attribute_desc[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attribute_desc[0].offset = offsetof(ImDrawVert, pos);
    attribute_desc[1].location = 1;
    attribute_desc[1].buffer_slot = binding_desc[0].slot;
    attribute_desc[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attribute_desc[1].offset = offsetof(ImDrawVert, uv);
    attribute_desc[2].location = 2;
    attribute_desc[2].buffer_slot = binding_desc[0].slot;
    attribute_desc[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    attribute_desc[2].offset = offsetof(ImDrawVert, col);

    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = binding_desc;
    pipeline_info.vertex_input_state.num_vertex_attributes = 3;
    pipeline_info.vertex_input_state.vertex_attributes = attribute_desc;

    bd->Pipeline = SDL_CreateGPUGraphicsPipeline(bd->Device, &pipeline_info);
    if(bd->Pipeline == nullptr)
    {
        SDL_Log("error creating graphics pipeline: %s", SDL_GetError());
    }
}

static void ImGui_ImplSDLGPU3_SetupRenderState(ImDrawData* draw_data, SDL_GPUGraphicsPipeline* pipeline, SDL_GPUCommandBuffer* command_buffer,
                                              SDL_GPURenderPass* render_pass, int fb_width, int fb_height)
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();

    // Bind pipeline:
    {
        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    }

    // Bind Vertex And Index Buffer:
    if(draw_data->TotalVtxCount > 0)
    {
        SDL_GPUBufferBinding buffer_binding{};
        buffer_binding.buffer = bd->VertexBuffer;
        SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);

        buffer_binding.buffer = bd->IndexBuffer;
        SDL_BindGPUIndexBuffer(render_pass, &buffer_binding,
                               sizeof(ImDrawIdx) == 2 ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);

        // vkCmdBindIndexBuffer(command_buffer, rb->IndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    // Setup viewport:
    {
        SDL_GPUViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.w = (float)fb_width;
        viewport.h = (float)fb_height;
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        SDL_SetGPUViewport(render_pass, &viewport);

        SDL_Rect scissor{};
        scissor.x = 0;
        scissor.y = 0;
        scissor.w = viewport.w;
        scissor.h = viewport.h;
        SDL_SetGPUScissor(render_pass, &scissor);
    }

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos
    // is (0,0) for single viewport apps.
    {
        float push_data[4]{};
        push_data[0] = 2.0f / draw_data->DisplaySize.x;                 // scale
        push_data[1] = -2.0f / draw_data->DisplaySize.y;                // scale
        push_data[2] = -1.0f - draw_data->DisplayPos.x * push_data[0];  // translate
        push_data[3] = 1.0f - draw_data->DisplayPos.y * push_data[1];   // translate
        SDL_PushGPUVertexUniformData(command_buffer, 0, push_data, sizeof(float) * 4);
    }
}

// Functions
bool ImGui_ImplSDLGPU3_Init(SDL_GPUDevice* device, SDL_Window* window)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    IM_ASSERT(device != nullptr && "SDL_Gpu not initialized!");

    // Setup backend capabilities flags
    ImGui_ImplSDLGPU3_Data* bd = IM_NEW(ImGui_ImplSDLGPU3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_sdlgpu3";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    bd->Device = device;
    bd->Window = window;

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

    if(!bd->FontTexture) ImGui_ImplSDLGPU3_CreateDeviceObjects();
}

void ImGui_ImplSDLGPU3_RenderDrawData(ImDrawData* draw_data, SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* render_target)
{
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();

    // If there's a scale factor set by the user, use that instead
    // If the user has specified a scale factor to SDL_Renderer already via SDL_RenderSetScale(), SDL will scale whatever we pass
    // to SDL_RenderGeometryRaw() by that scale factor. In that case we don't want to be also scaling it ourselves here.
    float rsx = 1.0f;
    float rsy = 1.0f;
    // SDL_GetRenderScale(renderer, &rsx, &rsy);
    ImVec2 render_scale;
    render_scale.x = (rsx == 1.0f) ? draw_data->FramebufferScale.x : 1.0f;
    render_scale.y = (rsy == 1.0f) ? draw_data->FramebufferScale.y : 1.0f;

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * render_scale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * render_scale.y);
    if(fb_width == 0 || fb_height == 0) return;

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;  // (0,0) unless using multi-viewports
    ImVec2 clip_scale = render_scale;

    // Copy over the vertices and indices
    if(draw_data->TotalVtxCount > 0)
    {
        // Create or resize the vertex/index buffers
        size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        if(bd->VertexBuffer == nullptr || bd->VertexBufferSize < vertex_size)
        {
            if(bd->VertexBuffer != nullptr)
            {
                SDL_ReleaseGPUBuffer(bd->Device, bd->VertexBuffer);
                bd->VertexBuffer = nullptr;
            }
            SDL_GPUBufferCreateInfo info{};
            info.size = vertex_size;
            info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            bd->VertexBuffer = SDL_CreateGPUBuffer(bd->Device, &info);
            bd->VertexBufferSize = vertex_size;
            SDL_SetGPUBufferName(bd->Device, bd->VertexBuffer, "ImguiVertexBuffer");
        }
        if(bd->IndexBuffer == nullptr || bd->IndexBufferSize < index_size)
        {
            if(bd->IndexBuffer != nullptr)
            {
                SDL_ReleaseGPUBuffer(bd->Device, bd->IndexBuffer);
                bd->IndexBuffer = nullptr;
            }
            SDL_GPUBufferCreateInfo info{};
            info.size = index_size;
            info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            bd->IndexBuffer = SDL_CreateGPUBuffer(bd->Device, &info);
            bd->IndexBufferSize = index_size;
            SDL_SetGPUBufferName(bd->Device, bd->IndexBuffer, "ImguiIndexBuffer");
        }

        // TODO: Reuse the transfer buffer?
        SDL_GPUTransferBufferCreateInfo transfer_info{};
        transfer_info.size = vertex_size + index_size;
        transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(bd->Device, &transfer_info);

        void* map = SDL_MapGPUTransferBuffer(bd->Device, transfer_buffer, false);

        // Upload vertex/index data into a single contiguous GPU buffer
        ImDrawVert* vtx_dst = reinterpret_cast<ImDrawVert*>(map);
        ImDrawIdx* idx_dst = reinterpret_cast<ImDrawIdx*>(reinterpret_cast<char*>(map) + vertex_size);

        for(int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        SDL_UnmapGPUTransferBuffer(bd->Device, transfer_buffer);

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTransferBufferLocation vertex_location{};
        vertex_location.transfer_buffer = transfer_buffer;
        vertex_location.offset = 0;

        SDL_GPUTransferBufferLocation index_location{};
        index_location.transfer_buffer = transfer_buffer;
        index_location.offset = vertex_size;

        SDL_GPUBufferRegion vertex_region{};
        vertex_region.buffer = bd->VertexBuffer;
        vertex_region.offset = 0;
        vertex_region.size = vertex_size;

        SDL_GPUBufferRegion index_region{};
        index_region.buffer = bd->IndexBuffer;
        index_region.offset = 0;
        index_region.size = index_size;

        SDL_UploadToGPUBuffer(copy_pass, &vertex_location, &vertex_region, true);
        SDL_UploadToGPUBuffer(copy_pass, &index_location, &index_region, true);
        SDL_EndGPUCopyPass(copy_pass);

        SDL_ReleaseGPUTransferBuffer(bd->Device, transfer_buffer);
    }

    SDL_GPUColorTargetInfo color_target_info{};
    color_target_info.texture = render_target;
    color_target_info.load_op = SDL_GPU_LOADOP_LOAD;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd, &color_target_info, 1, nullptr);

    ImGui_ImplSDLGPU3_SetupRenderState(draw_data, bd->Pipeline, cmd, render_pass, fb_width, fb_height);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for(int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if(pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if(pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplSDLGPU3_SetupRenderState(draw_data, bd->Pipeline, cmd, render_pass, fb_width, fb_height);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
                if(clip_min.x < 0.0f)
                {
                    clip_min.x = 0.0f;
                }
                if(clip_min.y < 0.0f)
                {
                    clip_min.y = 0.0f;
                }
                if(clip_max.x > fb_width)
                {
                    clip_max.x = (float)fb_width;
                }
                if(clip_max.y > fb_height)
                {
                    clip_max.y = (float)fb_height;
                }
                if(clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) continue;

                // Apply scissor/clipping rectangle
                SDL_Rect scissor;
                scissor.x = (int32_t)(clip_min.x);
                scissor.y = (int32_t)(clip_min.y);
                scissor.w = (int32_t)(clip_max.x - clip_min.x);
                scissor.h = (int32_t)(clip_max.y - clip_min.y);

                SDL_SetGPUScissor(render_pass, &scissor);

                // Bind DescriptorSet with font or user texture
                SDL_GPUTextureSamplerBinding texture_binding{};
                texture_binding.sampler = bd->FontSampler;
                texture_binding.texture = (SDL_GPUTexture*)pcmd->TextureId;
                if(sizeof(ImTextureID) < sizeof(ImU64))
                {
                    // We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures
                    // haven't been used.
                    IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontTexture);
                    texture_binding.texture = bd->FontTexture;
                }
                SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);

                // Draw
                SDL_DrawGPUIndexedPrimitives(render_pass, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset,
                                             pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    SDL_EndGPURenderPass(render_pass);
}

// Called by Init/NewFrame/Shutdown
bool ImGui_ImplSDLGPU3_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width,
                                 &height);  // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more
                                            // likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level
                                            // concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to
    // allow point/nearest sampling)
    SDL_GPUTextureCreateInfo info{};
    info.width = width;
    info.height = height;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    info.num_levels = 1;
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.layer_count_or_depth = 1;
    bd->FontTexture = SDL_CreateGPUTexture(bd->Device, &info);
    if(bd->FontTexture == nullptr)
    {
        SDL_Log("error creating texture: %s", SDL_GetError());
        return false;
    }

    // First transfer the data to a transfer buffer
    SDL_GPUTransferBufferCreateInfo transfer_create_info{};
    transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_create_info.size = 4 * width * height;
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(bd->Device, &transfer_create_info);
    if(!transfer_buffer)
    {
        SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
        return false;
    }

    void* map = SDL_MapGPUTransferBuffer(bd->Device, transfer_buffer, false);
    SDL_memcpy(map, pixels, transfer_create_info.size);
    SDL_UnmapGPUTransferBuffer(bd->Device, transfer_buffer);

    SDL_GPUTextureLocation location{};

    auto* cmd = SDL_AcquireGPUCommandBuffer(bd->Device);
    auto* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo transfer_info{};
    transfer_info.transfer_buffer = transfer_buffer;
    transfer_info.pixels_per_row = width;
    transfer_info.rows_per_layer = height;
    transfer_info.offset = 0;

    SDL_GPUTextureRegion region{};
    region.texture = bd->FontTexture;
    region.x = 0;
    region.y = 0;
    region.z = 0;
    region.w = width;
    region.h = height;
    region.d = 1;

    SDL_UploadToGPUTexture(copy_pass, &transfer_info, &region, false);
    SDL_EndGPUCopyPass(copy_pass);

    // Submit the commands
    SDL_SubmitGPUCommandBuffer(cmd);

    // No need for the transfer buffer now
    SDL_ReleaseGPUTransferBuffer(bd->Device, transfer_buffer);

    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.address_mode_u = sampler_info.address_mode_v = sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.mag_filter = sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;

    bd->FontSampler = SDL_CreateGPUSampler(bd->Device, &sampler_info);
    if(!bd->FontSampler)
    {
        SDL_Log("error creating sampler: %s", SDL_GetError());
        return false;
    }

    // SDL_SetTextureBlendMode(bd->FontTexture, SDL_BLENDMODE_BLEND);
    // SDL_SetTextureScaleMode(bd->FontTexture, SDL_SCALEMODE_LINEAR);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

    return true;
}

void ImGui_ImplSDLGPU3_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    if(bd->FontTexture)
    {
        io.Fonts->SetTexID(0);

        SDL_ReleaseGPUTexture(bd->Device, bd->FontTexture);
        SDL_ReleaseGPUSampler(bd->Device, bd->FontSampler);
        bd->FontTexture = nullptr;
        bd->FontSampler = nullptr;
    }
}

bool ImGui_ImplSDLGPU3_CreateDeviceObjects()
{
    ImGui_ImplSDLGPU3_CreateRenderPipeline();
    return ImGui_ImplSDLGPU3_CreateFontsTexture();
}

void ImGui_ImplSDLGPU3_DestroyDeviceObjects()
{
    ImGui_ImplSDLGPU3_DestroyRenderPipeline();
    ImGui_ImplSDLGPU3_DestroyFontsTexture();

    ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
    if(bd->VertexBuffer)
    {
        SDL_ReleaseGPUBuffer(bd->Device, bd->VertexBuffer);
        bd->VertexBuffer = nullptr;
    }

    if(bd->IndexBuffer)
    {
        SDL_ReleaseGPUBuffer(bd->Device, bd->IndexBuffer);
        bd->IndexBuffer = nullptr;
    }

    if(bd->ShaderModuleVert)
    {
        SDL_ReleaseGPUShader(bd->Device, bd->ShaderModuleVert);
        bd->ShaderModuleVert = nullptr;
    }

    if(bd->ShaderModuleFrag)
    {
        SDL_ReleaseGPUShader(bd->Device, bd->ShaderModuleFrag);
        bd->ShaderModuleFrag = nullptr;
    }
}

//-----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif  // #ifndef IMGUI_DISABLE

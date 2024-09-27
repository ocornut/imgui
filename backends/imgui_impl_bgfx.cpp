// Copyright 2010-2022 Luca Lolli
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
// File changes (yyyy-mm-dd)
// 2024-09-07: Vlad Kirilin: convert to glfw

#include "imgui.h"
#include "imgui_impl_bgfx.h"

// BGFX/BX
#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <bx/timer.h>

#if GLFW_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif GLFW_PLATFORM_COCOA
#define GLFW_EXPOSE_NATIVE_COCOA
#elif GLFW_PLATFORM_X11
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <vector>
#include <GLFW/glfw3native.h>

// Data
static bgfx::TextureHandle fontTexture = BGFX_INVALID_HANDLE;
static bgfx::ProgramHandle shaderHandle = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle attribLocationTex = BGFX_INVALID_HANDLE;
static bgfx::VertexLayout vertexLayout;

static std::vector<bgfx::ViewId> freeViewIds;
static bgfx::ViewId subViewId = 100;

bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout &_layout,
                                uint32_t _numIndices)
{
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout) &&
           (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices));
}

enum class BgfxTextureFlags : uint32_t
{
    Opaque = 1u << 31,
    PointSampler = 1u << 30,
    All = Opaque | PointSampler,
};

// This is the main rendering function that you have to implement and call after
// ImGui::Render(). Pass ImGui::GetDrawData() to this function.
// Note: If text or lines are blurry when integrating ImGui into your engine,
// in your Render function, try translating your projection matrix by
// (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_Implbgfx_RenderDrawLists(struct ImDrawData* draw_data, bgfx::ViewId view_id /*= 255*/)
{
    if (draw_data->DisplaySize.x <= 0 || draw_data->DisplaySize.y <= 0)
        return;

    // Set basic display parameters
    bgfx::setViewName(view_id, "ImGui");
    bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);
    bgfx::touch(view_id);

    const ImVec2 clip_position = draw_data->DisplayPos;
    const ImVec2 clip_size = draw_data->DisplaySize;
    const ImVec2 clip_scale = draw_data->FramebufferScale;

    const float L = clip_position.x;
    const float R = L + clip_size.x;
    const float T = clip_position.y;
    const float B = T + clip_size.y;

    float ortho[16];
    bx::mtxOrtho(ortho, L, R, B, T, 0.0f, 1000.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(view_id, nullptr, ortho);
    bgfx::setViewRect(view_id, 0, 0, uint16_t(clip_size.x * clip_scale.x), uint16_t(clip_size.y * clip_scale.y));

    bgfx::Encoder* encoder = bgfx::begin();

    for (int32_t cmdListIndex = 0; cmdListIndex < draw_data->CmdListsCount; ++cmdListIndex)
    {
        const ImDrawList* drawList = draw_data->CmdLists[cmdListIndex];
        uint32_t numVertices = static_cast<uint32_t>(drawList->VtxBuffer.size());
        uint32_t numIndices = static_cast<uint32_t>(drawList->IdxBuffer.size());

        if (!checkAvailTransientBuffers(numVertices, vertexLayout, numIndices))
            continue;  // If there is not enough space in the buffer, skip this list of commands

        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;
        bgfx::allocTransientVertexBuffer(&tvb, numVertices, vertexLayout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

        // Copy vertex and index data
        memcpy(tvb.data, drawList->VtxBuffer.Data, numVertices * sizeof(ImDrawVert));
        memcpy(tib.data, drawList->IdxBuffer.Data, numIndices * sizeof(ImDrawIdx));

        uint32_t offset = 0;

        for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(); cmd != drawList->CmdBuffer.end(); ++cmd)
        {
            if (cmd->UserCallback)
            {
                cmd->UserCallback(drawList, cmd);
            }
            else if (cmd->ElemCount > 0)
            {
                uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
                uint32_t sampler_state = 0;

                bgfx::TextureHandle texture_handle = fontTexture;
                bool alphaBlend = true;

                if (cmd->TextureId != nullptr)
                {
                    auto textureInfo = (uintptr_t)cmd->TextureId;
                    if (textureInfo & (uint32_t)BgfxTextureFlags::Opaque)
                        alphaBlend = false;
                    if (textureInfo & (uint32_t)BgfxTextureFlags::PointSampler)
                        sampler_state = BGFX_SAMPLER_POINT;

                    textureInfo &= ~(uint32_t)BgfxTextureFlags::All;
                    texture_handle = { (uint16_t)textureInfo };
                }

                if (alphaBlend)
                {
                    state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                }

                // Set scissor/clipping rectangles in framebuffer space
                const ImVec4 clipRect(
                    (cmd->ClipRect.x - clip_position.x) * clip_scale.x,
                    (cmd->ClipRect.y - clip_position.y) * clip_scale.y,
                    (cmd->ClipRect.z - clip_position.x) * clip_scale.x,
                    (cmd->ClipRect.w - clip_position.y) * clip_scale.y
                );

                if (clipRect.x < clip_size.x && clipRect.y < clip_size.y &&
                    clipRect.z >= 0.0f && clipRect.w >= 0.0f)
                {
                    const uint16_t scissorX = uint16_t(bx::max(clipRect.x, 0.0f));
                    const uint16_t scissorY = uint16_t(bx::max(clipRect.y, 0.0f));
                    const uint16_t scissorW = uint16_t(bx::min(clipRect.z - scissorX, 65535.0f));
                    const uint16_t scissorH = uint16_t(bx::min(clipRect.w - scissorY, 65535.0f));

                    encoder->setScissor(scissorX, scissorY, scissorW, scissorH);
                    encoder->setState(state);
                    encoder->setTexture(0, attribLocationTex, texture_handle, sampler_state);
                    encoder->setVertexBuffer(0, &tvb, 0, numVertices);
                    encoder->setIndexBuffer(&tib, offset, cmd->ElemCount);
                    encoder->submit(view_id, shaderHandle);
                }
            }

            offset += cmd->ElemCount;
        }
    }

    bgfx::end(encoder);
}

bool ImGui_Implbgfx_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    fontTexture = bgfx::createTexture2D(
        (uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8,
        0, bgfx::copy(pixels, width * height * 4));

    // Store our identifier
    io.Fonts->TexID = (void*)(intptr_t)fontTexture.idx;

    return true;
}

#include "bgfx/fs_ocornut_imgui.bin.h"
#include "bgfx/vs_ocornut_imgui.bin.h"

static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui), BGFX_EMBEDDED_SHADER_END()
};

bool ImGui_Implbgfx_CreateDeviceObjects()
{
    bgfx::RendererType::Enum type = bgfx::getRendererType();
    shaderHandle = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"),
        true);

    vertexLayout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    attribLocationTex =
        bgfx::createUniform("g_AttribLocationTex", bgfx::UniformType::Sampler);

    ImGui_Implbgfx_CreateFontsTexture();

    return true;
}

void ImGui_Implbgfx_InvalidateDeviceObjects()
{
    bgfx::destroy(attribLocationTex);
    bgfx::destroy(shaderHandle);

    if (isValid(fontTexture))
    {
        bgfx::destroy(fontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        fontTexture.idx = bgfx::kInvalidHandle;
    }
}

void ImGui_Implbgfx_Init()
{
    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = ImGui_Implbgfx_CreateWindow;
    platform_io.Renderer_DestroyWindow = ImGui_Implbgfx_DestroyWindow;
    platform_io.Renderer_SetWindowSize = ImGui_Implbgfx_SetWindowSize;
    platform_io.Renderer_RenderWindow = ImGui_Implbgfx_RenderWindow;
    platform_io.Renderer_SwapBuffers = nullptr;
}

void ImGui_Implbgfx_Shutdown()
{
    ImGui_Implbgfx_InvalidateDeviceObjects();
}

void ImGui_Implbgfx_NewFrame()
{
    if (!isValid(fontTexture))
    {
        ImGui_Implbgfx_CreateDeviceObjects();
    }
}

/*                */
/* Multi Viewport */
/*                */

struct imguiViewportData
{
    bgfx::FrameBufferHandle frameBufferHandle;
    bgfx::ViewId viewId = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

static void* glfwNativeWindowHandle(GLFWwindow* _window)
{
    #if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    return (void*)(uintptr_t)glfwGetX11Window(_window);
    #elif BX_PLATFORM_OSX
    return glfwGetCocoaWindow(_window);
    #elif BX_PLATFORM_WINDOWS
    return glfwGetWin32Window(_window);
    #endif // BX_PLATFORM_
}

static bgfx::ViewId allocate_view_id()
{
    if (!freeViewIds.empty())
    {
        const bgfx::ViewId id = freeViewIds.back();
        freeViewIds.pop_back();
        return id;
    }
    return subViewId++;
}

static void freeViewId(bgfx::ViewId id)
{
    freeViewIds.push_back(id);
}

void ImGui_Implbgfx_CreateWindow(ImGuiViewport* viewport)
{
    auto data = new imguiViewportData();
    viewport->RendererUserData = data;
    // Setup view id and size
    data->viewId = allocate_view_id();
    data->width = bx::max<uint16_t>((uint16_t)viewport->Size.x, 1);
    data->height = bx::max<uint16_t>((uint16_t)viewport->Size.y, 1);
    // Create frame buffer
    data->frameBufferHandle = bgfx::createFrameBuffer(glfwNativeWindowHandle((GLFWwindow *)viewport->PlatformHandle),
                              data->width * viewport->DrawData->FramebufferScale.x,
                              data->height * viewport->DrawData->FramebufferScale.y);
    // Set frame buffer
    bgfx::setViewFrameBuffer(data->viewId, data->frameBufferHandle);
}

void ImGui_Implbgfx_DestroyWindow(ImGuiViewport* viewport)
{
    if (auto data = (imguiViewportData *)viewport->RendererUserData; data)
    {
        viewport->RendererUserData = nullptr;
        freeViewId(data->viewId);
        bgfx::destroy(data->frameBufferHandle);
        data->frameBufferHandle.idx = bgfx::kInvalidHandle;
        delete data;
    }
}

void ImGui_Implbgfx_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    imguiViewportData* data = (imguiViewportData*)viewport->PlatformUserData;

    ImGui_Implbgfx_DestroyWindow(viewport);
    ImGui_Implbgfx_CreateWindow(viewport);
}

void ImGui_Implbgfx_RenderWindow(ImGuiViewport* viewport, void* renderArg)
{
    if (auto data = (imguiViewportData *)viewport->RendererUserData; data)
    {
        ImGui_Implbgfx_RenderDrawLists(viewport->DrawData, data->viewId);
    }
}

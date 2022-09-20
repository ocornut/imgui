#include "imgui_impl_metal_cpp.h"

// clang-format off
#if __has_include(<Foundation/Foundation.hpp>) && __has_include(<Metal/Metal.hpp>) && __has_include(<QuartzCore/QuartzCore.hpp>)
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#else
#error "Did you download metal-cpp-beta headers ? Cannot find the metal-cpp-beta headers !"
#endif
// clang-format on

#include "imgui.h"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#define NS_STRING_FROM_CSTRING(STR) NS::String::string(STR, NS::UTF8StringEncoding)

// ------------------------------------------------------------------------------------------------
// ALL STRUCTURES
// ------------------------------------------------------------------------------------------------

// A wrapper around a MTL::Buffer object that knows the last time it was reused
struct MetalBuffer
{
    explicit MetalBuffer(NS::SharedPtr<MTL::Buffer> buffer);
    MetalBuffer(MetalBuffer& other);
    MetalBuffer(MetalBuffer&& other);
    ~MetalBuffer() = default;
    bool operator<(const MetalBuffer& other) const;
    bool operator==(const MetalBuffer& other) const;

    NS::SharedPtr<MTL::Buffer> buffer;
    double                     lastReuseTime;
};

// An object that encapsulates the data necessary to uniquely identify a
// render pipeline state. These are used as cache keys.
struct FramebufferDescriptor
{
    explicit FramebufferDescriptor(MTL::RenderPassDescriptor* renderPassDescriptor);
    bool operator==(const FramebufferDescriptor& other) const;

    uint32_t         sampleCount;
    MTL::PixelFormat colorPixelFormat;
    MTL::PixelFormat depthPixelFormat;
    MTL::PixelFormat stencilPixelFormat;
};

namespace std {
template<>
struct hash<FramebufferDescriptor>
{
    size_t operator()(const FramebufferDescriptor& fbd) const;
};
}

// A singleton that stores long-lived objects that are needed by the Metal
// renderer backend. Stores the render pipeline state cache and the default
// font texture, and manages the reusable buffer cache.
struct MetalContext
{
    explicit MetalContext(MTL::Device* device);
    ~MetalContext();
    [[nodiscard]] std::shared_ptr<MetalBuffer>            dequeueReusableBuffer(NS::UInteger length);
    [[nodiscard]] NS::SharedPtr<MTL::RenderPipelineState> renderPipelineStateForFramebufferDescriptor(
      const FramebufferDescriptor* framebufferDescriptor) const;
    void synchronizedBufferCache(std::function<void(std::vector<std::shared_ptr<MetalBuffer>>&)>&& fn);

    MTL::Device*                                                                              device;
    NS::SharedPtr<MTL::DepthStencilState>                                                     depthStencilState;
    const FramebufferDescriptor*                                                              framebufferDescriptor;
    std::unordered_map<const FramebufferDescriptor*, NS::SharedPtr<MTL::RenderPipelineState>> renderPipelineStateCache;
    NS::SharedPtr<MTL::Texture>                                                               fontTexture;
    double                                                                                    lastBufferCachePurge;

  private:
    std::mutex                                _bufferCacheMutex;
    std::vector<std::shared_ptr<MetalBuffer>> _bufferCache;
};

struct ImGui_ImplMetal_Data
{
    ImGui_ImplMetal_Data();
    ~ImGui_ImplMetal_Data();

    MetalContext* SharedMetalContext;
};
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// API INTERNAL HELPER FUNCTIONS
// ------------------------------------------------------------------------------------------------
static inline double
GetMachAbsoluteTimeInSeconds()
{
    return static_cast<double>(static_cast<double>(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1e9);
}

static ImGui_ImplMetal_Data*
ImGui_ImplMetal_CreateBackendData()
{
    return IM_NEW(ImGui_ImplMetal_Data)();
}

static ImGui_ImplMetal_Data*
ImGui_ImplMetal_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplMetal_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static void
ImGui_ImplMetal_DestroyBackendData()
{
    IM_DELETE(ImGui_ImplMetal_GetBackendData());
}

bool
ImGui_ImplMetal_CreateFontsTexture(MTL::Device* device)
{
    ImGui_ImplMetal_Data* bd = ImGui_ImplMetal_GetBackendData();
    ImGuiIO&              io = ImGui::GetIO();

    // We are retrieving and uploading the font atlas as a 4-channels RGBA texture here.
    // In theory we could call GetTexDataAsAlpha8() and upload a 1-channel texture to save on memory access bandwidth.
    // However, using a shader designed for 1-channel texture would make it less obvious to use the ImTextureID facility
    // to render users own textures. You can make that change in your implementation.
    unsigned char* pixels;
    int            width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(
      MTL::PixelFormatRGBA8Unorm, (NS::UInteger)width, (NS::UInteger)height, false);
    textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
    textureDescriptor->setStorageMode(MTL::StorageModeManaged);
#else
    textureDescriptor.storageMode = MTL::StorageModeShared;
#endif
    auto texture = device->newTexture(textureDescriptor);
    if (!texture) { return false; }
    bd->SharedMetalContext->fontTexture = NS::TransferPtr(texture);
    bd->SharedMetalContext->fontTexture->replaceRegion(
      MTL::Region(0, 0, (NS::UInteger)width, (NS::UInteger)height), 0, pixels, (NS::UInteger)width * 4);
    io.Fonts->SetTexID((void*)bd->SharedMetalContext->fontTexture.get());
    return true;
}

void
ImGui_ImplMetal_DestroyFontsTexture()
{
    ImGui_ImplMetal_Data* bd = ImGui_ImplMetal_GetBackendData();
    ImGuiIO&              io = ImGui::GetIO();
    io.Fonts->SetTexID(nullptr);
}

bool
ImGui_ImplMetal_CreateDeviceObjects(MTL::Device* device)
{
    ImGui_ImplMetal_Data* bd                     = ImGui_ImplMetal_GetBackendData();
    auto                  depthStencilDescriptor = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
    depthStencilDescriptor->setDepthWriteEnabled(false);
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionAlways);
    bd->SharedMetalContext->depthStencilState =
      NS::TransferPtr(device->newDepthStencilState(depthStencilDescriptor.get()));
    ImGui_ImplMetal_CreateFontsTexture(device);
    return true;
}

void
ImGui_ImplMetal_DestroyDeviceObjects()
{
    ImGui_ImplMetal_Data* bd = ImGui_ImplMetal_GetBackendData();
    ImGui_ImplMetal_DestroyFontsTexture();
}

static void
ImGui_ImplMetal_SetupRenderState(ImDrawData*                             drawData,
                                 MTL::CommandBuffer*                     commandBuffer,
                                 MTL::RenderCommandEncoder*              commandEncoder,
                                 NS::SharedPtr<MTL::RenderPipelineState> renderPipelineState,
                                 const MetalBuffer*                      vertexBuffer,
                                 size_t                                  vertexBufferOffset)
{
    IM_UNUSED(commandBuffer);
    ImGui_ImplMetal_Data* bd = ImGui_ImplMetal_GetBackendData();
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setDepthStencilState(bd->SharedMetalContext->depthStencilState.get());

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport
    // apps.
    MTL::Viewport viewport = { .originX = 0.0,
                               .originY = 0.0,
                               .width   = (double)(drawData->DisplaySize.x * drawData->FramebufferScale.x),
                               .height  = (double)(drawData->DisplaySize.y * drawData->FramebufferScale.y),
                               .znear   = 0.0,
                               .zfar    = 1.0 };
    commandEncoder->setViewport(viewport);

    float       L                      = drawData->DisplayPos.x;
    float       R                      = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float       T                      = drawData->DisplayPos.y;
    float       B                      = drawData->DisplayPos.y + drawData->DisplaySize.y;
    float       N                      = (float)viewport.znear;
    float       F                      = (float)viewport.zfar;
    const float ortho_projection[4][4] = {
        { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
        { 0.0f, 0.0f, 1 / (F - N), 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), N / (F - N), 1.0f },
    };
    commandEncoder->setVertexBytes(&ortho_projection, sizeof(ortho_projection), 1);
    commandEncoder->setRenderPipelineState(renderPipelineState.get());
    commandEncoder->setVertexBuffer(vertexBuffer->buffer.get(), 0, 0);
    commandEncoder->setVertexBufferOffset(vertexBufferOffset, 0);
}
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// API
// ------------------------------------------------------------------------------------------------
IMGUI_IMPL_API bool
ImGui_ImplMetal_Init(MTL::Device* device)
{
    ImGui_ImplMetal_Data* bd   = ImGui_ImplMetal_CreateBackendData();
    ImGuiIO&              io   = ImGui::GetIO();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName     = "imgui_impl_metal_cpp";
    io.BackendFlags |=
      ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    bd->SharedMetalContext         = IM_NEW(MetalContext)(device);
    bd->SharedMetalContext->device = device;

    return true;
}

IMGUI_IMPL_API void
ImGui_ImplMetal_Shutdown()
{
    ImGui_ImplMetal_DestroyDeviceObjects();
    ImGui_ImplMetal_DestroyBackendData();
}

IMGUI_IMPL_API void
ImGui_ImplMetal_NewFrame(MTL::RenderPassDescriptor* renderPassDescriptor)
{
    ImGui_ImplMetal_Data* bd = ImGui_ImplMetal_GetBackendData();
    IM_ASSERT(bd->SharedMetalContext != nil && "No Metal context. Did you call ImGui_ImplMetal_Init() ?");

    static std::unordered_map<MTL::RenderPassDescriptor*, std::unique_ptr<FramebufferDescriptor>> fbdm;
    if (fbdm.find(renderPassDescriptor) == fbdm.end()) {
        fbdm[renderPassDescriptor] = std::make_unique<FramebufferDescriptor>(renderPassDescriptor);
        if (!bd->SharedMetalContext->depthStencilState) {
            ImGui_ImplMetal_CreateDeviceObjects(bd->SharedMetalContext->device);
        }
    }
    bd->SharedMetalContext->framebufferDescriptor = fbdm[renderPassDescriptor].get();
}

IMGUI_IMPL_API void
ImGui_ImplMetal_RenderDrawData(ImDrawData*                drawData,
                               MTL::CommandBuffer*        commandBuffer,
                               MTL::RenderCommandEncoder* commandEncoder)
{
    ImGui_ImplMetal_Data* bd  = ImGui_ImplMetal_GetBackendData();
    MetalContext*         ctx = bd->SharedMetalContext;

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer
    // coordinates)
    int fb_width  = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || drawData->CmdListsCount == 0) return;

    // Try to retrieve a render pipeline state that is compatible with the framebuffer config for this frame
    // The hit rate for this cache should be very near 100%.
    NS::SharedPtr<MTL::RenderPipelineState> renderPipelineState;
    auto renderPipelineStateIt = ctx->renderPipelineStateCache.find(ctx->framebufferDescriptor);
    if (renderPipelineStateIt != ctx->renderPipelineStateCache.end()) {
        renderPipelineState = renderPipelineStateIt->second;
    } else {
        // No luck; make a new render pipeline state
        renderPipelineState = ctx->renderPipelineStateForFramebufferDescriptor(ctx->framebufferDescriptor);
        auto [insertedIt, succeeded] =
          ctx->renderPipelineStateCache.insert({ ctx->framebufferDescriptor, renderPipelineState });
        IM_ASSERT(succeeded && "Could not insert a new render pipeline state in the cache !");
    }

    IM_ASSERT(renderPipelineState && "Must have a valid render pipeline state created !");

    size_t vertexBufferLength = (size_t)drawData->TotalVtxCount * sizeof(ImDrawVert);
    size_t indexBufferLength  = (size_t)drawData->TotalIdxCount * sizeof(ImDrawIdx);
    auto   vertexBuffer       = ctx->dequeueReusableBuffer(vertexBufferLength);
    auto   indexBuffer        = ctx->dequeueReusableBuffer(indexBufferLength);

    ImGui_ImplMetal_SetupRenderState(
      drawData, commandBuffer, commandEncoder, renderPipelineState, vertexBuffer.get(), 0);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off   = drawData->DisplayPos;       // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    size_t vertexBufferOffset = 0;
    size_t indexBufferOffset  = 0;
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmd_list = drawData->CmdLists[n];

        memcpy((char*)vertexBuffer->buffer->contents() + vertexBufferOffset,
               cmd_list->VtxBuffer.Data,
               (size_t)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy((char*)indexBuffer->buffer->contents() + indexBufferOffset,
               cmd_list->IdxBuffer.Data,
               (size_t)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pCmd = &cmd_list->CmdBuffer[cmd_i];
            if (pCmd->UserCallback) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer
                // to reset render state.)
                if (pCmd->UserCallback == ImDrawCallback_ResetRenderState) {
                    ImGui_ImplMetal_SetupRenderState(drawData,
                                                     commandBuffer,
                                                     commandEncoder,
                                                     renderPipelineState,
                                                     vertexBuffer.get(),
                                                     vertexBufferOffset);
                } else {
                    pCmd->UserCallback(cmd_list, pCmd);
                }
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pCmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pCmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pCmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pCmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as setScissorRect() won't accept values that are off bounds
                if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
                if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
                if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
                if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) continue;
                if (pCmd->ElemCount == 0) {
                    // drawIndexedPrimitives() validation doesn't accept this
                    continue;
                }

                // Apply scissor/clipping rectangle
                MTL::ScissorRect scissorRect = { .x      = NS::UInteger(clip_min.x),
                                                 .y      = NS::UInteger(clip_min.y),
                                                 .width  = NS::UInteger(clip_max.x - clip_min.x),
                                                 .height = NS::UInteger(clip_max.y - clip_min.y) };
                commandEncoder->setScissorRect(scissorRect);

                // Bind texture, Draw
                if (ImTextureID tex_id = pCmd->GetTexID()) {
                    commandEncoder->setFragmentTexture((MTL::Texture*)(tex_id), 0);
                }

                commandEncoder->setVertexBufferOffset((vertexBufferOffset + pCmd->VtxOffset * sizeof(ImDrawVert)), 0);
                commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
                                                      pCmd->ElemCount,
                                                      sizeof(ImDrawIdx) == 2 ? MTL::IndexTypeUInt16
                                                                             : MTL::IndexTypeUInt32,
                                                      indexBuffer->buffer.get(),
                                                      indexBufferOffset + pCmd->IdxOffset * sizeof(ImDrawIdx));
            }
        }

        vertexBufferOffset += (size_t)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        indexBufferOffset += (size_t)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }

    commandBuffer->addCompletedHandler(^(MTL::CommandBuffer*) {
      if (bd) {
          bd->SharedMetalContext->synchronizedBufferCache([&](std::vector<std::shared_ptr<MetalBuffer>>& bufferCache) {
              bufferCache.emplace_back(vertexBuffer);
              bufferCache.emplace_back(indexBuffer);
          });
      }
    });
}
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// STRUCTURES IMPLEMENTATION
// ------------------------------------------------------------------------------------------------
ImGui_ImplMetal_Data::ImGui_ImplMetal_Data()
  : SharedMetalContext(nullptr)
{
}

ImGui_ImplMetal_Data::~ImGui_ImplMetal_Data()
{
    if (SharedMetalContext) { IM_DELETE(SharedMetalContext); }
}

MetalBuffer::MetalBuffer(NS::SharedPtr<MTL::Buffer> buffer)
  : buffer(buffer)
  , lastReuseTime(GetMachAbsoluteTimeInSeconds())
{
}

MetalBuffer::MetalBuffer(MetalBuffer& other)
{
    other.buffer        = buffer;
    other.lastReuseTime = lastReuseTime;
}

MetalBuffer::MetalBuffer(MetalBuffer&& other)
{
    other.buffer        = std::move(buffer);
    other.lastReuseTime = std::move(lastReuseTime);
}

bool
MetalBuffer::operator<(const MetalBuffer& other) const
{
    return lastReuseTime < other.lastReuseTime;
}

bool
MetalBuffer::operator==(const MetalBuffer& other) const
{
    return lastReuseTime == other.lastReuseTime;
}

FramebufferDescriptor::FramebufferDescriptor(MTL::RenderPassDescriptor* renderPassDescriptor)
  : sampleCount(renderPassDescriptor->colorAttachments()->object(0)->texture()->sampleCount())
  , colorPixelFormat(renderPassDescriptor->colorAttachments()->object(0)->texture()->pixelFormat())
  , depthPixelFormat(renderPassDescriptor->depthAttachment()->texture()->pixelFormat())
  , stencilPixelFormat(renderPassDescriptor->stencilAttachment()->texture()->pixelFormat())
{
}

bool
FramebufferDescriptor::operator==(const FramebufferDescriptor& other) const
{
    return (sampleCount == other.sampleCount && colorPixelFormat == other.colorPixelFormat &&
            depthPixelFormat == other.depthPixelFormat && stencilPixelFormat == other.stencilPixelFormat);
}

std::size_t
std::hash<FramebufferDescriptor>::operator()(const FramebufferDescriptor& fbd) const
{
    std::size_t sc   = fbd.sampleCount & 0x3;
    std::size_t cf   = fbd.colorPixelFormat & 0x3FF;
    std::size_t df   = fbd.depthPixelFormat & 0x3FF;
    std::size_t sf   = fbd.stencilPixelFormat & 0x3FF;
    std::size_t hash = (sf << 22) | (df << 12) | (cf << 2) | sc;
    return hash;
}

MetalContext::MetalContext(MTL::Device* device)
  : device(device)
  , depthStencilState()
  , framebufferDescriptor(nullptr)
  , renderPipelineStateCache()
  , fontTexture()
  , _bufferCache()
  , lastBufferCachePurge(GetMachAbsoluteTimeInSeconds())
{
}

MetalContext::~MetalContext()
{
    synchronizedBufferCache([&](std::vector<std::shared_ptr<MetalBuffer>>& bufferCache) { bufferCache.clear(); });
    renderPipelineStateCache.clear();
    device->release();
}

std::shared_ptr<MetalBuffer>
MetalContext::dequeueReusableBuffer(NS::UInteger length)
{
    double now = GetMachAbsoluteTimeInSeconds();

    std::shared_ptr<MetalBuffer> bestCandidate;

    synchronizedBufferCache([&](std::vector<std::shared_ptr<MetalBuffer>>& bufferCache) {
        // Purge old buffers that haven't been useful for a while
        if (now - lastBufferCachePurge > 1.0) {
            for (auto it = bufferCache.begin(); it != bufferCache.end();) {
                if ((*it)->lastReuseTime <= lastBufferCachePurge) {
                    it = bufferCache.erase(it);
                } else {
                    ++it;
                }
            }
            lastBufferCachePurge = now;
        }

        // see if we have a buffer we can reuse
        auto bestCandidateIt = bufferCache.end();
        for (auto it = bufferCache.begin(); it != bufferCache.end(); ++it) {
            if ((*it)->buffer->length() >= length &&
                (bestCandidateIt == bufferCache.end() || (*bestCandidateIt)->lastReuseTime > (*it)->lastReuseTime)) {
                bestCandidateIt = it;
            }
        }

        if (bestCandidateIt != bufferCache.end()) {
            (*bestCandidateIt)->lastReuseTime = now;
            bestCandidate                     = std::move(*bestCandidateIt);
            bufferCache.erase(bestCandidateIt);
            return;
        }

        // no luck, make a new buffer
        // the backing buffer is released when MetalBuffer is destructed, see destructor
        auto backing  = NS::TransferPtr(device->newBuffer(length, MTL::ResourceStorageModeShared));
        bestCandidate = std::make_unique<MetalBuffer>(backing);
    });
    return bestCandidate;
}

NS::SharedPtr<MTL::RenderPipelineState>
MetalContext::renderPipelineStateForFramebufferDescriptor(const FramebufferDescriptor* framebufferDescriptor) const
{
    // clang-format off
        NS::String* shaderSource = NS_STRING_FROM_CSTRING(
          ""
          "#include <metal_stdlib>\n"
          "using namespace metal;\n"
          "\n"
          "struct Uniforms {\n"
          "    float4x4 projectionMatrix;\n"
          "};\n"
          "\n"
          "struct VertexIn {\n"
          "    float2 position  [[attribute(0)]];\n"
          "    float2 texCoords [[attribute(1)]];\n"
          "    uchar4 color     [[attribute(2)]];\n"
          "};\n"
          "\n"
          "struct VertexOut {\n"
          "    float4 position [[position]];\n"
          "    float2 texCoords;\n"
          "    float4 color;\n"
          "};\n"
          "\n"
          "vertex VertexOut vertex_main(VertexIn in                 [[stage_in]],\n"
          "                             constant Uniforms &uniforms [[buffer(1)]]) {\n"
          "    VertexOut out;\n"
          "    out.position = uniforms.projectionMatrix * float4(in.position, 0, 1);\n"
          "    out.texCoords = in.texCoords;\n"
          "    out.color = float4(in.color) / float4(255.0);\n"
          "    return out;\n"
          "}\n"
          "\n"
          "fragment half4 fragment_main(VertexOut in [[stage_in]],\n"
          "                             texture2d<half, access::sample> texture [[texture(0)]]) {\n"
          "    constexpr sampler linearSampler(coord::normalized, min_filter::linear, mag_filter::linear, mip_filter::linear);\n"
          "    half4 texColor = texture.sample(linearSampler, in.texCoords);\n"
          "    return half4(in.color) * texColor;\n"
          "}\n");
    // clang-format on

    NS::Error*                 error   = nullptr;
    const MTL::CompileOptions* options = nullptr;
    auto                       library = NS::TransferPtr(device->newLibrary(shaderSource, options, &error));
    IM_ASSERT(library && "Failed to create metal library");

    auto vertexFunction = NS::TransferPtr(library->newFunction(NS_STRING_FROM_CSTRING("vertex_main")));
    IM_ASSERT(vertexFunction && "Failed to find metal vertex shader function");

    auto fragmentFunction = NS::TransferPtr(library->newFunction(NS_STRING_FROM_CSTRING("fragment_main")));
    IM_ASSERT(fragmentFunction && "Failed to find metal fragment shader function");

    auto vertexDescriptor = MTL::VertexDescriptor::vertexDescriptor();
    // position
    vertexDescriptor->attributes()->object(0)->setOffset(IM_OFFSETOF(ImDrawVert, pos));
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat2);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(0);
    // texture coordinates
    vertexDescriptor->attributes()->object(1)->setOffset(IM_OFFSETOF(ImDrawVert, uv));
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
    vertexDescriptor->attributes()->object(1)->setBufferIndex(0);
    // color
    vertexDescriptor->attributes()->object(2)->setOffset(IM_OFFSETOF(ImDrawVert, col));
    vertexDescriptor->attributes()->object(2)->setFormat(MTL::VertexFormatUChar4);
    vertexDescriptor->attributes()->object(2)->setBufferIndex(0);

    vertexDescriptor->layouts()->object(0)->setStepRate(1);
    vertexDescriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(ImDrawVert));

    auto pipelineDescriptor = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
    pipelineDescriptor->setVertexFunction(vertexFunction.get());
    pipelineDescriptor->setFragmentFunction(fragmentFunction.get());
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    pipelineDescriptor->setRasterSampleCount(framebufferDescriptor->sampleCount);
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(framebufferDescriptor->colorPixelFormat);
    pipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
    pipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
      MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(
      MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->setDepthAttachmentPixelFormat(framebufferDescriptor->depthPixelFormat);
    pipelineDescriptor->setStencilAttachmentPixelFormat(framebufferDescriptor->stencilPixelFormat);

    auto pipelineState = NS::TransferPtr(device->newRenderPipelineState(pipelineDescriptor.get(), &error));
    IM_ASSERT(pipelineState && "Failed to create metal pipeline state");

    return { pipelineState };
}

void
MetalContext::synchronizedBufferCache(std::function<void(std::vector<std::shared_ptr<MetalBuffer>>&)>&& fn)
{
    std::lock_guard<std::mutex> lock(_bufferCacheMutex);
    fn(_bufferCache);
}

// ------------------------------------------------------------------------------------------------

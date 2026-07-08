// dear imgui: Renderer Backend for Metal 4
// This needs to be used along with a Platform Backend (e.g. OSX)
// Metal 4 requires Apple Silicon and macOS 26+.

// Implemented features:
//  [X] Renderer: User texture binding. Use 'MTLTexture' as texture identifier. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Texture updates support for dynamic font atlas (ImGuiBackendFlags_RendererHasTextures).
//  [X] Renderer: Multi-viewport support (multiple windows).

// Missing features or Issues:
//  [ ] Texture view pool support? Reevaluate which type to use for ImtextureID.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-07-07: Metal 4: Added metal-cpp support. (#9461)
//  2026-07-02: Metal 4: Added new Metal 4 backend implementation. (#9458)

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_metal4.h"
#import <time.h>
#import <Metal/Metal.h>

#ifdef IMGUI_IMPL_METAL_CPP
#include <Metal/Metal.hpp>
#endif

// Forward Declarations
static void ImGui_ImplMetal_InitMultiViewportSupport();
static void ImGui_ImplMetal_ShutdownMultiViewportSupport();
static void ImGui_ImplMetal_CreateDeviceObjectsForPlatformWindows();
static void ImGui_ImplMetal_InvalidateDeviceObjectsForPlatformWindows();

#pragma mark - Support classes and structs

#define METAL_IMGUI_VIEWPORTS_PER_CHUNK 64

struct ImGui_Metal4_ConstantData
{
    // 1 projection matrix is encoded as 4x4 float.
    // Store up to 64 of them in same constant buffer, so multiple viewports can use same constant buffer.
    float ModelViewProjectionMatrix[METAL_IMGUI_VIEWPORTS_PER_CHUNK][4][4];
};

@interface MetalBuffer : NSObject
@property (nonatomic, strong) id<MTLBuffer> buffer;
@property (nonatomic, assign) double        lastReuseTime;
- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer;
@end

// An object that encapsulates the data necessary to uniquely identify a
// render pipeline state. These are used as cache keys.
@interface FramebufferDescriptor : NSObject<NSCopying>
@property (nonatomic, assign) unsigned long  sampleCount;
@property (nonatomic, assign) MTLPixelFormat colorPixelFormat;
@property (nonatomic, assign) MTLPixelFormat depthPixelFormat;
@property (nonatomic, assign) MTLPixelFormat stencilPixelFormat;
- (instancetype)initWithRenderPassDescriptor:(MTL4RenderPassDescriptor*)renderPassDescriptor;
@end

@interface MetalTexture : NSObject
@property (nonatomic, strong) id<MTLTexture> metalTexture;
- (instancetype)initWithTexture:(id<MTLTexture>)metalTexture;
@end

// A singleton that stores long-lived objects that are needed by the Metal
// renderer backend. Stores the render pipeline state cache and the default
// font texture, and manages the reusable buffer cache.
@interface MetalContext : NSObject
@property (nonatomic, strong) id<MTLDevice>                 device;
@property (nonatomic, strong) id<MTL4CommandQueue>          commandQueue;
@property (nonatomic, strong) id<MTLDepthStencilState>      depthStencilState;
@property (nonatomic, strong) id<MTL4ArgumentTable>         argumentTable;
@property (nonatomic, strong) id<MTLSamplerState>           samplerStateLinear;
@property (nonatomic, strong) id<MTLSamplerState>           samplerStateNearest;
@property (nonatomic, strong) id<MTLResidencySet>           residencySet;
@property (nonatomic, strong) FramebufferDescriptor*        framebufferDescriptor;
@property (nonatomic, strong) NSMutableDictionary*          renderPipelineStateCache;
@property (nonatomic, assign) NSUInteger                    framesInFlight;
@property (nonatomic, assign) NSUInteger                    currentFrameSlot;
@property (nonatomic, strong) NSMutableArray<NSMutableArray<MetalBuffer*>*>* bufferCaches;
@property (nonatomic, strong) NSObject*                     bufferCacheLock;
@property (nonatomic, assign) double                        lastBufferCachePurge;
@property (nonatomic, strong) NSArray<id<MTLSharedEvent>>*  events; // for tracking when a commands are complete to reset allocator
@property (nonatomic) uint64_t                              eventValue;
@property (nonatomic, strong) NSArray<id<MTL4CommandAllocator>>* commandAllocators;
@property (nonatomic, strong) NSMutableArray<NSMutableArray<id<MTLBuffer>>*>*  constantBuffers;
@property (nonatomic) uint64_t                              constantBufferChunkCount;
@property (nonatomic) uint64_t                              currentConstantBufferIndex;
- (MetalBuffer*)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device;
- (id<MTLRenderPipelineState>)renderPipelineStateForFramebufferDescriptor:(FramebufferDescriptor*)descriptor device:(id<MTLDevice>)device;
@end

struct ImGui_ImplMetal4_Data
{
    MetalContext*                SharedMetalContext;
    id<MTL4RenderCommandEncoder> RenderCommandEncoder;

    ImGui_ImplMetal4_Data()       { memset((void*)this, 0, sizeof(*this)); }
};

static ImGui_ImplMetal4_Data*    ImGui_ImplMetal4_GetBackendData()    { return ImGui::GetCurrentContext() ? (ImGui_ImplMetal4_Data*)ImGui::GetIO().BackendRendererUserData : nullptr; }
static void                      ImGui_ImplMetal4_DestroyBackendData(){ IM_DELETE(ImGui_ImplMetal4_GetBackendData()); }

static inline CFTimeInterval    GetMachAbsoluteTimeInSeconds()      { return (CFTimeInterval)(double)(clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1e9); }

#ifdef IMGUI_IMPL_METAL_CPP

#pragma mark - Dear ImGui Metal C++ Backend API

bool ImGui_ImplMetal4_Init(MTL::Device* device, MTL4::CommandQueue* commandQueue, int framesInFlight)
{
    return ImGui_ImplMetal4_Init((__bridge id<MTLDevice>)(device),(__bridge id<MTL4CommandQueue>)(commandQueue), framesInFlight);
}

void ImGui_ImplMetal4_NewFrame(MTL4::RenderPassDescriptor* renderPassDescriptor, int frameInFlightIndex)
{
    ImGui_ImplMetal4_NewFrame((__bridge MTL4RenderPassDescriptor*)(renderPassDescriptor), frameInFlightIndex);
}

void ImGui_ImplMetal4_RenderDrawData(ImDrawData* draw_data,
                                    MTL4::CommandBuffer* commandBuffer,
                                    MTL4::RenderCommandEncoder* commandEncoder)
{
    ImGui_ImplMetal4_RenderDrawData(draw_data,
                                   (__bridge id<MTL4CommandBuffer>)(commandBuffer),
                                   (__bridge id<MTL4RenderCommandEncoder>)(commandEncoder));
}

bool ImGui_ImplMetal_CreateDeviceObjects(MTL::Device* device)
{
    return ImGui_ImplMetal4_CreateDeviceObjects((__bridge id<MTLDevice>)(device));
}

#endif // #ifdef IMGUI_IMPL_METAL_CPP

#pragma mark - Dear ImGui Metal Backend API

void ImGui_ImplMetal4_NewFrame(MTL4RenderPassDescriptor* renderPassDescriptor, int frameInFlightIndex)
{
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();
    IM_ASSERT(bd != nil && "Context or backend not initialized! Did you call ImGui_ImplMetal4_Init()?");
    IM_ASSERT(frameInFlightIndex < bd->SharedMetalContext.framesInFlight && "frameInFlightIndex out of range! See framesInFlight passed to ImGui_ImplMetal4_Init().");
#ifdef IMGUI_IMPL_METAL_CPP
    bd->SharedMetalContext.framebufferDescriptor = [[[FramebufferDescriptor alloc] initWithRenderPassDescriptor:renderPassDescriptor]autorelease];
#else
    bd->SharedMetalContext.framebufferDescriptor = [[FramebufferDescriptor alloc] initWithRenderPassDescriptor:renderPassDescriptor];
#endif
    bd->SharedMetalContext.currentFrameSlot = (NSUInteger)frameInFlightIndex;
    if (bd->SharedMetalContext.depthStencilState == nil)
        ImGui_ImplMetal4_CreateDeviceObjects(bd->SharedMetalContext.device);
    
    bd->SharedMetalContext.currentConstantBufferIndex = 0;
    [bd->SharedMetalContext.events[frameInFlightIndex] waitUntilSignaledValue:bd->SharedMetalContext.eventValue timeoutMS:UINT64_MAX];
    [bd->SharedMetalContext.commandAllocators[frameInFlightIndex] reset];
}

static void ImGui_ImplMetal4_SetupRenderState(ImDrawData* draw_data, id<MTL4CommandBuffer> commandBuffer,
    id<MTL4RenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> renderPipelineState,
    MetalBuffer* vertexBuffer, size_t vertexBufferOffset)
{
    IM_UNUSED(commandBuffer);
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();
    [commandEncoder setCullMode:MTLCullModeNone];
    [commandEncoder setDepthStencilState:bd->SharedMetalContext.depthStencilState];

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
    MTLViewport viewport =
    {
        .originX = 0.0,
        .originY = 0.0,
        .width = (double)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x),
        .height = (double)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y),
        .znear = 0.0,
        .zfar = 1.0
    };
    [commandEncoder setViewport:viewport];

    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float N = (float)viewport.znear;
    float F = (float)viewport.zfar;
    const float ortho_projection[4][4] =
    {
        { 2.0f/(R-L),   0.0f,           0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),     0.0f,   0.0f },
        { 0.0f,         0.0f,        1/(F-N),   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T), N/(F-N),   1.0f },
    };
    
    int currentChunk = (int)((float)bd->SharedMetalContext.currentConstantBufferIndex / (float)METAL_IMGUI_VIEWPORTS_PER_CHUNK);
    int currentIndex = bd->SharedMetalContext.currentConstantBufferIndex % METAL_IMGUI_VIEWPORTS_PER_CHUNK;
    
    int currentFrameIndex = (int)bd->SharedMetalContext.currentFrameSlot;
    id<MTLBuffer> constantBuffer = bd->SharedMetalContext.constantBuffers[currentFrameIndex][currentChunk];
    ImGui_Metal4_ConstantData* constantBufferContents = (ImGui_Metal4_ConstantData*)constantBuffer.contents;
    
    memcpy(&constantBufferContents->ModelViewProjectionMatrix[currentIndex], ortho_projection, sizeof(ortho_projection));
    
    id<MTL4ArgumentTable> argumentTable = bd->SharedMetalContext.argumentTable;
    [argumentTable setAddress:constantBuffer.gpuAddress+(uint64_t)currentIndex * sizeof(constantBufferContents->ModelViewProjectionMatrix[0]) atIndex:1];
    [argumentTable setAddress:(vertexBuffer.buffer.gpuAddress + vertexBufferOffset) attributeStride:sizeof(ImDrawVert) atIndex:0];
    [argumentTable setSamplerState:bd->SharedMetalContext.samplerStateLinear.gpuResourceID atIndex:0];
    [commandEncoder setArgumentTable:argumentTable atStages:MTLRenderStageVertex | MTLRenderStageFragment];
    [commandEncoder setRenderPipelineState:renderPipelineState];
}

static void ImGui_ImplMetal4_DrawCallback_ResetRenderState(const ImDrawList*, const ImDrawCmd*)  {} // Intentionally empty. Used as an identifier for rendering loop to call its code. Simpler to implement this way.
static void ImGui_ImplMetal4_DrawCallback_SetSamplerLinear(const ImDrawList*, const ImDrawCmd*)  { ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData(); [bd->SharedMetalContext.argumentTable setSamplerState:bd->SharedMetalContext.samplerStateLinear.gpuResourceID atIndex:0]; }
static void ImGui_ImplMetal4_DrawCallback_SetSamplerNearest(const ImDrawList*, const ImDrawCmd*) { ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData(); [bd->SharedMetalContext.argumentTable setSamplerState:bd->SharedMetalContext.samplerStateNearest.gpuResourceID atIndex:0]; }

void ImGui_ImplMetal4_RenderDrawData(ImDrawData* draw_data, id<MTL4CommandBuffer> commandBuffer, id<MTL4RenderCommandEncoder> commandEncoder)
{
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();
    MetalContext* ctx = bd->SharedMetalContext;

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->CmdLists.Size == 0)
        return;

    // Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplMetal4_UpdateTexture(tex);

    // Try to retrieve a render pipeline state that is compatible with the framebuffer config for this frame
    // The hit rate for this cache should be very near 100%.
    id<MTLRenderPipelineState> renderPipelineState = ctx.renderPipelineStateCache[ctx.framebufferDescriptor];
    if (renderPipelineState == nil)
    {
        // No luck; make a new render pipeline state
        renderPipelineState = [ctx renderPipelineStateForFramebufferDescriptor:ctx.framebufferDescriptor device:commandBuffer.device];

        // Cache render pipeline state for later reuse
        ctx.renderPipelineStateCache[ctx.framebufferDescriptor] = renderPipelineState;
    }

    size_t vertexBufferLength = (size_t)draw_data->TotalVtxCount * sizeof(ImDrawVert);
    size_t indexBufferLength = (size_t)draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    MetalBuffer* vertexBuffer = [ctx dequeueReusableBufferOfLength:vertexBufferLength device:commandBuffer.device];
    MetalBuffer* indexBuffer = [ctx dequeueReusableBufferOfLength:indexBufferLength device:commandBuffer.device];

    bd->RenderCommandEncoder = commandEncoder;
    
    // check if more chunks are required
    const int chunksRequired = 1 + (int)((float)bd->SharedMetalContext.currentConstantBufferIndex / (float)METAL_IMGUI_VIEWPORTS_PER_CHUNK);
    if (chunksRequired > bd->SharedMetalContext.constantBufferChunkCount)
    {
        for (NSUInteger i = 0; i < bd->SharedMetalContext.framesInFlight; i++)
        {
            id<MTLBuffer> buffer = [bd->SharedMetalContext.device newBufferWithLength:sizeof(ImGui_Metal4_ConstantData) options:MTLResourceStorageModeShared];
            [bd->SharedMetalContext.constantBuffers[i] addObject:buffer];
            [bd->SharedMetalContext.residencySet addAllocation:buffer];
        }
        bd->SharedMetalContext.constantBufferChunkCount++;
    }
    
    ImGui_ImplMetal4_SetupRenderState(draw_data, commandBuffer, commandEncoder, renderPipelineState, vertexBuffer, 0);
    bd->SharedMetalContext.currentConstantBufferIndex++;
    
    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    size_t vertexBufferOffset = 0;
    size_t indexBufferOffset = 0;
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        memcpy((char*)vertexBuffer.buffer.contents + vertexBufferOffset, draw_list->VtxBuffer.Data, (size_t)draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy((char*)indexBuffer.buffer.contents + indexBufferOffset, draw_list->IdxBuffer.Data, (size_t)draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));

        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                if (pcmd->UserCallback == ImGui_ImplMetal4_DrawCallback_ResetRenderState)
                    ImGui_ImplMetal4_SetupRenderState(draw_data, commandBuffer, commandEncoder, renderPipelineState, vertexBuffer, vertexBufferOffset);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as setScissorRect() won't accept values that are off bounds
                if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
                if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
                if (clip_max.x > (float)fb_width) { clip_max.x = (float)fb_width; }
                if (clip_max.y > (float)fb_height) { clip_max.y = (float)fb_height; }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;
                if (pcmd->ElemCount == 0) // drawIndexedPrimitives() validation doesn't accept this
                    continue;

                // Apply scissor/clipping rectangle
                MTLScissorRect scissorRect =
                {
                    .x = NSUInteger(clip_min.x),
                    .y = NSUInteger(clip_min.y),
                    .width = NSUInteger(clip_max.x - clip_min.x),
                    .height = NSUInteger(clip_max.y - clip_min.y)
                };
                [commandEncoder setScissorRect:scissorRect];

                // Bind texture, Draw
                ImTextureID tex_id = pcmd->GetTexID();
                if (tex_id != ImTextureID_Invalid)
                {
                    id<MTLTexture> texture = (__bridge id<MTLTexture>)(void*)(intptr_t)tex_id;
                    [bd->SharedMetalContext.residencySet addAllocation:texture];
                    [bd->SharedMetalContext.argumentTable setTexture:texture.gpuResourceID atIndex:0];
                }

                [bd->SharedMetalContext.argumentTable setAddress:(vertexBuffer.buffer.gpuAddress + vertexBufferOffset + (pcmd->VtxOffset * sizeof(ImDrawVert))) attributeStride:sizeof(ImDrawVert) atIndex:0];

                size_t indexBufferCmdOffset = indexBufferOffset + (pcmd->IdxOffset * sizeof(ImDrawIdx));
                [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                   indexCount:pcmd->ElemCount
                                    indexType:sizeof(ImDrawIdx) == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32
                                  indexBuffer:indexBuffer.buffer.gpuAddress + indexBufferCmdOffset
                            indexBufferLength:indexBuffer.buffer.length - indexBufferCmdOffset];
            }
        }

        vertexBufferOffset += (size_t)draw_list->VtxBuffer.Size * sizeof(ImDrawVert);
        indexBufferOffset += (size_t)draw_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }

    MetalContext* sharedMetalContext = bd->SharedMetalContext;
    @synchronized(sharedMetalContext.bufferCacheLock)
    {
        NSMutableArray<MetalBuffer*>* slotCache = sharedMetalContext.bufferCaches[sharedMetalContext.currentFrameSlot];
        [slotCache addObject:vertexBuffer];
        [slotCache addObject:indexBuffer];
    }
    
    // Commit residency set
    [bd->SharedMetalContext.residencySet commit];
    bd->RenderCommandEncoder = nil;
}

static void ImGui_ImplMetal4_DestroyTexture(ImTextureData* tex)
{
    if (MetalTexture* backend_tex = (__bridge_transfer MetalTexture*)(tex->BackendUserData))
    {
        IM_ASSERT(backend_tex.metalTexture == (__bridge id<MTLTexture>)(void*)(intptr_t)tex->TexID);
        backend_tex.metalTexture = nil;

        // Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
        tex->SetTexID(ImTextureID_Invalid);
        tex->BackendUserData = nullptr;
    }
    tex->SetStatus(ImTextureStatus_Destroyed);
}

void ImGui_ImplMetal4_UpdateTexture(ImTextureData* tex)
{
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create and upload new texture to graphics system
        //IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

        // We are retrieving and uploading the font atlas as a 4-channels RGBA texture here.
        // In theory we could call GetTexDataAsAlpha8() and upload a 1-channel texture to save on memory access bandwidth.
        // However, using a shader designed for 1-channel texture would make it less obvious to use the ImTextureID facility to render users own textures.
        // You can make that change in your implementation.
        MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                     width:(NSUInteger)tex->Width
                                                                                                    height:(NSUInteger)tex->Height
                                                                                                 mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        textureDescriptor.storageMode = MTLStorageModeShared;

        id <MTLTexture> texture = [bd->SharedMetalContext.device newTextureWithDescriptor:textureDescriptor];
        [bd->SharedMetalContext.residencySet addAllocation:texture];
        [texture replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)tex->Width, (NSUInteger)tex->Height) mipmapLevel:0 withBytes:tex->Pixels bytesPerRow:(NSUInteger)tex->Width * 4];
        MetalTexture* backend_tex = [[MetalTexture alloc] initWithTexture:texture];

        // Store identifiers
        tex->SetTexID((ImTextureID)(intptr_t)texture);
        tex->SetStatus(ImTextureStatus_OK);
        tex->BackendUserData = (__bridge_retained void*)(backend_tex);
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        // Update selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->Updates[] but you can use tex->UpdateRect to upload a single region.
        MetalTexture* backend_tex = (__bridge MetalTexture*)(tex->BackendUserData);
        for (ImTextureRect& r : tex->Updates)
        {
            [backend_tex.metalTexture replaceRegion:MTLRegionMake2D((NSUInteger)r.x, (NSUInteger)r.y, (NSUInteger)r.w, (NSUInteger)r.h)
                                        mipmapLevel:0
                                          withBytes:tex->GetPixelsAt(r.x, r.y)
                                        bytesPerRow:(NSUInteger)tex->Width * 4];
        }
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0)
    {
        ImGui_ImplMetal4_DestroyTexture(tex);
    }
}

bool ImGui_ImplMetal4_CreateDeviceObjects(id<MTLDevice> device)
{
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();

    MTLResidencySetDescriptor* residencySetDescriptor = [[MTLResidencySetDescriptor alloc] init];
    residencySetDescriptor.initialCapacity = 1000;

    NSError* error = nil;
    bd->SharedMetalContext.residencySet = [device newResidencySetWithDescriptor:residencySetDescriptor error:&error];
    IM_ASSERT(bd->SharedMetalContext.residencySet != nil && error == nil);

    [bd->SharedMetalContext.commandQueue addResidencySet:bd->SharedMetalContext.residencySet];

    MTLDepthStencilDescriptor* depthStencilDescriptor = [[MTLDepthStencilDescriptor alloc] init];
    depthStencilDescriptor.depthWriteEnabled = NO;
    depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
    bd->SharedMetalContext.depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
    MTLSamplerDescriptor* samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    samplerDescriptor.supportArgumentBuffers = YES;
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
    bd->SharedMetalContext.samplerStateLinear = [device newSamplerStateWithDescriptor:samplerDescriptor];
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
    samplerDescriptor.mipFilter = MTLSamplerMipFilterNearest;
    bd->SharedMetalContext.samplerStateNearest = [device newSamplerStateWithDescriptor:samplerDescriptor];

    NSMutableArray<id<MTL4CommandAllocator>>* commandAllocators = [NSMutableArray array];
    NSMutableArray<id<MTLSharedEvent>>* events = [NSMutableArray array];
    bd->SharedMetalContext.constantBuffers = [NSMutableArray array];
    for (NSUInteger i = 0; i < bd->SharedMetalContext.framesInFlight; i++)
    {
        events[i] = [device newSharedEvent];
        commandAllocators[i] = [device newCommandAllocator];
        bd->SharedMetalContext.constantBuffers[i] = [NSMutableArray array];
        id<MTLBuffer> buffer = [device newBufferWithLength:sizeof(ImGui_Metal4_ConstantData) options:MTLResourceStorageModeShared];
        [bd->SharedMetalContext.constantBuffers[i] addObject:buffer];
        [bd->SharedMetalContext.residencySet addAllocation:buffer];
    }
    bd->SharedMetalContext.constantBufferChunkCount = 1;
    bd->SharedMetalContext.events = events;

    MTL4ArgumentTableDescriptor* argumentTableDescriptor = [[MTL4ArgumentTableDescriptor alloc] init];
    argumentTableDescriptor.maxBufferBindCount = 2; // vertex buffer + constant buffer
    argumentTableDescriptor.maxTextureBindCount = 1; // font atlas or user texture
    argumentTableDescriptor.maxSamplerStateBindCount = 2;
    argumentTableDescriptor.supportAttributeStrides = YES; // required: vertex buffer is bound via setAddress:stride:atIndex: for stage_in fetch

    bd->SharedMetalContext.commandAllocators = commandAllocators;

    bd->SharedMetalContext.argumentTable = [device newArgumentTableWithDescriptor:argumentTableDescriptor error:&error];
    IM_ASSERT(bd->SharedMetalContext.argumentTable != nil && error == nil);

    ImGui_ImplMetal_CreateDeviceObjectsForPlatformWindows();
    return true;
}

void ImGui_ImplMetal4_DestroyDeviceObjects()
{
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();

    // Destroy all textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1)
            ImGui_ImplMetal4_DestroyTexture(tex);

    [bd->SharedMetalContext.renderPipelineStateCache removeAllObjects];
    bd->SharedMetalContext.samplerStateLinear = nil;
    bd->SharedMetalContext.samplerStateNearest = nil;
    ImGui_ImplMetal_InvalidateDeviceObjectsForPlatformWindows();
    [bd->SharedMetalContext.renderPipelineStateCache removeAllObjects];
}

bool ImGui_ImplMetal4_Init(id<MTLDevice> device, id<MTL4CommandQueue> commandQueue, int framesInFlight)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    IM_ASSERT(framesInFlight > 0 && "framesInFlight must be at least 1!");

    ImGui_ImplMetal4_Data* bd = IM_NEW(ImGui_ImplMetal4_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_metal4";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // We can honor ImGuiPlatformIO::Textures[] requests during render.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.DrawCallback_ResetRenderState = ImGui_ImplMetal4_DrawCallback_ResetRenderState;
    platform_io.DrawCallback_SetSamplerLinear = ImGui_ImplMetal4_DrawCallback_SetSamplerLinear;
    platform_io.DrawCallback_SetSamplerNearest = ImGui_ImplMetal4_DrawCallback_SetSamplerNearest;

    bd->SharedMetalContext = [[MetalContext alloc] init];
    bd->SharedMetalContext.device = device;
    bd->SharedMetalContext.commandQueue = commandQueue;
    bd->SharedMetalContext.framesInFlight = (NSUInteger)framesInFlight;
    NSMutableArray<NSMutableArray<MetalBuffer*>*>* bufferCaches = [NSMutableArray array];
    for (NSUInteger i = 0; i < framesInFlight; i++)
        [bufferCaches addObject:[NSMutableArray array]];
    bd->SharedMetalContext.bufferCaches = bufferCaches;
    
    ImGui_ImplMetal_InitMultiViewportSupport();
    return true;
}

void ImGui_ImplMetal4_Shutdown()
{
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();
    IM_UNUSED(bd);
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGui_ImplMetal_ShutdownMultiViewportSupport();
    ImGui_ImplMetal4_DestroyDeviceObjects();
    ImGui_ImplMetal4_DestroyBackendData();

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    platform_io.ClearRendererHandlers();
}

@implementation MetalBuffer
- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer
{
    if ((self = [super init]))
    {
        _buffer = buffer;
        _lastReuseTime = GetMachAbsoluteTimeInSeconds();
    }
    return self;
}
@end

#pragma mark - FramebufferDescriptor implementation

@implementation FramebufferDescriptor
- (instancetype)initWithRenderPassDescriptor:(MTL4RenderPassDescriptor*)renderPassDescriptor
{
    if ((self = [super init]))
    {
        _sampleCount = renderPassDescriptor.colorAttachments[0].texture.sampleCount;
        _colorPixelFormat = renderPassDescriptor.colorAttachments[0].texture.pixelFormat;
        _depthPixelFormat = renderPassDescriptor.depthAttachment.texture.pixelFormat;
        _stencilPixelFormat = renderPassDescriptor.stencilAttachment.texture.pixelFormat;
    }
    return self;
}

- (nonnull id)copyWithZone:(nullable NSZone*)zone
{
    FramebufferDescriptor* copy = [[FramebufferDescriptor allocWithZone:zone] init];
    copy.sampleCount = self.sampleCount;
    copy.colorPixelFormat = self.colorPixelFormat;
    copy.depthPixelFormat = self.depthPixelFormat;
    copy.stencilPixelFormat = self.stencilPixelFormat;
    return copy;
}

- (NSUInteger)hash
{
    NSUInteger sc = _sampleCount & 0x3;
    NSUInteger cf = _colorPixelFormat & 0x3FF;
    NSUInteger df = _depthPixelFormat & 0x3FF;
    NSUInteger sf = _stencilPixelFormat & 0x3FF;
    NSUInteger hash = (sf << 22) | (df << 12) | (cf << 2) | sc;
    return hash;
}

- (BOOL)isEqual:(id)object
{
    FramebufferDescriptor* other = object;
    if (![other isKindOfClass:[FramebufferDescriptor class]])
        return NO;
    return other.sampleCount == self.sampleCount      &&
    other.colorPixelFormat   == self.colorPixelFormat &&
    other.depthPixelFormat   == self.depthPixelFormat &&
    other.stencilPixelFormat == self.stencilPixelFormat;
}

@end

#pragma mark - MetalTexture implementation

@implementation MetalTexture
- (instancetype)initWithTexture:(id<MTLTexture>)metalTexture
{
    if ((self = [super init]))
        self.metalTexture = metalTexture;
    return self;
}

@end

#pragma mark - MetalContext implementation

@implementation MetalContext
- (instancetype)init
{
    if ((self = [super init]))
    {
        self.renderPipelineStateCache = [NSMutableDictionary dictionary];
        self.bufferCacheLock = [[NSObject alloc] init];
        _lastBufferCachePurge = GetMachAbsoluteTimeInSeconds();
    }
    return self;
}

- (void)dealloc
{
}

- (MetalBuffer*)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device
{
    double now = GetMachAbsoluteTimeInSeconds();
    NSMutableArray<MetalBuffer*>* slotCache = self.bufferCaches[self.currentFrameSlot];

    @synchronized(self.bufferCacheLock)
    {
        // Purge old buffers that haven't been useful for a while
        if (now - self.lastBufferCachePurge > 1.0)
        {
            for (NSMutableArray<MetalBuffer*>* cache in self.bufferCaches)
            {
                NSMutableArray* survivors = [NSMutableArray array];
                for (MetalBuffer* candidate in cache)
                    if (candidate.lastReuseTime > self.lastBufferCachePurge)
                        [survivors addObject:candidate];
                [cache setArray:survivors];
            }
            self.lastBufferCachePurge = now;
        }

        // See if we have a buffer we can reuse, from this frame-in-flight slot's own cache
        MetalBuffer* bestCandidate = nil;
        for (MetalBuffer* candidate in slotCache)
            if (candidate.buffer.length >= length && (bestCandidate == nil || bestCandidate.lastReuseTime > candidate.lastReuseTime))
                bestCandidate = candidate;

        if (bestCandidate != nil)
        {
            [slotCache removeObject:bestCandidate];
            bestCandidate.lastReuseTime = now;
            return bestCandidate;
        }
    }

    // No luck; make a new buffer
    id<MTLBuffer> backing = [device newBufferWithLength:length options:MTLResourceStorageModeShared];
    [self.residencySet addAllocation:backing];
    return [[MetalBuffer alloc] initWithBuffer:backing];
}

const char* shaderCode = R"(
#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float4x4 projectionMatrix;
};

struct VertexIn {
    float2 position  [[attribute(0)]];
    float2 texCoords [[attribute(1)]];
    uchar4 color     [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoords;
    float4 color;
};

vertex VertexOut vertex_main(VertexIn in                 [[stage_in]],
                             constant Uniforms &uniforms [[buffer(1)]])
{
    VertexOut out;
    out.position = uniforms.projectionMatrix * float4(in.position, 0, 1);
    out.texCoords = in.texCoords;
    out.color = float4(in.color) / float4(255.0);
    return out;
}

fragment half4 fragment_main(VertexOut in [[stage_in]],
                             texture2d<half, access::sample> texture [[texture(0)]],
                             sampler textureSampler [[sampler(0)]])
{
    half4 texColor = texture.sample(textureSampler, in.texCoords);
    return half4(in.color) * texColor;
}
)";

- (id<MTLRenderPipelineState>)renderPipelineStateForFramebufferDescriptor:(FramebufferDescriptor*)descriptor device:(id<MTLDevice>)device
{
    NSError* error = nil;

    id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:shaderCode] options:nil error:&error];
    if (library == nil)
    {
        NSLog(@"Error: failed to create Metal library: %@", error);
        return nil;
    }

    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_main"];

    if (vertexFunction == nil || fragmentFunction == nil)
    {
        NSLog(@"Error: failed to find Metal shader functions in library: %@", error);
        return nil;
    }

    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    vertexDescriptor.attributes[0].offset = offsetof(ImDrawVert, pos);
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2; // position
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].offset = offsetof(ImDrawVert, uv);
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2; // texCoords
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.attributes[2].offset = offsetof(ImDrawVert, col);
    vertexDescriptor.attributes[2].format = MTLVertexFormatUChar4; // color
    vertexDescriptor.attributes[2].bufferIndex = 0;
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride = sizeof(ImDrawVert);

    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    pipelineDescriptor.rasterSampleCount = self.framebufferDescriptor.sampleCount;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.framebufferDescriptor.colorPixelFormat;
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.depthAttachmentPixelFormat = self.framebufferDescriptor.depthPixelFormat;
    pipelineDescriptor.stencilAttachmentPixelFormat = self.framebufferDescriptor.stencilPixelFormat;

    id<MTLRenderPipelineState> renderPipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (error != nil)
        NSLog(@"Error: failed to create Metal pipeline state: %@", error);

    return renderPipelineState;
}

@end


#pragma mark - Multi-viewport support

#import <QuartzCore/CAMetalLayer.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#endif

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the back-end to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

struct ImGuiViewportDataMetal
{
    CAMetalLayer*               MetalLayer;
    id<MTLCommandQueue>         CommandQueue;
    MTL4RenderPassDescriptor*   RenderPassDescriptor;
    void*                       Handle = nullptr;
    bool                        FirstFrame = true;
};

static void ImGui_ImplMetal_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();
    ImGuiViewportDataMetal* data = IM_NEW(ImGuiViewportDataMetal)();
    viewport->RendererUserData = data;

    // PlatformHandleRaw should always be a NSWindow*, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
    // Some back-ends will leave PlatformHandleRaw == 0, in which case we assume PlatformHandle will contain the NSWindow*.
    void* handle = viewport->PlatformHandleRaw ? viewport->PlatformHandleRaw : viewport->PlatformHandle;
    IM_ASSERT(handle != nullptr);

    id<MTLDevice> device = bd->SharedMetalContext.device;
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = device;
    layer.framebufferOnly = YES;
    layer.pixelFormat = bd->SharedMetalContext.framebufferDescriptor.colorPixelFormat;
#if TARGET_OS_OSX
    NSWindow* window = (__bridge NSWindow*)handle;
    NSView* view = window.contentView;
    view.layer = layer;
    view.wantsLayer = YES;
#endif
    data->MetalLayer = layer;
    data->CommandQueue = [device newCommandQueue];
    data->RenderPassDescriptor = [[MTL4RenderPassDescriptor alloc] init];
    data->Handle = handle;
}

static void ImGui_ImplMetal_DestroyWindow(ImGuiViewport* viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == 0 since we didn't create the data for it.
    if (ImGuiViewportDataMetal* data = (ImGuiViewportDataMetal*)viewport->RendererUserData)
        IM_DELETE(data);
    viewport->RendererUserData = nullptr;
}

inline static CGSize MakeScaledSize(CGSize size, CGFloat scale)
{
    return CGSizeMake(size.width * scale, size.height * scale);
}

static void ImGui_ImplMetal_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiViewportDataMetal* data = (ImGuiViewportDataMetal*)viewport->RendererUserData;
    data->MetalLayer.drawableSize = MakeScaledSize(CGSizeMake(size.x, size.y), viewport->DpiScale);
}

static void ImGui_ImplMetal_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGuiViewportDataMetal* data = (ImGuiViewportDataMetal*)viewport->RendererUserData;

#if TARGET_OS_OSX
    void* handle = viewport->PlatformHandleRaw ? viewport->PlatformHandleRaw : viewport->PlatformHandle;
    NSWindow* window = (__bridge NSWindow*)handle;

    // Always render the first frame, regardless of occlusionState, to avoid an initial flicker
    if ((window.occlusionState & NSWindowOcclusionStateVisible) == 0 && !data->FirstFrame)
    {
        // Do not render windows which are completely occluded. Calling -[CAMetalLayer nextDrawable] will hang for
        // approximately 1 second if the Metal layer is completely occluded.
        return;
    }
    data->FirstFrame = false;

    float fb_scale = (float)window.backingScaleFactor;
    if (data->MetalLayer.contentsScale != fb_scale)
    {
        data->MetalLayer.contentsScale = fb_scale;
        data->MetalLayer.drawableSize = MakeScaledSize(window.frame.size, fb_scale);
    }
#endif

    id <CAMetalDrawable> drawable = [data->MetalLayer nextDrawable];
    if (drawable == nil)
        return;

    MTL4RenderPassDescriptor* renderPassDescriptor = data->RenderPassDescriptor;
    renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    if ((viewport->Flags & ImGuiViewportFlags_NoRendererClear) == 0)
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

    ImGui_ImplMetal4_Data* bd = ImGui_ImplMetal4_GetBackendData();
    id <MTL4CommandBuffer> commandBuffer = [bd->SharedMetalContext.device newCommandBuffer];
    [commandBuffer beginCommandBufferWithAllocator:bd->SharedMetalContext.commandAllocators[bd->SharedMetalContext.currentFrameSlot]];

    id <MTL4RenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    ImGui_ImplMetal4_RenderDrawData(viewport->DrawData, commandBuffer, renderEncoder);
    [renderEncoder endEncoding];
    [commandBuffer endCommandBuffer];

    [bd->SharedMetalContext.commandQueue waitForDrawable:drawable];
    [bd->SharedMetalContext.commandQueue commit:&commandBuffer count:1];
    [bd->SharedMetalContext.commandQueue signalDrawable:drawable];
    [drawable present];
    [bd->SharedMetalContext.commandQueue signalEvent:bd->SharedMetalContext.events[bd->SharedMetalContext.currentFrameSlot] value:++(bd->SharedMetalContext.eventValue)];
}

static void ImGui_ImplMetal_InitMultiViewportSupport()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = ImGui_ImplMetal_CreateWindow;
    platform_io.Renderer_DestroyWindow = ImGui_ImplMetal_DestroyWindow;
    platform_io.Renderer_SetWindowSize = ImGui_ImplMetal_SetWindowSize;
    platform_io.Renderer_RenderWindow = ImGui_ImplMetal_RenderWindow;
}

static void ImGui_ImplMetal_ShutdownMultiViewportSupport()
{
    ImGui::DestroyPlatformWindows();
}

static void ImGui_ImplMetal_CreateDeviceObjectsForPlatformWindows()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++)
        if (!platform_io.Viewports[i]->RendererUserData)
            ImGui_ImplMetal_CreateWindow(platform_io.Viewports[i]);
}

static void ImGui_ImplMetal_InvalidateDeviceObjectsForPlatformWindows()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++)
        if (platform_io.Viewports[i]->RendererUserData)
            ImGui_ImplMetal_DestroyWindow(platform_io.Viewports[i]);
}

//-----------------------------------------------------------------------------


#endif // #ifndef IMGUI_DISABLE

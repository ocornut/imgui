// dear imgui: Renderer for Metal
// This needs to be used along with a Platform Binding (e.g. OSX)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'MTLTexture' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-07-05: Metal: Added new Metal backend implementation.

#include "imgui.h"
#include "imgui_impl_metal.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <simd/simd.h>

#pragma mark - Support classes

// A wrapper around a MTLBuffer object that knows the last time it was reused
@interface MetalBuffer : NSObject
@property (nonatomic, strong) id<MTLBuffer> buffer;
@property (nonatomic, assign) NSTimeInterval lastReuseTime;
- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer;
@end

// An object that encapsulates the data necessary to uniquely identify a
// render pipeline state. These are used as cache keys.
@interface FramebufferDescriptor : NSObject<NSCopying>
@property (nonatomic, assign) unsigned long sampleCount;
@property (nonatomic, assign) MTLPixelFormat colorPixelFormat;
@property (nonatomic, assign) MTLPixelFormat depthPixelFormat;
@property (nonatomic, assign) MTLPixelFormat stencilPixelFormat;
- (instancetype)initWithRenderPassDescriptor:(MTLRenderPassDescriptor *)renderPassDescriptor;
@end

// A singleton that stores long-lived objects that are needed by the Metal
// renderer backend. Stores the render pipeline state cache and the default
// font texture, and manages the reusable buffer cache.
@interface MetalContext : NSObject
@property (nonatomic, strong) id<MTLDepthStencilState> depthStencilState;
@property (nonatomic, strong) FramebufferDescriptor *framebufferDescriptor; // framebuffer descriptor for current frame; transient
@property (nonatomic, strong) NSMutableDictionary *renderPipelineStateCache; // pipeline cache; keyed on framebuffer descriptors
@property (nonatomic, strong, nullable) id<MTLTexture> fontTexture;
@property (nonatomic, strong) NSMutableArray<MetalBuffer *> *bufferCache;
@property (nonatomic, assign) NSTimeInterval lastBufferCachePurge;
- (void)makeDeviceObjectsWithDevice:(id<MTLDevice>)device;
- (void)makeFontTextureWithDevice:(id<MTLDevice>)device;
- (MetalBuffer *)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device;
- (void)enqueueReusableBuffer:(MetalBuffer *)buffer;
- (id<MTLRenderPipelineState>)renderPipelineStateForFrameAndDevice:(id<MTLDevice>)device;
- (void)emptyRenderPipelineStateCache;
- (void)renderDrawData:(ImDrawData *)drawData
         commandBuffer:(id<MTLCommandBuffer>)commandBuffer
        commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder;
@end

static MetalContext *g_sharedMetalContext = nil;

#pragma mark - ImGui API implementation

bool ImGui_ImplMetal_Init(id<MTLDevice> device)
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        g_sharedMetalContext = [[MetalContext alloc] init];
    });
    
    ImGui_ImplMetal_CreateDeviceObjects(device);

    return true;
}

void ImGui_ImplMetal_Shutdown()
{
    ImGui_ImplMetal_DestroyDeviceObjects();
}

void ImGui_ImplMetal_NewFrame(MTLRenderPassDescriptor *renderPassDescriptor)
{
    IM_ASSERT(g_sharedMetalContext != nil && "No Metal context. Did you call ImGui_ImplMetal_Init?");

    g_sharedMetalContext.framebufferDescriptor = [[FramebufferDescriptor alloc] initWithRenderPassDescriptor:renderPassDescriptor];
}

// Metal Render function.
void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data, id<MTLCommandBuffer> commandBuffer, id<MTLRenderCommandEncoder> commandEncoder)
{
    [g_sharedMetalContext renderDrawData:draw_data commandBuffer:commandBuffer commandEncoder:commandEncoder];
}

bool ImGui_ImplMetal_CreateFontsTexture(id<MTLDevice> device)
{
    [g_sharedMetalContext makeFontTextureWithDevice:device];
    
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->TexID = (__bridge void *)g_sharedMetalContext.fontTexture; // ImTextureID == void*

    return (g_sharedMetalContext.fontTexture != nil);
}

void ImGui_ImplMetal_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    g_sharedMetalContext.fontTexture = nil;
    io.Fonts->TexID = nullptr;
}

bool ImGui_ImplMetal_CreateDeviceObjects(id<MTLDevice> device)
{
    [g_sharedMetalContext makeDeviceObjectsWithDevice:device];

    ImGui_ImplMetal_CreateFontsTexture(device);

    return true;
}

void ImGui_ImplMetal_DestroyDeviceObjects()
{
    ImGui_ImplMetal_DestroyFontsTexture();
    [g_sharedMetalContext emptyRenderPipelineStateCache];
}

#pragma mark - MetalBuffer implementation

@implementation MetalBuffer
- (instancetype)initWithBuffer:(id<MTLBuffer>)buffer 
{
    if ((self = [super init])) 
    {
        _buffer = buffer;
        _lastReuseTime = [NSDate date].timeIntervalSince1970;
    }
    return self;
}
@end

#pragma mark - FramebufferDescriptor implementation

@implementation FramebufferDescriptor
- (instancetype)initWithRenderPassDescriptor:(MTLRenderPassDescriptor *)renderPassDescriptor 
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

- (nonnull id)copyWithZone:(nullable NSZone *)zone 
{
    FramebufferDescriptor *copy = [[FramebufferDescriptor allocWithZone:zone] init];
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
    FramebufferDescriptor *other = object;
    if (![other isKindOfClass:[FramebufferDescriptor class]])
        return NO;
    return other.sampleCount == self.sampleCount      &&
    other.colorPixelFormat   == self.colorPixelFormat &&
    other.depthPixelFormat   == self.depthPixelFormat &&
    other.stencilPixelFormat == self.stencilPixelFormat;
}

@end

#pragma mark - MetalContext implementation

@implementation MetalContext
- (instancetype)init {
    if ((self = [super init])) 
    {
        _renderPipelineStateCache = [NSMutableDictionary dictionary];
        _bufferCache = [NSMutableArray array];
        _lastBufferCachePurge = [NSDate date].timeIntervalSince1970;
    }
    return self;
}

- (void)makeDeviceObjectsWithDevice:(id<MTLDevice>)device 
{
    MTLDepthStencilDescriptor *depthStencilDescriptor = [[MTLDepthStencilDescriptor alloc] init];
    depthStencilDescriptor.depthWriteEnabled = NO;
    depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
    self.depthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
}

// We are retrieving and uploading the font atlas as a 4-channels RGBA texture here.
// In theory we could call GetTexDataAsAlpha8() and upload a 1-channel texture to save on memory access bandwidth.
// However, using a shader designed for 1-channel texture would make it less obvious to use the ImTextureID facility to render users own textures.
// You can make that change in your implementation.
- (void)makeFontTextureWithDevice:(id<MTLDevice>)device 
{
    ImGuiIO &io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                 width:width
                                                                                                height:height
                                                                                             mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead;
#if TARGET_OS_OSX
    textureDescriptor.storageMode = MTLStorageModeManaged;
#else
    textureDescriptor.storageMode = MTLStorageModeShared;
#endif
    id <MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    [texture replaceRegion:MTLRegionMake2D(0, 0, width, height) mipmapLevel:0 withBytes:pixels bytesPerRow:width * 4];
    self.fontTexture = texture;
}

- (MetalBuffer *)dequeueReusableBufferOfLength:(NSUInteger)length device:(id<MTLDevice>)device 
{
    NSTimeInterval now = [NSDate date].timeIntervalSince1970;
    
    // Purge old buffers that haven't been useful for a while
    if (now - self.lastBufferCachePurge > 1.0) 
    {
        NSMutableArray *survivors = [NSMutableArray array];
        for (MetalBuffer *candidate in self.bufferCache) 
        {
            if (candidate.lastReuseTime > self.lastBufferCachePurge) 
            {
                [survivors addObject:candidate];
            }
        }
        self.bufferCache = [survivors mutableCopy];
        self.lastBufferCachePurge = now;
    }
    
    // See if we have a buffer we can reuse
    MetalBuffer *bestCandidate = nil;
    for (MetalBuffer *candidate in self.bufferCache) 
        if (candidate.buffer.length >= length && (bestCandidate == nil || bestCandidate.lastReuseTime > candidate.lastReuseTime))
            bestCandidate = candidate;
    
    if (bestCandidate != nil) 
    {
        [self.bufferCache removeObject:bestCandidate];
        bestCandidate.lastReuseTime = now;
        return bestCandidate;
    }
    
    // No luck; make a new buffer
    id<MTLBuffer> backing = [device newBufferWithLength:length options:MTLResourceStorageModeShared];
    return [[MetalBuffer alloc] initWithBuffer:backing];
}

- (void)enqueueReusableBuffer:(MetalBuffer *)buffer 
{
    [self.bufferCache addObject:buffer];
}

- (_Nullable id<MTLRenderPipelineState>)renderPipelineStateForFrameAndDevice:(id<MTLDevice>)device 
{
    // Try to retrieve a render pipeline state that is compatible with the framebuffer config for this frame
    // Thie hit rate for this cache should be very near 100%.
    id<MTLRenderPipelineState> renderPipelineState = self.renderPipelineStateCache[self.framebufferDescriptor];
    
    if (renderPipelineState == nil) 
    {
        // No luck; make a new render pipeline state
        renderPipelineState = [self _renderPipelineStateForFramebufferDescriptor:self.framebufferDescriptor device:device];
        // Cache render pipeline state for later reuse
        self.renderPipelineStateCache[self.framebufferDescriptor] = renderPipelineState;
    }
    
    return renderPipelineState;
}

- (id<MTLRenderPipelineState>)_renderPipelineStateForFramebufferDescriptor:(FramebufferDescriptor *)descriptor device:(id<MTLDevice>)device
{
    NSError *error = nil;
    
    NSString *shaderSource = @""
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
    "}\n";
    
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
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
    
    MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    vertexDescriptor.attributes[0].offset = IM_OFFSETOF(ImDrawVert, pos);
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2; // position
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].offset = IM_OFFSETOF(ImDrawVert, uv);
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2; // texCoords
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.attributes[2].offset = IM_OFFSETOF(ImDrawVert, col);
    vertexDescriptor.attributes[2].format = MTLVertexFormatUChar4; // color
    vertexDescriptor.attributes[2].bufferIndex = 0;
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride = sizeof(ImDrawVert);
    
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    pipelineDescriptor.sampleCount = self.framebufferDescriptor.sampleCount;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.framebufferDescriptor.colorPixelFormat;
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.depthAttachmentPixelFormat = self.framebufferDescriptor.depthPixelFormat;
    pipelineDescriptor.stencilAttachmentPixelFormat = self.framebufferDescriptor.stencilPixelFormat;
    
    id<MTLRenderPipelineState> renderPipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (error != nil) 
    {
        NSLog(@"Error: failed to create Metal pipeline state: %@", error);
    }
    
    return renderPipelineState;
}

- (void)emptyRenderPipelineStateCache 
{
    [self.renderPipelineStateCache removeAllObjects];
}

- (void)renderDrawData:(ImDrawData *)drawData
         commandBuffer:(id<MTLCommandBuffer>)commandBuffer
        commandEncoder:(id<MTLRenderCommandEncoder>)commandEncoder
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO &io = ImGui::GetIO();
    int fb_width = (int)(drawData->DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || drawData->CmdListsCount == 0)
        return;
    drawData->ScaleClipRects(io.DisplayFramebufferScale);
    
    [commandEncoder setCullMode:MTLCullModeNone];
    [commandEncoder setDepthStencilState:g_sharedMetalContext.depthStencilState];
    
    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
    MTLViewport viewport = 
    {   
        .originX = 0.0,
        .originY = 0.0,
        .width = double(fb_width),
        .height = double(fb_height),
        .znear = 0.0,
        .zfar = 1.0 
    };
    [commandEncoder setViewport:viewport];
    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    float N = viewport.znear;
    float F = viewport.zfar;
    const float ortho_projection[4][4] =
    {
        { 2.0f/(R-L),   0.0f,           0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),     0.0f,   0.0f },
        { 0.0f,         0.0f,        1/(F-N),   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T), N/(F-N),   1.0f },
    };
    
    [commandEncoder setVertexBytes:&ortho_projection length:sizeof(ortho_projection) atIndex:1];
    
    size_t vertexBufferLength = 0;
    size_t indexBufferLength = 0;
    for (int n = 0; n < drawData->CmdListsCount; n++) 
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        vertexBufferLength += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        indexBufferLength += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }
    
    MetalBuffer *vertexBuffer = [self dequeueReusableBufferOfLength:vertexBufferLength device:commandBuffer.device];
    MetalBuffer *indexBuffer = [self dequeueReusableBufferOfLength:indexBufferLength device:commandBuffer.device];
    
    id<MTLRenderPipelineState> renderPipelineState = [self renderPipelineStateForFrameAndDevice:commandBuffer.device];
    [commandEncoder setRenderPipelineState:renderPipelineState];
    
    [commandEncoder setVertexBuffer:vertexBuffer.buffer offset:0 atIndex:0];
    
    size_t vertexBufferOffset = 0;
    size_t indexBufferOffset = 0;
    ImVec2 pos = drawData->DisplayPos;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        ImDrawIdx idx_buffer_offset = 0;
        
        memcpy((char *)vertexBuffer.buffer.contents + vertexBufferOffset, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy((char *)indexBuffer.buffer.contents + indexBufferOffset, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        
        [commandEncoder setVertexBufferOffset:vertexBufferOffset atIndex:0];
        
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback (registered via ImDrawList::AddCallback)
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                ImVec4 clip_rect = ImVec4(pcmd->ClipRect.x - pos.x, pcmd->ClipRect.y - pos.y, pcmd->ClipRect.z - pos.x, pcmd->ClipRect.w - pos.y);
                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    // Apply scissor/clipping rectangle
                    MTLScissorRect scissorRect = { .x = NSUInteger(clip_rect.x),
                        .y = NSUInteger(clip_rect.y),
                        .width = NSUInteger(clip_rect.z - clip_rect.x),
                        .height = NSUInteger(clip_rect.w - clip_rect.y) };
                    [commandEncoder setScissorRect:scissorRect];
                    
                    
                    // Bind texture, Draw
                    if (pcmd->TextureId != NULL)
                        [commandEncoder setFragmentTexture:(__bridge id<MTLTexture>)(pcmd->TextureId) atIndex:0];
                    [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                               indexCount:pcmd->ElemCount
                                                indexType:sizeof(ImDrawIdx) == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32
                                              indexBuffer:indexBuffer.buffer
                                        indexBufferOffset:indexBufferOffset + idx_buffer_offset];
                }
            }
            idx_buffer_offset += pcmd->ElemCount * sizeof(ImDrawIdx);
        }
        
        vertexBufferOffset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        indexBufferOffset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }
    
    __weak id weakSelf = self;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer>) 
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf enqueueReusableBuffer:vertexBuffer];
            [weakSelf enqueueReusableBuffer:indexBuffer];
        });
    }];
}

@end

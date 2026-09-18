// Dear ImGui: standalone example application for OSX + Metal 4.
// Uses CAMetalLayer directly (instead of MTKView) for glitch-free resizing.
// Based on Tristan Hume's MetalLayerView pattern with presentsWithTransaction.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#import <Foundation/Foundation.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#else
#import <UIKit/UIKit.h>
#endif

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "imgui.h"
#include "imgui_impl_metal4.h"
#if TARGET_OS_OSX
#include "imgui_impl_osx.h"
#endif

//-----------------------------------------------------------------------------------
// MetalLayerView — custom view backed by CAMetalLayer
//-----------------------------------------------------------------------------------

// Render callback type: the view calls this to draw a frame
typedef void (^RenderCallback)(CAMetalLayer* layer, CGSize viewSize, CGFloat scaleFactor);

// Idle CPU throttling: after the last user input (or while a widget is active) keep rendering at the full
// display rate for this many frames, then fall back to a slow heartbeat so the main thread can idle.
static const NSInteger kActiveRenderFrames = 30;

#if TARGET_OS_OSX

@interface MetalLayerView : NSView <CALayerDelegate>
@property (nonatomic, strong) CAMetalLayer* metalLayer;
@property (nonatomic, copy) RenderCallback renderCallback;
@property (atomic, assign) NSInteger activeFrames; // >0 => keep driving redraws; read on the display-link thread
@end

@implementation MetalLayerView

-(instancetype)initWithFrame:(NSRect)frame device:(id<MTLDevice>)device
{
    self = [super initWithFrame:frame];
    if (self)
    {
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
        self.layerContentsPlacement = NSViewLayerContentsPlacementScaleAxesIndependently;

        _metalLayer = (CAMetalLayer*)self.layer;
        _metalLayer.device = device;
        _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        _metalLayer.delegate = self;
        _metalLayer.allowsNextDrawableTimeout = NO;
        _metalLayer.autoresizingMask = kCALayerHeightSizable | kCALayerWidthSizable;
        _metalLayer.needsDisplayOnBoundsChange = YES;
        // Default to async presentation. presentsWithTransaction is turned on only during a live resize
        // (in renderWithLayer:) so content stays glued to the window frame; see the note there and in
        // displayLinkCallback for why it must stay off otherwise.
        _metalLayer.presentsWithTransaction = NO;
        _activeFrames = kActiveRenderFrames; // render at startup until things settle
    }
    return self;
}

-(CALayer*)makeBackingLayer
{
    CAMetalLayer* layer = [CAMetalLayer layer];
    return layer;
}

-(BOOL)wantsUpdateLayer { return YES; }

-(void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    _metalLayer.drawableSize = [self convertSizeToBacking:newSize];
}

-(void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    if (self.window)
    {
        _metalLayer.contentsScale = self.window.backingScaleFactor;
        // Moving to a display with a different backing scale changes the pixel size without changing the
        // point size, so setFrameSize: is not called — update the drawable resolution here too.
        _metalLayer.drawableSize = [self convertSizeToBacking:self.bounds.size];
    }
}

-(void)displayLayer:(CALayer*)layer
{
    if (_renderCallback)
    {
        CGFloat scale = self.window.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
        _renderCallback(_metalLayer, self.bounds.size, scale);
    }
}

@end

#else // iOS

@interface MetalLayerView : UIView
@property (nonatomic, strong) CAMetalLayer* metalLayer;
@property (nonatomic, copy) RenderCallback renderCallback;
@property (nonatomic, strong) CADisplayLink* displayLink;
@end

@implementation MetalLayerView

+(Class)layerClass
{
    return [CAMetalLayer class];
}

-(instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device
{
    self = [super initWithFrame:frame];
    if (self)
    {
        _metalLayer = (CAMetalLayer*)self.layer;
        _metalLayer.device = device;
        _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        _metalLayer.framebufferOnly = YES;
        self.contentScaleFactor = UIScreen.mainScreen.scale;
        _metalLayer.drawableSize = CGSizeMake(frame.size.width * self.contentScaleFactor,
                                               frame.size.height * self.contentScaleFactor);
    }
    return self;
}

-(void)layoutSubviews
{
    [super layoutSubviews];
    _metalLayer.drawableSize = CGSizeMake(self.bounds.size.width * self.contentScaleFactor,
                                           self.bounds.size.height * self.contentScaleFactor);
}

-(void)startDisplayLink
{
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkFired:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

-(void)stopDisplayLink
{
    [_displayLink invalidate];
    _displayLink = nil;
}

-(void)displayLinkFired:(CADisplayLink*)link
{
    if (_renderCallback)
    {
        CGFloat scale = self.contentScaleFactor;
        _renderCallback(_metalLayer, self.bounds.size, scale);
    }
}

@end

#endif

//-----------------------------------------------------------------------------------
// AppViewController
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX
@interface AppViewController : NSViewController<NSWindowDelegate>
@end
#else
@interface AppViewController : UIViewController
@end
#endif

@interface AppViewController ()
@property (nonatomic, strong) MetalLayerView* metalView;
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTL4CommandQueue> commandQueue;
@property (nonatomic, strong) NSArray<id<MTL4CommandAllocator>>* commandAllocators;
@property (nonatomic, strong) id<MTLSharedEvent> frameEvent;
@property (nonatomic, assign) uint64_t frameValue;
@property (nonatomic, strong) dispatch_semaphore_t inFlightSemaphore;
@property (nonatomic, strong) MTLSharedEventListener* frameListener;
@property (nonatomic, assign) uint64_t frameCount;
#if TARGET_OS_OSX
@property (nonatomic, assign) CVDisplayLinkRef displayLink;
@property (nonatomic, strong) id eventMonitor; // local NSEvent monitor that keeps rendering active on input
#endif
@end

#define FRAMES_IN_FLIGHT 2

@implementation AppViewController

-(instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];

    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newMTL4CommandQueue];

    // One command allocator per frame-in-flight slot so a slot's allocator is only reset/reused once
    // its previous frame's GPU work has completed (gated by the semaphore below).
    NSMutableArray<id<MTL4CommandAllocator>>* commandAllocators = [NSMutableArray array];
    for (NSUInteger i = 0; i < FRAMES_IN_FLIGHT; i++)
        [commandAllocators addObject:[_device newCommandAllocator]];
    _commandAllocators = commandAllocators;

    _frameEvent = [_device newSharedEvent];
    _frameValue = 0;
    _frameCount = 0;

    // Frames-in-flight gate: never let the CPU get more than FRAMES_IN_FLIGHT frames ahead of the GPU,
    // so per-slot resources (command allocators, constant buffers, argument tables) are safely double
    // buffered. The listener signals the semaphore from a serial queue when a frame's GPU work finishes.
    dispatch_queue_t listenerQueue = dispatch_queue_create("imgui.metal4.frameListener", DISPATCH_QUEUE_SERIAL);
    _frameListener = [[MTLSharedEventListener alloc] initWithDispatchQueue:listenerQueue];
    _inFlightSemaphore = dispatch_semaphore_create(FRAMES_IN_FLIGHT);

    if (!self.device)
    {
        NSLog(@"Metal is not supported");
        abort();
    }

    // Setup Dear ImGui context
    // FIXME: This example doesn't have proper cleanup...
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Renderer backend
    ImGui_ImplMetal4_Init(_device, _commandQueue, FRAMES_IN_FLIGHT);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    return self;
}

-(void)loadView
{
#if TARGET_OS_OSX
    self.metalView = [[MetalLayerView alloc] initWithFrame:CGRectMake(0, 0, 1200, 800) device:self.device];
#else
    self.metalView = [[MetalLayerView alloc] initWithFrame:UIScreen.mainScreen.bounds device:self.device];
#endif
    self.view = self.metalView;
}

-(void)viewDidLoad
{
    [super viewDidLoad];

#if TARGET_OS_OSX
    ImGui_ImplOSX_Init(self.view);
    [NSApp activateIgnoringOtherApps:YES];
#endif

    // Setup render callback — captures self weakly to avoid retain cycle
    __weak AppViewController* weakSelf = self;
    self.metalView.renderCallback = ^(CAMetalLayer* layer, CGSize viewSize, CGFloat scaleFactor) {
        [weakSelf renderWithLayer:layer viewSize:viewSize scaleFactor:scaleFactor];
    };

#if TARGET_OS_OSX
    // Keep rendering at the full rate for a short grace period after any user input, so hover/typing/drag
    // stay responsive; the display-link callback throttles once this decays. (See kActiveRenderFrames.)
    NSEventMask inputMask = NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskRightMouseDown |
                            NSEventMaskRightMouseUp | NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp |
                            NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged |
                            NSEventMaskOtherMouseDragged | NSEventMaskScrollWheel | NSEventMaskKeyDown |
                            NSEventMaskKeyUp | NSEventMaskFlagsChanged;
    __weak MetalLayerView* weakView = self.metalView;
    self.eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:inputMask handler:^NSEvent* (NSEvent* event) {
        weakView.activeFrames = kActiveRenderFrames;
        return event;
    }];

    // Start CVDisplayLink for continuous rendering
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &displayLinkCallback, (__bridge void*)self.metalView);
    CVDisplayLinkStart(_displayLink);
#else
    [self.metalView startDisplayLink];
#endif
}

#if TARGET_OS_OSX
static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink,
                                     const CVTimeStamp* now,
                                     const CVTimeStamp* outputTime,
                                     CVOptionFlags flagsIn,
                                     CVOptionFlags* flagsOut,
                                     void* context)
{
    MetalLayerView* view = (__bridge MetalLayerView*)context;
    // Idle throttle: only drive a redraw when the UI was recently active (view.activeFrames, bumped by
    // input and by active widgets), or on a slow heartbeat so time-based animations still advance and we
    // periodically re-check for activity. When idle we don't even wake the main thread. (activeFrames is
    // an atomic property; this runs on the display-link thread.)
    static uint64_t tick = 0;
    tick++;
    if (view.activeFrames <= 0 && (tick % 60) != 0)
        return kCVReturnSuccess;

    dispatch_async(dispatch_get_main_queue(), ^{
        // While the OS is live-resizing the window, AppKit already redraws the layer each step inside its
        // resize CATransaction (which commits, releasing the in-transaction drawable). Driving an extra
        // render from the display link here would present in-transaction WITHOUT a commit and starve the
        // drawable pool, so skip it during resize and let AppKit be the sole driver.
        if (!view.inLiveResize)
            [view setNeedsDisplay:YES];
    });
    return kCVReturnSuccess;
}
#endif

-(void)renderWithLayer:(CAMetalLayer*)layer viewSize:(CGSize)viewSize scaleFactor:(CGFloat)scaleFactor
{
    // Frames-in-flight gate: block until a slot is free (its previous frame's GPU work has completed).
    // Once past this, the slot's per-frame resources are guaranteed no longer read by the GPU.
    dispatch_semaphore_wait(self.inFlightSemaphore, DISPATCH_TIME_FOREVER);
    NSUInteger slot = self.frameCount % FRAMES_IN_FLIGHT;
    self.frameCount++;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = viewSize.width;
    io.DisplaySize.y = viewSize.height;
    io.DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);

#if TARGET_OS_OSX
    // While the OS is live-resizing the window, present in-transaction so the content stays glued to the
    // window frame (glitch-free resize). This is safe because during a live resize AppKit drives the
    // redraw inside its resize transaction — which commits and releases the drawable — while the display
    // link stands down (see displayLinkCallback). Async otherwise, to avoid a per-frame stall.
    layer.presentsWithTransaction = self.metalView.inLiveResize;
#endif

    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (!drawable)
    {
        // Release the gate we just took, otherwise the semaphore count leaks and we deadlock.
        dispatch_semaphore_signal(self.inFlightSemaphore);
        return;
    }

    [self.commandAllocators[slot] reset];
    id<MTL4CommandBuffer> commandBuffer = [self.device newCommandBuffer];
    [commandBuffer beginCommandBufferWithAllocator:self.commandAllocators[slot]];

    // Build the Metal 4 render pass descriptor by hand (MTKView's currentMTL4RenderPassDescriptor is gone).
    MTL4RenderPassDescriptor* renderPassDescriptor = [[MTL4RenderPassDescriptor alloc] init];
    renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

    // Start the Dear ImGui frame
    ImGui_ImplMetal4_NewFrame(renderPassDescriptor, (int)slot);

#if TARGET_OS_OSX
    ImGui_ImplOSX_NewFrame(self.view);
#endif
    ImGui::NewFrame();

    // Our state (make them static = more or less global) as a convenience to keep the example terse.
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

#if TARGET_OS_OSX
    // Idle throttle bookkeeping: keep rendering while the UI is doing something (text-cursor blink, an
    // active/dragged widget, a held mouse button, or a live resize); otherwise let the counter decay so
    // the display-link callback can drop to the idle heartbeat. Mouse movement is handled by the input
    // monitor in viewDidLoad.
    if (io.WantTextInput || ImGui::IsAnyItemActive() || [NSEvent pressedMouseButtons] != 0 || self.metalView.inLiveResize)
        self.metalView.activeFrames = kActiveRenderFrames;
    else if (self.metalView.activeFrames > 0)
        self.metalView.activeFrames = self.metalView.activeFrames - 1;
#endif

    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    id <MTL4RenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
    ImGui_ImplMetal4_RenderDrawData(draw_data, commandBuffer, renderEncoder);
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];
    [commandBuffer endCommandBuffer];

    // Present (canonical Metal 4 sequence, as the original MTKView example used).
    [self.commandQueue waitForDrawable:drawable];
    [self.commandQueue commit:&commandBuffer count:1];
    [self.commandQueue signalDrawable:drawable];
    [drawable present];

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // Release the frames-in-flight gate once all of this frame's GPU work (main + secondary viewports)
    // completes: the listener signals the semaphore when the shared event reaches this frame's value.
    // Capture the semaphore itself — not self — so the completion block does not create a retain cycle.
    uint64_t v = ++self.frameValue;
    [self.commandQueue signalEvent:self.frameEvent value:v];
    dispatch_semaphore_t sema = self.inFlightSemaphore;
    [self.frameEvent notifyListener:self.frameListener atValue:v block:^(id<MTLSharedEvent>, uint64_t){
        dispatch_semaphore_signal(sema);
    }];
}

-(void)shutdown
{
#if TARGET_OS_OSX
    if (_eventMonitor)
    {
        [NSEvent removeMonitor:_eventMonitor];
        _eventMonitor = nil;
    }
    if (_displayLink)
    {
        CVDisplayLinkStop(_displayLink);
        CVDisplayLinkRelease(_displayLink);
        _displayLink = NULL;
    }
#else
    [self.metalView stopDisplayLink];
#endif

    ImGui_ImplMetal4_Shutdown();
#if TARGET_OS_OSX
    ImGui_ImplOSX_Shutdown();
#endif
    ImGui::DestroyContext();
}

-(void)dealloc
{
    [self shutdown];
}

//-----------------------------------------------------------------------------------
// Input processing
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

-(void)viewWillAppear
{
    [super viewWillAppear];
    self.view.window.delegate = self;
    // Deliver mouse-moved events so the input monitor can keep rendering active while hovering.
    self.view.window.acceptsMouseMovedEvents = YES;
}

-(void)windowWillClose:(NSNotification *)notification
{
    // Closing the main window terminates the app (matching the GLFW example), so any torn-off
    // viewport windows are torn down with it instead of being left behind as orphan windows.
    [NSApp terminate:nil];
}

#else

// This touch mapping is super cheesy/hacky. We treat any touch on the screen
// as if it were a depressed left mouse button, and we don't bother handling
// multitouch correctly at all. This causes the "cursor" to behave very erratically
// when there are multiple active touches. But for demo purposes, single-touch
// interaction actually works surprisingly well.
-(void)updateIOWithTouchEvent:(UIEvent *)event
{
    UITouch *anyTouch = event.allTouches.anyObject;
    CGPoint touchLocation = [anyTouch locationInView:self.view];
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    io.AddMousePosEvent(touchLocation.x, touchLocation.y);

    BOOL hasActiveTouch = NO;
    for (UITouch *touch in event.allTouches)
    {
        if (touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled)
        {
            hasActiveTouch = YES;
            break;
        }
    }
    io.AddMouseButtonEvent(0, hasActiveTouch);
}

-(void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event      { [self updateIOWithTouchEvent:event]; }
-(void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event      { [self updateIOWithTouchEvent:event]; }
-(void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event  { [self updateIOWithTouchEvent:event]; }
-(void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event      { [self updateIOWithTouchEvent:event]; }

#endif

@end

//-----------------------------------------------------------------------------------
// AppDelegate
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@end

@implementation AppDelegate

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

-(instancetype)init
{
    if (self = [super init])
    {
        NSViewController *rootViewController = [[AppViewController alloc] initWithNibName:nil bundle:nil];
        self.window = [[NSWindow alloc] initWithContentRect:NSZeroRect
                                                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
        self.window.contentViewController = rootViewController;
        [self.window center];
        [self.window makeKeyAndOrderFront:self];
    }
    return self;
}

@end

#else

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation AppDelegate

-(BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey,id> *)launchOptions
{
    UIViewController *rootViewController = [[AppViewController alloc] init];
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController = rootViewController;
    [self.window makeKeyAndVisible];
    return YES;
}

@end

#endif

//-----------------------------------------------------------------------------------
// Application main() function
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

int main(int, const char**)
{
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        AppDelegate *appDelegate = [[AppDelegate alloc] init];   // creates window
        [NSApp setDelegate:appDelegate];

        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}

#else

int main(int argc, char * argv[])
{
    @autoreleasepool
    {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

#endif

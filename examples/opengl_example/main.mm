#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#include "../../imgui.h"

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
static float highDpiScale = 1.0;
static bool g_mousePressed[2] = { false, false };
static float g_mouseCoords[2] {0,0};
static clock_t g_lastClock;
static unsigned int g_windowWidth, g_windowHeight;
static unsigned int g_backingWidth, g_backingHeight;

void ImImpl_RenderDrawLists(ImDrawData* draw_data)
{
    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Render command lists
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->VtxBuffer.front();
        const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((GLint)(pcmd->ClipRect.x*highDpiScale),
                    (GLint)((height - pcmd->ClipRect.w)*highDpiScale),
                    (GLint)((pcmd->ClipRect.z - pcmd->ClipRect.x)*highDpiScale),
                    (GLint)((pcmd->ClipRect.w - pcmd->ClipRect.y)*highDpiScale));

                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
#undef OFFSETOF

    // Restore modified state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void LoadFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)tex_id;
}

void IMGUIExample_InitImGui()
{
    ImGuiIO& io = ImGui::GetIO();
    // Time elapsed since last frame, in seconds
    // (in this sample app we'll override this every frame because our time step is variable)
    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab] = 9;
    io.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
    io.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
    io.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
    io.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
    io.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
    io.KeyMap[ImGuiKey_End] = ImGuiKey_End;
    io.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
    io.KeyMap[ImGuiKey_Backspace] = 127;
    io.KeyMap[ImGuiKey_Enter] = 13;
    io.KeyMap[ImGuiKey_Escape] = 27;
    io.KeyMap[ImGuiKey_A] = 'a';
    io.KeyMap[ImGuiKey_C] = 'c';
    io.KeyMap[ImGuiKey_V] = 'v';
    io.KeyMap[ImGuiKey_X] = 'x';
    io.KeyMap[ImGuiKey_Y] = 'y';
    io.KeyMap[ImGuiKey_Z] = 'z';
    
    io.DeltaTime = 1.0f/60.0f;
    
    io.RenderDrawListsFn = ImImpl_RenderDrawLists;
    
    LoadFontsTexture();
}

void IMGUIExample_Draw(double elapsedMilliseconds)
{
    ImGuiIO& io = ImGui::GetIO();
    // Setup resolution (every frame to accommodate for window resizing)
    int w,h;
    int display_w, display_h;
    display_w = g_backingWidth;
    display_h = g_backingHeight;
    w = g_windowWidth;
    h = g_windowHeight;
    highDpiScale = g_backingWidth / g_windowWidth;
    // Display size, in pixels. For clamping windows positions.
    io.DisplaySize = ImVec2((float)g_windowWidth, (float)g_windowHeight);
    
    io.DeltaTime = elapsedMilliseconds/100.0; //convert in seconds

    // Setup inputs
    double mouse_x = 0, mouse_y = 0;
    mouse_x = g_mouseCoords[0];
    mouse_y = g_mouseCoords[1];
    // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
    io.MouseDown[0] = g_mousePressed[0];

    // If a mouse press event came, always pass it as "mouse held this frame",
    // so we don't miss click-release events that are shorter than 1 frame.

    io.MouseDown[1] = g_mousePressed[1];

    // Start the frame
    ImGui::NewFrame();
    static bool show_test_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_col = ImColor(114, 144, 154);

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    {
        static float f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_col);
        if (ImGui::Button("Test Window")) show_test_window ^= 1;
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Mouse X=%.1f, Y=%.1f", mouse_x, mouse_y);
    }

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window);
    }

    // Rendering
    GLsizei width  = (GLsizei)(io.DisplaySize.x * highDpiScale);
    GLsizei height = (GLsizei)(io.DisplaySize.y * highDpiScale);
    glViewport(0, 0, width, height);
    glClearColor(clear_col.x, clear_col.y, clear_col.z, clear_col.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
}

//------------------------------------------------------------------
// IMGUIExampleView
//------------------------------------------------------------------

@interface IMGUIExampleView : NSOpenGLView
{
    NSTimer *animationTimer;
}
@end

@implementation IMGUIExampleView

-(void)animationTimerFired:(NSTimer*)timer
{
    [self setNeedsDisplay:YES];
}

-(id)initWithFrame:(NSRect)frameRect pixelFormat:(NSOpenGLPixelFormat *)format
{
    self = [super initWithFrame:frameRect pixelFormat:format];
    if (self)
    {
        g_lastClock = clock();
    }
    return(self);
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];
    
#ifndef DEBUG
    GLint swapInterval = 1;
    [[self openGLContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
    if (swapInterval == 0)
    {
        NSLog(@"Error: Cannot set swap interval.");
    }
#endif
}

- (void)drawView
{
    NSWindow *mainWindow = [self window];
    NSPoint mousePosition = [mainWindow mouseLocationOutsideOfEventStream];

    //convert to View
    mousePosition = [self convertPoint:mousePosition fromView:nil];

    g_mouseCoords[0] = mousePosition.x;
    g_mouseCoords[1] = mousePosition.y - 1.0f;

    clock_t thisclock = clock();
    unsigned long clock_delay = thisclock - g_lastClock;
    double milliseconds = clock_delay * 1000.0f / CLOCKS_PER_SEC;

    IMGUIExample_Draw(milliseconds);

    g_lastClock = thisclock;

    [[self openGLContext] flushBuffer];
    
    if (!animationTimer)
    {
        animationTimer = [[NSTimer scheduledTimerWithTimeInterval:0.017 target:self selector:@selector(animationTimerFired:) userInfo:nil repeats:YES] retain];
    }
}

-(void)setViewportRect:(NSRect)bounds
{
    glViewport(0, 0, bounds.size.width, bounds.size.height);
    g_windowWidth = bounds.size.width;
	g_windowHeight = bounds.size.height;
    
    if (g_windowHeight == 0)
    {
        g_windowHeight = 1;
	}
    

    ImGui::GetIO().DisplaySize = ImVec2((float)bounds.size.width, (float)bounds.size.height);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6)
    {
        NSRect backing = [self convertRectToBacking:bounds];
        g_backingWidth = backing.size.width;
        g_backingHeight= backing.size.height;
        if (g_backingHeight == 0)
        {
            g_backingHeight = g_windowHeight * 2;
        }
    }
    else
#endif /*MAC_OS_X_VERSION_MAX_ALLOWED*/
    {
        g_backingWidth = g_windowWidth;
        g_backingHeight= g_windowHeight;
    }
}

-(void)reshape
{
    [self setViewportRect:self.bounds];
    [[self openGLContext] update];
    [self drawView];
}

-(void)drawRect:(NSRect)bounds
{
    [self drawView];
}

#pragma mark -

-(BOOL)acceptsFirstResponder
{
    return(YES);
}

-(BOOL)becomeFirstResponder
{
    return(YES);
}

-(BOOL)resignFirstResponder
{
    return(YES);
}

// Flips coordinate system upside down on Y
-(BOOL)isFlipped
{
    return(YES);
}

#pragma mark Mouse and Key Events.

static bool mapKeymap(int* keymap)
{
    if(*keymap == NSUpArrowFunctionKey)
        *keymap = ImGuiKey_LeftArrow;
    else if(*keymap == NSDownArrowFunctionKey)
        *keymap = ImGuiKey_DownArrow;
    else if(*keymap == NSLeftArrowFunctionKey)
        *keymap = ImGuiKey_LeftArrow;
    else if(*keymap == NSRightArrowFunctionKey)
        *keymap = ImGuiKey_RightArrow;
    else if(*keymap == NSHomeFunctionKey)
        *keymap = ImGuiKey_Home;
    else if(*keymap == NSEndFunctionKey)
        *keymap = ImGuiKey_End;
    else if(*keymap == NSDeleteFunctionKey)
        *keymap = ImGuiKey_Delete;
    else if(*keymap == 25) // SHIFT + TAB
        *keymap = 9; // TAB
    else
        return true;
    
    return false;
}

static void resetKeys()
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[io.KeyMap[ImGuiKey_A]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_C]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_V]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_X]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Y]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Z]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_LeftArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_RightArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Tab]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_UpArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_DownArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Tab]] = false;
}

-(void)keyUp:(NSEvent *)theEvent
{
    NSString *str = [theEvent characters];
    ImGuiIO& io = ImGui::GetIO();
    int len = (int)[str length];
    for(int i = 0; i < len; i++)
    {
        int keymap = [str characterAtIndex:i];
        mapKeymap(&keymap);
        if(keymap < 512)
        {
            io.KeysDown[keymap] = false;
        }
    }
}

-(void)keyDown:(NSEvent *)theEvent
{
    NSString *str = [theEvent characters];
    ImGuiIO& io = ImGui::GetIO();
    int len = (int)[str length];
    for(int i = 0; i < len; i++)
    {
        int keymap = [str characterAtIndex:i];
        if(mapKeymap(&keymap) && !io.KeyCtrl)
            io.AddInputCharacter(keymap);
        if(keymap < 512)
        {
            if(io.KeyCtrl)
            {
                // we must reset in case we're pressing a sequence
                // of special keys while keeping the command pressed
                resetKeys();
            }
            io.KeysDown[keymap] = true;
        }
    }
}

- (void)flagsChanged:(NSEvent *)event
{
    unsigned int flags;
    flags = [event modifierFlags] & NSDeviceIndependentModifierFlagsMask;
    ImGuiIO& io = ImGui::GetIO();
    bool wasKeyShift= io.KeyShift;
    bool wasKeyCtrl = io.KeyCtrl;
    io.KeyShift     = flags & NSShiftKeyMask;
    io.KeyCtrl      = flags & NSCommandKeyMask;
    bool keyShiftReleased = wasKeyShift && !io.KeyShift;
    bool keyCtrlReleased  = wasKeyCtrl  && !io.KeyCtrl;
    if(keyShiftReleased || keyCtrlReleased)
    {
        // we must reset them as we will not receive any
        // keyUp event if they where pressed during shift or command
        resetKeys();
    }
}

-(void)mouseDown:(NSEvent *)theEvent
{
    int button = (int)[theEvent buttonNumber];
    g_mousePressed[button] = true;
}

-(void)mouseUp:(NSEvent *)theEvent
{
    int button = (int)[theEvent buttonNumber];
    g_mousePressed[button] = false;
}

- (void)scrollWheel:(NSEvent *)event
{
    double deltaX, deltaY;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6)
    {
        deltaX = [event scrollingDeltaX];
        deltaY = [event scrollingDeltaY];

        if ([event hasPreciseScrollingDeltas])
        {
            deltaX *= 0.1;
            deltaY *= 0.1;
        }
    }
    else
#endif /*MAC_OS_X_VERSION_MAX_ALLOWED*/
    {
        deltaX = [event deltaX];
        deltaY = [event deltaY];
    }

    if (fabs(deltaX) > 0.0 || fabs(deltaY) > 0.0)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel += deltaY * 0.1f;
    }
}

-(void)dealloc
{
    [animationTimer release];
    [super dealloc];
}

@end

//------------------------------------------------------------------
// IMGUIExampleAppDelegate
//------------------------------------------------------------------
@interface IMGUIExampleAppDelegate : NSObject <NSApplicationDelegate>

@property (nonatomic, readonly) NSWindow *window;

@end

@implementation IMGUIExampleAppDelegate

@synthesize window = _window;

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

- (NSWindow*)window
{
    if (_window != nil)
        return(_window);
    
    NSRect viewRect = NSMakeRect(100.0, 100.0, 1300.0, 800.0);
    
    _window = [[NSWindow alloc] initWithContentRect:viewRect styleMask:NSTitledWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask|NSClosableWindowMask backing:NSBackingStoreBuffered defer:YES];
    [_window setTitle:@"IMGUI OSX Sample"];
    [_window setOpaque:YES];
    
    [_window makeKeyAndOrderFront:NSApp];
    
    return(_window);
}

- (void)setupMenu
{
    NSMenu *mainMenuBar;
    NSMenu *appMenu;
    NSMenuItem *menuItem;
    
    mainMenuBar = [[NSMenu alloc] init];
    
    appMenu = [[NSMenu alloc] initWithTitle:@"IMGUI OSX Sample"];
    menuItem = [appMenu addItemWithTitle:@"Quit IMGUI OSX Sample" action:@selector(terminate:) keyEquivalent:@"q"];
    [menuItem setKeyEquivalentModifierMask:NSCommandKeyMask];
    
    menuItem = [[NSMenuItem alloc] init];
    [menuItem setSubmenu:appMenu];
    
    [mainMenuBar addItem:menuItem];
    
    [appMenu release];
    [NSApp setMainMenu:mainMenuBar];
}

- (void)dealloc
{
    [_window dealloc];
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [self setupMenu];
    
    NSOpenGLPixelFormatAttribute attrs[] =
    {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 32,
        0
    };
    
    NSOpenGLPixelFormat *format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    IMGUIExampleView *view = [[IMGUIExampleView alloc] initWithFrame:self.window.frame pixelFormat:format];
    [format release];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6)
        [view setWantsBestResolutionOpenGLSurface:YES];
#endif /*MAC_OS_X_VERSION_MAX_ALLOWED*/

    [self.window setContentView:view];
    
    if ([view openGLContext] == nil)
    {
        NSLog(@"No OpenGL Context!");
    }
    IMGUIExample_InitImGui();
}

@end

//------------------------------------------------------------------
// main
//------------------------------------------------------------------

int main(int argc, const char * argv[])
{
    [[NSAutoreleasePool alloc] init];
    NSApp = [NSApplication sharedApplication];
    
    IMGUIExampleAppDelegate *delegate = [[IMGUIExampleAppDelegate alloc] init];
    
    [[NSApplication sharedApplication] setDelegate:delegate];
    [NSApp run];
    return NSApplicationMain(argc, argv);
}

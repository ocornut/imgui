// ImGui iOS+OpenGL+Synergy binding
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// Providing a standalone iOS application with Synergy integration makes this sample more verbose than others. It also hasn't been tested as much.
// Refer to other examples to get an easier understanding of how to integrate ImGui into your existing application.

// TODO:
// - Clipboard is not supported.

#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "imgui_impl_ios.h"
#include "imgui.h"

#include "uSynergy.h"

// From Carbon HIToolbox/Events.h
// FIXME: Keyboard mapping is hacked in because Synergy doesn't give us character but only keycode which aren't really portable if you consider keyboard locale. See https://github.com/ocornut/imgui/pull/247
enum {
    kVK_ANSI_A                    = 0x00,
    kVK_ANSI_S                    = 0x01,
    kVK_ANSI_D                    = 0x02,
    kVK_ANSI_F                    = 0x03,
    kVK_ANSI_H                    = 0x04,
    kVK_ANSI_G                    = 0x05,
    kVK_ANSI_Z                    = 0x06,
    kVK_ANSI_X                    = 0x07,
    kVK_ANSI_C                    = 0x08,
    kVK_ANSI_V                    = 0x09,
    kVK_ANSI_B                    = 0x0B,
    kVK_ANSI_Q                    = 0x0C,
    kVK_ANSI_W                    = 0x0D,
    kVK_ANSI_E                    = 0x0E,
    kVK_ANSI_R                    = 0x0F,
    kVK_ANSI_Y                    = 0x10,
    kVK_ANSI_T                    = 0x11,
    kVK_ANSI_1                    = 0x12,
    kVK_ANSI_2                    = 0x13,
    kVK_ANSI_3                    = 0x14,
    kVK_ANSI_4                    = 0x15,
    kVK_ANSI_6                    = 0x16,
    kVK_ANSI_5                    = 0x17,
    kVK_ANSI_Equal                = 0x18,
    kVK_ANSI_9                    = 0x19,
    kVK_ANSI_7                    = 0x1A,
    kVK_ANSI_Minus                = 0x1B,
    kVK_ANSI_8                    = 0x1C,
    kVK_ANSI_0                    = 0x1D,
    kVK_ANSI_RightBracket         = 0x1E,
    kVK_ANSI_O                    = 0x1F,
    kVK_ANSI_U                    = 0x20,
    kVK_ANSI_LeftBracket          = 0x21,
    kVK_ANSI_I                    = 0x22,
    kVK_ANSI_P                    = 0x23,
    kVK_ANSI_L                    = 0x25,
    kVK_ANSI_J                    = 0x26,
    kVK_ANSI_Quote                = 0x27,
    kVK_ANSI_K                    = 0x28,
    kVK_ANSI_Semicolon            = 0x29,
    kVK_ANSI_Backslash            = 0x2A,
    kVK_ANSI_Comma                = 0x2B,
    kVK_ANSI_Slash                = 0x2C,
    kVK_ANSI_N                    = 0x2D,
    kVK_ANSI_M                    = 0x2E,
    kVK_ANSI_Period               = 0x2F,
    kVK_ANSI_Grave                = 0x32,
    kVK_ANSI_KeypadDecimal        = 0x41,
    kVK_ANSI_KeypadMultiply       = 0x43,
    kVK_ANSI_KeypadPlus           = 0x45,
    kVK_ANSI_KeypadClear          = 0x47,
    kVK_ANSI_KeypadDivide         = 0x4B,
    kVK_ANSI_KeypadEnter          = 0x4C,
    kVK_ANSI_KeypadMinus          = 0x4E,
    kVK_ANSI_KeypadEquals         = 0x51,
    kVK_ANSI_Keypad0              = 0x52,
    kVK_ANSI_Keypad1              = 0x53,
    kVK_ANSI_Keypad2              = 0x54,
    kVK_ANSI_Keypad3              = 0x55,
    kVK_ANSI_Keypad4              = 0x56,
    kVK_ANSI_Keypad5              = 0x57,
    kVK_ANSI_Keypad6              = 0x58,
    kVK_ANSI_Keypad7              = 0x59,
    kVK_ANSI_Keypad8              = 0x5B,
    kVK_ANSI_Keypad9              = 0x5C
};

/* keycodes for keys that are independent of keyboard layout*/
enum {
    kVK_Return                    = 0x24,
    kVK_Tab                       = 0x30,
    kVK_Space                     = 0x31,
    kVK_Delete                    = 0x33,
    kVK_Escape                    = 0x35,
    kVK_Command                   = 0x37,
    kVK_Shift                     = 0x38,
    kVK_CapsLock                  = 0x39,
    kVK_Option                    = 0x3A,
    kVK_Control                   = 0x3B,
    kVK_RightShift                = 0x3C,
    kVK_RightOption               = 0x3D,
    kVK_RightControl              = 0x3E,
    kVK_Function                  = 0x3F,
    kVK_F17                       = 0x40,
    kVK_VolumeUp                  = 0x48,
    kVK_VolumeDown                = 0x49,
    kVK_Mute                      = 0x4A,
    kVK_F18                       = 0x4F,
    kVK_F19                       = 0x50,
    kVK_F20                       = 0x5A,
    kVK_F5                        = 0x60,
    kVK_F6                        = 0x61,
    kVK_F7                        = 0x62,
    kVK_F3                        = 0x63,
    kVK_F8                        = 0x64,
    kVK_F9                        = 0x65,
    kVK_F11                       = 0x67,
    kVK_F13                       = 0x69,
    kVK_F16                       = 0x6A,
    kVK_F14                       = 0x6B,
    kVK_F10                       = 0x6D,
    kVK_F12                       = 0x6F,
    kVK_F15                       = 0x71,
    kVK_Help                      = 0x72,
    kVK_Home                      = 0x73,
    kVK_PageUp                    = 0x74,
    kVK_ForwardDelete             = 0x75,
    kVK_F4                        = 0x76,
    kVK_End                       = 0x77,
    kVK_F2                        = 0x78,
    kVK_PageDown                  = 0x79,
    kVK_F1                        = 0x7A,
    kVK_LeftArrow                 = 0x7B,
    kVK_RightArrow                = 0x7C,
    kVK_DownArrow                 = 0x7D,
    kVK_UpArrow                   = 0x7E
};

static char g_keycodeCharUnshifted[256] = {};
static char g_keycodeCharShifted[256] = {};

//static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_mouseWheelX = 0.0f;
static float        g_mouseWheelY = 0.0f;

static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static size_t       g_VboSize = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0;
static float        g_displayScale;

static int usynergy_sockfd;
static bool g_synergyPtrActive = false;
static uint16_t g_mousePosX = 0;
static uint16_t g_mousePosY = 0;

static void ImGui_ImplIOS_RenderDrawLists (ImDrawData *draw_data);
bool ImGui_ImplIOS_CreateDeviceObjects();

static NSString *g_serverName;

uSynergyBool ImGui_ConnectFunc(uSynergyCookie cookie)
{
    // NOTE: You need to turn off "Use SSL Encryption" in Synergy preferences, since
    // uSynergy does not support SSL.
    
    NSLog( @"Connect Func!");
    struct addrinfo hints, *res;
    
    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    
    // get server address
    getaddrinfo([g_serverName UTF8String], "24800", &hints, &res);
    
    if (!res)
    {
        NSLog( @"Could not find server: %@", g_serverName );
        return USYNERGY_FALSE;
    }
    
    // make a socket:
    usynergy_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    // connect it to the address and port we passed in to getaddrinfo():
    int ret = connect(usynergy_sockfd, res->ai_addr, res->ai_addrlen);
    if (!ret) {
        NSLog( @"Connect succeeded...");
    } else {
        NSLog( @"Connect failed, %d", ret );
    }
    
    
    return USYNERGY_TRUE;
}

uSynergyBool ImGui_SendFunc(uSynergyCookie cookie, const uint8_t *buffer, int length)
{
//    NSLog( @"Send Func" );
    send( usynergy_sockfd, buffer, length, 0 );
    
    return USYNERGY_TRUE;
}

uSynergyBool ImGui_RecvFunc(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength)
{
    *outLength = (int)recv( usynergy_sockfd, buffer, maxLength, 0 );
    
    return USYNERGY_TRUE;
}

void ImGui_SleepFunc(uSynergyCookie cookie, int timeMs)
{
    usleep( timeMs * 1000 );
}

uint32_t ImGui_GetTimeFunc()
{
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    
    return (int32_t)((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
}

void ImGui_TraceFunc(uSynergyCookie cookie, const char *text)
{
    puts(text);
}

void ImGui_ScreenActiveCallback(uSynergyCookie cookie, uSynergyBool active)
{
    g_synergyPtrActive = active;
//    printf( "Synergy: screen activate %s\n", active?"YES":"NO" );
}

void ImGui_MouseCallback(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX, int16_t wheelY,
                         uSynergyBool buttonLeft, uSynergyBool buttonRight, uSynergyBool buttonMiddle)
{
//    printf("Synergy: mouse callback %d %d -- wheel %d %d\n", x, y,  wheelX, wheelY );
    uSynergyContext *ctx = (uSynergyContext*)cookie;
    g_mousePosX = x;
    g_mousePosY = y;
    g_mouseWheelX = wheelX;
    g_mouseWheelY = wheelY;
    g_MousePressed[0] = buttonLeft;
    g_MousePressed[1] = buttonMiddle;
    g_MousePressed[2] = buttonRight;
    
    ctx->m_mouseWheelX = 0;
    ctx->m_mouseWheelY = 0;
}

void ImGui_KeyboardCallback(uSynergyCookie cookie, uint16_t key,
                            uint16_t modifiers, uSynergyBool down, uSynergyBool repeat)
{
    int scanCode = key-1;
//    printf("Synergy: keyboard callback: 0x%02X (%s)", scanCode, down?"true":"false");
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[key] = down;
    io.KeyShift = (modifiers & USYNERGY_MODIFIER_SHIFT);
    io.KeyCtrl = (modifiers & USYNERGY_MODIFIER_CTRL);
    io.KeyAlt = (modifiers & USYNERGY_MODIFIER_ALT);
    io.KeySuper = (modifiers & USYNERGY_MODIFIER_WIN);
    
    // Add this as keyboard input
    if ((down) && (key) && (scanCode<256) && !(modifiers & USYNERGY_MODIFIER_CTRL)) 
	{
        // If this key maps to a character input, apply it
        int charForKeycode = (modifiers & USYNERGY_MODIFIER_SHIFT) ? g_keycodeCharShifted[scanCode] : g_keycodeCharUnshifted[scanCode];
        io.AddInputCharacter((unsigned short)charForKeycode);
    }
    
}

void ImGui_JoystickCallback(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons, int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY)
{
    printf("Synergy: joystick callback TODO\n");
}

void ImGui_ClipboardCallback(uSynergyCookie cookie, enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size)
{
    printf("Synergy: clipboard callback TODO\n" );
}

@interface ImGuiHelper ()
{
    BOOL _mouseDown;
    BOOL _mouseTapped;
    CGPoint _touchPos;

    uSynergyContext _synergyCtx;
    dispatch_queue_t _synergyQueue;
}
@property (nonatomic, weak) UIView *view;
@property (nonatomic, strong) NSString *serverName;

@end

@implementation ImGuiHelper

- (id) initWithView: (UIView *)view
{
    self = [super init];
    if (self)
    {
        self.view = view;

        [self setupImGuiHooks];
    }
    return self;
}

- (void)setupKeymaps
{
    // The keyboard mapping is a big headache. I tried for a while to find a better way to do this,
    // but this was the best I could come up with. There are some device independent API's available
    // to convert scan codes to unicode characters, but these are only available on mac and not
    // on iOS as far as I can tell (it's part of Carbon). I didn't see any better way to do
    // this or  any way to get the character codes out of usynergy.
    g_keycodeCharUnshifted[ kVK_ANSI_A ]='a';
    g_keycodeCharUnshifted[ kVK_ANSI_S ]='s';
    g_keycodeCharUnshifted[ kVK_ANSI_D ]='d';
    g_keycodeCharUnshifted[ kVK_ANSI_F ]='f';
    g_keycodeCharUnshifted[ kVK_ANSI_H ]='h';
    g_keycodeCharUnshifted[ kVK_ANSI_G ]='g';
    g_keycodeCharUnshifted[ kVK_ANSI_Z ]='z';
    g_keycodeCharUnshifted[ kVK_ANSI_X ]='x';
    g_keycodeCharUnshifted[ kVK_ANSI_C ]='c';
    g_keycodeCharUnshifted[ kVK_ANSI_V ]='v';
    g_keycodeCharUnshifted[ kVK_ANSI_B ]='b';
    g_keycodeCharUnshifted[ kVK_ANSI_Q ]='q';
    g_keycodeCharUnshifted[ kVK_ANSI_W ]='w';
    g_keycodeCharUnshifted[ kVK_ANSI_E ]='e';
    g_keycodeCharUnshifted[ kVK_ANSI_R ]='r';
    g_keycodeCharUnshifted[ kVK_ANSI_Y ]='y';
    g_keycodeCharUnshifted[ kVK_ANSI_T ]='t';
    g_keycodeCharUnshifted[ kVK_ANSI_1 ]='1';
    g_keycodeCharUnshifted[ kVK_ANSI_2 ]='2';
    g_keycodeCharUnshifted[ kVK_ANSI_3 ]='3';
    g_keycodeCharUnshifted[ kVK_ANSI_4 ]='4';
    g_keycodeCharUnshifted[ kVK_ANSI_6 ]='6';
    g_keycodeCharUnshifted[ kVK_ANSI_5 ]='5';
    g_keycodeCharUnshifted[ kVK_ANSI_Equal ]='=';
    g_keycodeCharUnshifted[ kVK_ANSI_9 ]='9';
    g_keycodeCharUnshifted[ kVK_ANSI_7 ]='7';
    g_keycodeCharUnshifted[ kVK_ANSI_Minus ]='-';
    g_keycodeCharUnshifted[ kVK_ANSI_8 ]='8';
    g_keycodeCharUnshifted[ kVK_ANSI_0 ]='0';
    g_keycodeCharUnshifted[ kVK_ANSI_RightBracket ]=']';
    g_keycodeCharUnshifted[ kVK_ANSI_O ]='o';
    g_keycodeCharUnshifted[ kVK_ANSI_U ]='u';
    g_keycodeCharUnshifted[ kVK_ANSI_LeftBracket ]='[';
    g_keycodeCharUnshifted[ kVK_ANSI_I ]='i';
    g_keycodeCharUnshifted[ kVK_ANSI_P ]='p';
    g_keycodeCharUnshifted[ kVK_ANSI_L ]='l';
    g_keycodeCharUnshifted[ kVK_ANSI_J ]='j';
    g_keycodeCharUnshifted[ kVK_ANSI_Quote ]='\'';
    g_keycodeCharUnshifted[ kVK_ANSI_K ]='k';
    g_keycodeCharUnshifted[ kVK_ANSI_Semicolon ]=';';
    g_keycodeCharUnshifted[ kVK_ANSI_Backslash ]='\\';
    g_keycodeCharUnshifted[ kVK_ANSI_Comma ]=',';
    g_keycodeCharUnshifted[ kVK_ANSI_Slash ]='/';
    g_keycodeCharUnshifted[ kVK_ANSI_N ]='n';
    g_keycodeCharUnshifted[ kVK_ANSI_M ]='m';
    g_keycodeCharUnshifted[ kVK_ANSI_Period ]='.';
    g_keycodeCharUnshifted[ kVK_ANSI_Grave ]='`';
    g_keycodeCharUnshifted[ kVK_ANSI_KeypadDecimal ]='.';
    g_keycodeCharUnshifted[ kVK_ANSI_KeypadMultiply ]='*';
    g_keycodeCharUnshifted[ kVK_ANSI_KeypadPlus ]='+';
    g_keycodeCharUnshifted[ kVK_ANSI_KeypadDivide ]='/';
    g_keycodeCharUnshifted[ kVK_ANSI_KeypadEnter ]='\n';
    g_keycodeCharUnshifted[ kVK_ANSI_KeypadMinus ]='-';
    g_keycodeCharUnshifted[ kVK_ANSI_KeypadEquals ]='=';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad0 ]='0';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad1 ]='1';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad2 ]='2';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad3 ]='3';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad4 ]='4';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad5 ]='5';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad6 ]='6';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad7 ]='7';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad8 ]='8';
    g_keycodeCharUnshifted[ kVK_ANSI_Keypad9 ]='9';
    g_keycodeCharUnshifted[ kVK_Space ]=' ';
    
    g_keycodeCharShifted[ kVK_ANSI_A ]='A';
    g_keycodeCharShifted[ kVK_ANSI_S ]='S';
    g_keycodeCharShifted[ kVK_ANSI_D ]='D';
    g_keycodeCharShifted[ kVK_ANSI_F ]='F';
    g_keycodeCharShifted[ kVK_ANSI_H ]='H';
    g_keycodeCharShifted[ kVK_ANSI_G ]='G';
    g_keycodeCharShifted[ kVK_ANSI_Z ]='Z';
    g_keycodeCharShifted[ kVK_ANSI_X ]='X';
    g_keycodeCharShifted[ kVK_ANSI_C ]='C';
    g_keycodeCharShifted[ kVK_ANSI_V ]='V';
    g_keycodeCharShifted[ kVK_ANSI_B ]='B';
    g_keycodeCharShifted[ kVK_ANSI_Q ]='Q';
    g_keycodeCharShifted[ kVK_ANSI_W ]='W';
    g_keycodeCharShifted[ kVK_ANSI_E ]='E';
    g_keycodeCharShifted[ kVK_ANSI_R ]='R';
    g_keycodeCharShifted[ kVK_ANSI_Y ]='Y';
    g_keycodeCharShifted[ kVK_ANSI_T ]='T';
    g_keycodeCharShifted[ kVK_ANSI_1 ]='!';
    g_keycodeCharShifted[ kVK_ANSI_2 ]='@';
    g_keycodeCharShifted[ kVK_ANSI_3 ]='#';
    g_keycodeCharShifted[ kVK_ANSI_4 ]='$';
    g_keycodeCharShifted[ kVK_ANSI_6 ]='^';
    g_keycodeCharShifted[ kVK_ANSI_5 ]='%';
    g_keycodeCharShifted[ kVK_ANSI_Equal ]='+';
    g_keycodeCharShifted[ kVK_ANSI_9 ]='(';
    g_keycodeCharShifted[ kVK_ANSI_7 ]='&';
    g_keycodeCharShifted[ kVK_ANSI_Minus ]='_';
    g_keycodeCharShifted[ kVK_ANSI_8 ]='*';
    g_keycodeCharShifted[ kVK_ANSI_0 ]=')';
    g_keycodeCharShifted[ kVK_ANSI_RightBracket ]='}';
    g_keycodeCharShifted[ kVK_ANSI_O ]='O';
    g_keycodeCharShifted[ kVK_ANSI_U ]='U';
    g_keycodeCharShifted[ kVK_ANSI_LeftBracket ]='{';
    g_keycodeCharShifted[ kVK_ANSI_I ]='I';
    g_keycodeCharShifted[ kVK_ANSI_P ]='P';
    g_keycodeCharShifted[ kVK_ANSI_L ]='L';
    g_keycodeCharShifted[ kVK_ANSI_J ]='J';
    g_keycodeCharShifted[ kVK_ANSI_Quote ]='\"';
    g_keycodeCharShifted[ kVK_ANSI_K ]='K';
    g_keycodeCharShifted[ kVK_ANSI_Semicolon ]=':';
    g_keycodeCharShifted[ kVK_ANSI_Backslash ]='|';
    g_keycodeCharShifted[ kVK_ANSI_Comma ]='<';
    g_keycodeCharShifted[ kVK_ANSI_Slash ]='?';
    g_keycodeCharShifted[ kVK_ANSI_N ]='N';
    g_keycodeCharShifted[ kVK_ANSI_M ]='M';
    g_keycodeCharShifted[ kVK_ANSI_Period ]='>';
    g_keycodeCharShifted[ kVK_ANSI_Grave ]='~';
    g_keycodeCharShifted[ kVK_ANSI_KeypadDecimal ]='.';
    g_keycodeCharShifted[ kVK_ANSI_KeypadMultiply ]='*';
    g_keycodeCharShifted[ kVK_ANSI_KeypadPlus ]='+';
    g_keycodeCharShifted[ kVK_ANSI_KeypadDivide ]='/';
    g_keycodeCharShifted[ kVK_ANSI_KeypadEnter ]='\n';
    g_keycodeCharShifted[ kVK_ANSI_KeypadMinus ]='-';
    g_keycodeCharShifted[ kVK_ANSI_KeypadEquals ]='=';
    g_keycodeCharShifted[ kVK_ANSI_Keypad0 ]='0';
    g_keycodeCharShifted[ kVK_ANSI_Keypad1 ]='1';
    g_keycodeCharShifted[ kVK_ANSI_Keypad2 ]='2';
    g_keycodeCharShifted[ kVK_ANSI_Keypad3 ]='3';
    g_keycodeCharShifted[ kVK_ANSI_Keypad4 ]='4';
    g_keycodeCharShifted[ kVK_ANSI_Keypad5 ]='5';
    g_keycodeCharShifted[ kVK_ANSI_Keypad6 ]='6';
    g_keycodeCharShifted[ kVK_ANSI_Keypad7 ]='7';
    g_keycodeCharShifted[ kVK_ANSI_Keypad8 ]='8';
    g_keycodeCharShifted[ kVK_ANSI_Keypad9 ]='9';
    g_keycodeCharShifted[ kVK_Space ]=' ';
}

- (void)setupImGuiHooks
{
    ImGuiIO &io = ImGui::GetIO();
    
    [self setupKeymaps];
    
    // Account for retina display for glScissor
    g_displayScale = [[UIScreen mainScreen] scale];
    
    ImGuiStyle &style = ImGui::GetStyle();
    style.TouchExtraPadding = ImVec2( 4.0, 4.0 );
    
    io.RenderDrawListsFn = ImGui_ImplIOS_RenderDrawLists;
    
    UIPanGestureRecognizer *panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(viewDidPan:) ];
    [self.view addGestureRecognizer:panRecognizer];
    
    UITapGestureRecognizer *tapRecoginzer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector( viewDidTap:)];
    [self.view addGestureRecognizer:tapRecoginzer];
    
    // Fill out the Synergy key map
    // (for some reason synergy scan codes are off by 1)
    io.KeyMap[ImGuiKey_Tab] = kVK_Tab+1;
    io.KeyMap[ImGuiKey_LeftArrow] = kVK_LeftArrow+1;
    io.KeyMap[ImGuiKey_RightArrow] = kVK_RightArrow+1;
    io.KeyMap[ImGuiKey_UpArrow] = kVK_UpArrow+1;
    io.KeyMap[ImGuiKey_DownArrow] = kVK_DownArrow+1;
    io.KeyMap[ImGuiKey_Home] = kVK_Home+1;
    io.KeyMap[ImGuiKey_End] = kVK_End+1;
    io.KeyMap[ImGuiKey_Delete] = kVK_ForwardDelete+1;
    io.KeyMap[ImGuiKey_Backspace] = kVK_Delete+1;
    io.KeyMap[ImGuiKey_Enter] = kVK_Return+1;
    io.KeyMap[ImGuiKey_Escape] = kVK_Escape+1;
    io.KeyMap[ImGuiKey_A] = kVK_ANSI_A+1;
    io.KeyMap[ImGuiKey_C] = kVK_ANSI_C+1;
    io.KeyMap[ImGuiKey_V] = kVK_ANSI_V+1;
    io.KeyMap[ImGuiKey_X] = kVK_ANSI_X+1;
    io.KeyMap[ImGuiKey_Y] = kVK_ANSI_Y+1;
    io.KeyMap[ImGuiKey_Z] = kVK_ANSI_Z+1;
}

- (void)connectServer: (NSString*)serverName
{
    self.serverName = serverName;
    g_serverName = serverName;
    
    // Init synergy
    NSString *bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:(NSString*)kCFBundleNameKey];
    
    uSynergyInit( &_synergyCtx );
    _synergyCtx.m_clientName = strdup( [bundleName UTF8String] );
    _synergyCtx.m_clientWidth = self.view.bounds.size.width;
    _synergyCtx.m_clientHeight = self.view.bounds.size.height;
    
    _synergyCtx.m_connectFunc = ImGui_ConnectFunc;
    _synergyCtx.m_sendFunc = ImGui_SendFunc;
    _synergyCtx.m_receiveFunc = ImGui_RecvFunc;
    _synergyCtx.m_sleepFunc = ImGui_SleepFunc;
    _synergyCtx.m_traceFunc = ImGui_TraceFunc;
    _synergyCtx.m_getTimeFunc = ImGui_GetTimeFunc;
    
    _synergyCtx.m_traceFunc = ImGui_TraceFunc;
    _synergyCtx.m_screenActiveCallback = ImGui_ScreenActiveCallback;
    _synergyCtx.m_mouseCallback = ImGui_MouseCallback;
    _synergyCtx.m_keyboardCallback = ImGui_KeyboardCallback;
    
    _synergyCtx.m_cookie = (uSynergyCookie)&_synergyCtx;
    
    // Create a background thread for synergy
    _synergyQueue = dispatch_queue_create( "imgui-usynergy", NULL );
    dispatch_async( _synergyQueue, ^{
        while (1) {
            uSynergyUpdate( &_synergyCtx );
        }
    });
}


- (void)viewDidPan: (UIPanGestureRecognizer *)recognizer
{
    
    if ((recognizer.state == UIGestureRecognizerStateBegan) ||
        (recognizer.state == UIGestureRecognizerStateChanged))
    {
        _mouseDown = YES;
        _touchPos = [recognizer locationInView:self.view];
    }
    else
    {
        _mouseDown = NO;
        _touchPos = CGPointMake( -1, -1 );
    }
}

- (void)viewDidTap: (UITapGestureRecognizer*)recognizer
{
    _touchPos = [recognizer locationInView:self.view];
    _mouseTapped = YES;
}
    
- (void)render
{
    ImGui::Render();
}

- (void)newFrame
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle &style = ImGui::GetStyle();
    
    if (!g_FontTexture)
    {
        ImGui_ImplIOS_CreateDeviceObjects();
    }
    
    io.DisplaySize = ImVec2( _view.bounds.size.width, _view.bounds.size.height );

    io.MouseDrawCursor = g_synergyPtrActive;
    if (g_synergyPtrActive)
    {
        style.TouchExtraPadding = ImVec2( 0.0, 0.0 );
        io.MousePos = ImVec2( g_mousePosX, g_mousePosY );
        for (int i=0; i < 3; i++)
        {
            io.MouseDown[i] = g_MousePressed[i];
        }

        // This is an arbitrary scaling factor that works for me. Not sure what units these
        // mousewheel values from synergy are supposed to be in
        io.MouseWheel = g_mouseWheelY / 500.0;
    }
    else
    {
        // Synergy not active, use touch events
        style.TouchExtraPadding = ImVec2( 4.0, 4.0 );
        io.MousePos = ImVec2(_touchPos.x, _touchPos.y );
        if ((_mouseDown) || (_mouseTapped))
        {
            io.MouseDown[0] = true;
            _mouseTapped = NO;
        }
        else
        {
            io.MouseDown[0] = false;
        }
    }
    
    ImGui::NewFrame();
}
@end

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
// NOTE: this is copied pretty much entirely from the opengl3_example, with only minor changes for ES
static void ImGui_ImplIOS_RenderDrawLists (ImDrawData *draw_data)
{
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
	// FIXME: Backport changes from imgui_impl_glfw_gl3.cpp
    GLint last_program, last_texture;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    
    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f/width,	0.0f,			0.0f,		0.0f },
        { 0.0f,			2.0f/-height,	0.0f,		0.0f },
        { 0.0f,			0.0f,			-1.0f,		0.0f },
        { -1.0f,		1.0f,			0.0f,		1.0f },
    };
    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(g_VaoHandle);
    
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        ImDrawList* cmd_list = draw_data->CmdLists[n];
        ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
        
        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        const int needed_vtx_size = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        if (g_VboSize < needed_vtx_size)
        {
            // Grow our buffer if needed
            g_VboSize = needed_vtx_size + 2000 * sizeof(ImDrawVert);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)g_VboSize, NULL, GL_STREAM_DRAW);
        }

        unsigned char* vtx_data = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, needed_vtx_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        if (!vtx_data)
            continue;
        memcpy(vtx_data, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        glUnmapBuffer(GL_ARRAY_BUFFER);
        
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)(pcmd->ClipRect.x * g_displayScale),
                          (int)((height - pcmd->ClipRect.w) * g_displayScale),
                          (int)((pcmd->ClipRect.z - pcmd->ClipRect.x) * g_displayScale),
                          (int)((pcmd->ClipRect.w - pcmd->ClipRect.y) * g_displayScale));
                glDrawElements( GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer );
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
    
    // Restore modified state
    glBindVertexArray(0);
    glBindBuffer( GL_ARRAY_BUFFER, 0);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(last_program);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, last_texture);
}

void ImGui_ImplIOS_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.
    
    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);
}

bool ImGui_ImplIOS_CreateDeviceObjects()
{
    const GLchar *vertex_shader =
    "uniform mat4 ProjMtx;\n"
    "attribute highp vec2 Position;\n"
    "attribute highp vec2 UV;\n"
    "attribute highp vec4 Color;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "	Frag_UV = UV;\n"
    "	Frag_Color = Color;\n"
    "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";
    
    const GLchar* fragment_shader =
    "uniform sampler2D Texture;\n"
    "varying highp vec2 Frag_UV;\n"
    "varying highp vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "	gl_FragColor = Frag_Color * texture2D( Texture, Frag_UV.st);\n"
    "}\n";
    
    g_ShaderHandle = glCreateProgram();
    g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
    g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
    glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
    glCompileShader(g_VertHandle);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv( g_VertHandle, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(g_VertHandle, logLength, &logLength, log);
        NSLog(@"VERTEX Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glCompileShader(g_FragHandle);
    
#if defined(DEBUG)
    glGetShaderiv( g_FragHandle, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(g_FragHandle, logLength, &logLength, log);
        NSLog(@"FRAGMENT Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glAttachShader(g_ShaderHandle, g_VertHandle);
    glAttachShader(g_ShaderHandle, g_FragHandle);
    glLinkProgram(g_ShaderHandle);
    
    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");
    
    glGenBuffers(1, &g_VboHandle);
    
    glGenVertexArrays(1, &g_VaoHandle);
    glBindVertexArray(g_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glEnableVertexAttribArray(g_AttribLocationPosition);
    glEnableVertexAttribArray(g_AttribLocationUV);
    glEnableVertexAttribArray(g_AttribLocationColor);
    
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    ImGui_ImplIOS_CreateFontsTexture();
    
    return true;
}

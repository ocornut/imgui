//
//  imgui_impl_ios.cpp
//  imguiex
//
//  Joel Davis (joeld42@gmail.com)
//

#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>

#include "imgui_impl_ios.h"
#include "imgui.h"

static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static size_t       g_VboSize = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0;
static float        g_displayScale;

static void ImGui_ImplIOS_RenderDrawLists (ImDrawList** const cmd_lists, int cmd_lists_count);
bool ImGui_ImplIOS_CreateDeviceObjects();

@interface ImGuiHelper ()
{
    BOOL _mouseDown;
    BOOL _mouseTapped;
    CGPoint _touchPos;
}
@property (nonatomic, weak) UIView *view;
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

- (void)setupImGuiHooks
{
    ImGuiIO &io = ImGui::GetIO();
    
    // Account for retina display for glScissor
    g_displayScale = [[UIScreen mainScreen] scale];
    
    ImGuiStyle &style = ImGui::GetStyle();
    style.TouchExtraPadding = ImVec2( 8.0, 8.0 );
    
    io.RenderDrawListsFn = ImGui_ImplIOS_RenderDrawLists;
    
    UIPanGestureRecognizer *panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(viewDidPan:) ];
    [self.view addGestureRecognizer:panRecognizer];
    
    UITapGestureRecognizer *tapRecoginzer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector( viewDidTap:)];
    [self.view addGestureRecognizer:tapRecoginzer];
    
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
    
    if (!g_FontTexture)
    {
        ImGui_ImplIOS_CreateDeviceObjects();
    }
    
    io.DisplaySize = ImVec2( _view.bounds.size.width, _view.bounds.size.height );

    io.MousePos = ImVec2(_touchPos.x, _touchPos.y );
    if ((_mouseDown) || (_mouseTapped))
    {
        printf("Button0 down\n" );
        io.MouseDown[0] = true;
        _mouseTapped = NO;
    }
    else
    {
        printf("Button0 UP\n" );
        io.MouseDown[0] = false;
    }
    
    ImGui::NewFrame();
}
@end

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
static void ImGui_ImplIOS_RenderDrawLists (ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;
    
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
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
    
    // Grow our buffer according to what we need
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    size_t needed_vtx_size = total_vtx_count * sizeof(ImDrawVert);
    if (g_VboSize < needed_vtx_size)
    {
        g_VboSize = needed_vtx_size + 5000 * sizeof(ImDrawVert);  // Grow buffer
        glBufferData(GL_ARRAY_BUFFER, g_VboSize, NULL, GL_STREAM_DRAW);
    }
    
    // Copy and convert all vertices into a single contiguous buffer
    unsigned char* buffer_data = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, g_VboSize, GL_MAP_WRITE_BIT);
    if (!buffer_data)
        return;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        memcpy(buffer_data, &cmd_list->vtx_buffer[0], cmd_list->vtx_buffer.size() * sizeof(ImDrawVert));
        buffer_data += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(g_VaoHandle);
    
    int cmd_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        int vtx_offset = cmd_offset;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            if (pcmd->user_callback)
            {
                pcmd->user_callback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);
                glScissor((int)(pcmd->clip_rect.x * g_displayScale),
                          (int)((height - pcmd->clip_rect.w) * g_displayScale),
                          (int)((pcmd->clip_rect.z - pcmd->clip_rect.x) * g_displayScale),
                          (int)((pcmd->clip_rect.w - pcmd->clip_rect.y) * g_displayScale));
                glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            }
            vtx_offset += pcmd->vtx_count;
        }
        cmd_offset = vtx_offset;
    }
    
    // Restore modified state
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(last_program);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, last_texture);
}

void ImGui_ImplIOS_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.
    
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;
    
    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();
}

bool ImGui_ImplIOS_CreateDeviceObjects()
{
    const GLchar *vertex_shader =
//    "#version 330\n"
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
//    "#version 330\n"
    "uniform sampler2D Texture;\n"
    "varying highp vec2 Frag_UV;\n"
    "varying highp vec4 Frag_Color;\n"
//    "varying vec4 Out_Color;\n"
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


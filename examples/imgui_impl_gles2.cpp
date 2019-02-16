// dear imgui: Renderer for GLES2 (or WebGL, if run with Emscripten toolchain)
// This needs to be used along with a Platform Binding (e.g. SDL, emscripten..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-02-15: OpenGL: Initial version.

#include "imgui.h"
#include "imgui_impl_gles2.h"
#include <stdio.h>
#include <stdint.h>     // intptr_t

// OpenGL ES 2
#include <GLES2/gl2.h>

// OpenGL Data
static char g_GlslVersionString[32] = "#version 100\n";
static GLuint g_FontTexture = 0;
static GLuint g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_ElementsHandle = 0;

// Functions
bool ImGui_ImplGLES2_Init() {
  ImGuiIO& io = ImGui::GetIO();
  io.BackendRendererName = "imgui_impl_gles2";
  return true;
}

void ImGui_ImplGLES2_Shutdown() {
  ImGui_ImplGLES2_DestroyDeviceObjects();
}

void ImGui_ImplGLES2_NewFrame() {
  if ( !g_FontTexture ) {
    ImGui_ImplGLES2_CreateDeviceObjects();
  }
}

// GLES2 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so.
void ImGui_ImplGLES2_RenderDrawData(ImDrawData* draw_data) {
  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  int fb_width = (int) ( draw_data->DisplaySize.x * draw_data->FramebufferScale.x );
  int fb_height = (int) ( draw_data->DisplaySize.y * draw_data->FramebufferScale.y );
  if ( fb_width <= 0 || fb_height <= 0 ) {
    return;
  }

  // Backup GL state
  GLenum last_active_texture;
  glGetIntegerv(GL_ACTIVE_TEXTURE, ( GLint * ) & last_active_texture);
  glActiveTexture(GL_TEXTURE0);
  GLint last_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  GLint last_array_buffer;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
  GLint last_viewport[4];
  glGetIntegerv(GL_VIEWPORT, last_viewport);
  GLint last_scissor_box[4];
  glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
  GLenum last_blend_src_rgb;
  glGetIntegerv(GL_BLEND_SRC_RGB, ( GLint * ) & last_blend_src_rgb);
  GLenum last_blend_dst_rgb;
  glGetIntegerv(GL_BLEND_DST_RGB, ( GLint * ) & last_blend_dst_rgb);
  GLenum last_blend_src_alpha;
  glGetIntegerv(GL_BLEND_SRC_ALPHA, ( GLint * ) & last_blend_src_alpha);
  GLenum last_blend_dst_alpha;
  glGetIntegerv(GL_BLEND_DST_ALPHA, ( GLint * ) & last_blend_dst_alpha);
  GLenum last_blend_equation_rgb;
  glGetIntegerv(GL_BLEND_EQUATION_RGB, ( GLint * ) & last_blend_equation_rgb);
  GLenum last_blend_equation_alpha;
  glGetIntegerv(GL_BLEND_EQUATION_ALPHA, ( GLint * ) & last_blend_equation_alpha);
  GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
  GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
  GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
  GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
  bool clip_origin_lower_left = true;

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);

  // Setup viewport, orthographic projection matrix
  // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
  glViewport(0, 0, (GLsizei) fb_width, (GLsizei) fb_height);
  float L = draw_data->DisplayPos.x;
  float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
  float T = draw_data->DisplayPos.y;
  float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  const float ortho_projection[4][4] =
    {
      { 2.0f / ( R - L ),      0.0f,                  0.0f,  0.0f },
      { 0.0f,                  2.0f / ( T - B ),      0.0f,  0.0f },
      { 0.0f,                  0.0f,                  -1.0f, 0.0f },
      { ( R + L ) / ( L - R ), ( T + B ) / ( B - T ), 0.0f,  1.0f },
    };
  glUseProgram(g_ShaderHandle);
  glUniform1i(g_AttribLocationTex, 0);
  glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
  glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
  glEnableVertexAttribArray(g_AttribLocationPosition);
  glEnableVertexAttribArray(g_AttribLocationUV);
  glEnableVertexAttribArray(g_AttribLocationColor);
  glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                        (GLvoid*) IM_OFFSETOF(ImDrawVert, pos));
  glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                        (GLvoid*) IM_OFFSETOF(ImDrawVert, uv));
  glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                        (GLvoid*) IM_OFFSETOF(ImDrawVert, col));

  // Will project scissor/clipping rectangles into framebuffer space
  ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
  ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

  // Render command lists
  for ( int n = 0; n < draw_data->CmdListsCount; n++ ) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    size_t idx_buffer_offset = 0;

    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                 (const GLvoid*) cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                 (const GLvoid*) cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

    for ( int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++ ) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if ( pcmd->UserCallback ) {
        // User callback (registered via ImDrawList::AddCallback)
        pcmd->UserCallback(cmd_list, pcmd);
      }
      else {
        // Project scissor/clipping rectangles into framebuffer space
        ImVec4 clip_rect;
        clip_rect.x = ( pcmd->ClipRect.x - clip_off.x ) * clip_scale.x;
        clip_rect.y = ( pcmd->ClipRect.y - clip_off.y ) * clip_scale.y;
        clip_rect.z = ( pcmd->ClipRect.z - clip_off.x ) * clip_scale.x;
        clip_rect.w = ( pcmd->ClipRect.w - clip_off.y ) * clip_scale.y;

        if ( clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f ) {
          // Apply scissor/clipping rectangle
          if ( clip_origin_lower_left ) {
            glScissor((int) clip_rect.x, (int) ( fb_height - clip_rect.w ), (int) ( clip_rect.z - clip_rect.x ),
                      (int) ( clip_rect.w - clip_rect.y ));
          }
          else {
            glScissor((int) clip_rect.x, (int) clip_rect.y, (int) clip_rect.z,
                      (int) clip_rect.w);
          } // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)

          // Bind texture, Draw
          glBindTexture(GL_TEXTURE_2D, ( GLuint )(intptr_t)
          pcmd->TextureId);
          glDrawElements(GL_TRIANGLES, (GLsizei) pcmd->ElemCount,
                         sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*) idx_buffer_offset);
        }
      }
      idx_buffer_offset += pcmd->ElemCount * sizeof(ImDrawIdx);
    }
  }

  // Restore modified GL state
  glUseProgram(last_program);
  glBindTexture(GL_TEXTURE_2D, last_texture);

  glActiveTexture(last_active_texture);
  glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
  glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
  glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
  if ( last_enable_blend ) { glEnable(GL_BLEND); } else { glDisable(GL_BLEND); }
  if ( last_enable_cull_face ) { glEnable(GL_CULL_FACE); } else { glDisable(GL_CULL_FACE); }
  if ( last_enable_depth_test ) { glEnable(GL_DEPTH_TEST); } else { glDisable(GL_DEPTH_TEST); }
  if ( last_enable_scissor_test ) { glEnable(GL_SCISSOR_TEST); } else { glDisable(GL_SCISSOR_TEST); }
  glViewport(last_viewport[0], last_viewport[1], (GLsizei) last_viewport[2], (GLsizei) last_viewport[3]);
  glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei) last_scissor_box[2], (GLsizei) last_scissor_box[3]);
}

bool ImGui_ImplGLES2_CreateFontsTexture() {
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width,
                               &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

  // Upload texture to graphics system
  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGenTextures(1, &g_FontTexture);
  glBindTexture(GL_TEXTURE_2D, g_FontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->TexID = ( ImTextureID )(intptr_t)
  g_FontTexture;

  // Restore state
  glBindTexture(GL_TEXTURE_2D, last_texture);

  return true;
}

void ImGui_ImplGLES2_DestroyFontsTexture() {
  if ( g_FontTexture ) {
    ImGuiIO& io = ImGui::GetIO();
    glDeleteTextures(1, &g_FontTexture);
    io.Fonts->TexID = 0;
    g_FontTexture = 0;
  }
}

// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
static bool CheckShader(GLuint handle, const char* desc) {
  GLint status = 0, log_length = 0;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
  glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
  if ((GLboolean) status == GL_FALSE ) {
    fprintf(stderr, "ERROR: ImGui_ImplGLES2_CreateDeviceObjects: failed to compile %s!\n", desc);
  }
  if ( log_length > 0 ) {
    ImVector<char> buf;
    buf.resize((int) ( log_length + 1 ));
    glGetShaderInfoLog(handle, log_length, NULL, (GLchar*) buf.begin());
    fprintf(stderr, "%s\n", buf.begin());
  }
  return (GLboolean) status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
static bool CheckProgram(GLuint handle, const char* desc) {
  GLint status = 0, log_length = 0;
  glGetProgramiv(handle, GL_LINK_STATUS, &status);
  glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
  if ((GLboolean) status == GL_FALSE ) {
    fprintf(stderr, "ERROR: ImGui_ImplGLES2_CreateDeviceObjects: failed to link %s! (with GLSL '%s')\n", desc,
            g_GlslVersionString);
  }
  if ( log_length > 0 ) {
    ImVector<char> buf;
    buf.resize((int) ( log_length + 1 ));
    glGetProgramInfoLog(handle, log_length, NULL, (GLchar*) buf.begin());
    fprintf(stderr, "%s\n", buf.begin());
  }
  return (GLboolean) status == GL_TRUE;
}

bool ImGui_ImplGLES2_CreateDeviceObjects() {
  // Backup GL state
  GLint last_texture, last_array_buffer, last_vertex_array;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
//    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

  // Parse GLSL version string
  int glsl_version = 100;
  sscanf(g_GlslVersionString, "#version %d", &glsl_version);

  static const GLchar* vertex_shader_glsl_100 =
    "uniform mat4 ProjMtx;\n"
    "attribute vec2 Position;\n"
    "attribute vec2 UV;\n"
    "attribute vec4 Color;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "    Frag_UV = UV;\n"
    "    Frag_Color = Color;\n"
    "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";

  static const GLchar* fragment_shader_glsl_100 =
    "#ifdef GL_ES\n"
    "    precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D Texture;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
    "}\n";

  const GLchar* vertex_shader = vertex_shader_glsl_100;
  const GLchar* fragment_shader = fragment_shader_glsl_100;

  // Create shaders
  const GLchar* vertex_shader_with_version[2] = { g_GlslVersionString, vertex_shader };
  g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(g_VertHandle, 2, vertex_shader_with_version, NULL);
  glCompileShader(g_VertHandle);
  CheckShader(g_VertHandle, "vertex shader");

  const GLchar* fragment_shader_with_version[2] = { g_GlslVersionString, fragment_shader };
  g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(g_FragHandle, 2, fragment_shader_with_version, NULL);
  glCompileShader(g_FragHandle);
  CheckShader(g_FragHandle, "fragment shader");

  g_ShaderHandle = glCreateProgram();
  glAttachShader(g_ShaderHandle, g_VertHandle);
  glAttachShader(g_ShaderHandle, g_FragHandle);
  glLinkProgram(g_ShaderHandle);
  CheckProgram(g_ShaderHandle, "shader program");

  g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
  g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
  g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
  g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
  g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

  // Create buffers
  glGenBuffers(1, &g_VboHandle);
  glGenBuffers(1, &g_ElementsHandle);

  ImGui_ImplGLES2_CreateFontsTexture();

  // Restore modified GL state
  glBindTexture(GL_TEXTURE_2D, last_texture);
  glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);

  return true;
}

void ImGui_ImplGLES2_DestroyDeviceObjects() {
  if ( g_VboHandle ) { glDeleteBuffers(1, &g_VboHandle); }
  if ( g_ElementsHandle ) { glDeleteBuffers(1, &g_ElementsHandle); }
  g_VboHandle = g_ElementsHandle = 0;

  if ( g_ShaderHandle && g_VertHandle ) { glDetachShader(g_ShaderHandle, g_VertHandle); }
  if ( g_VertHandle ) { glDeleteShader(g_VertHandle); }
  g_VertHandle = 0;

  if ( g_ShaderHandle && g_FragHandle ) { glDetachShader(g_ShaderHandle, g_FragHandle); }
  if ( g_FragHandle ) { glDeleteShader(g_FragHandle); }
  g_FragHandle = 0;

  if ( g_ShaderHandle ) { glDeleteProgram(g_ShaderHandle); }
  g_ShaderHandle = 0;

  ImGui_ImplGLES2_DestroyFontsTexture();
}

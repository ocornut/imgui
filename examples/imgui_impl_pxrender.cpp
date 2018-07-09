// ImGui Renderer for: px_render
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)
// (Note: We are using GL3W as a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc..)

// Implemented features:
//  [X] Renderer: User texture binding. Use Pointer to Texture Object ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG 
// (minor and older changes stripped away, please see git history for details)
//  2018-07-09: Initial version (@pplux) based on previous OpenGL3 impl.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cassert>
#include "imgui.h"
#include "imgui_impl_pxrender.h"

#include "px_render.h"

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions. You may use another OpenGL loader/header such as: glew, glext, glad, glLoadGen, etc.

static struct {
  px_render::RenderContext *ctx = nullptr;
  px_render::Pipeline pipeline;
  px_render::Texture font;
  px_render::Buffer vertex;
  px_render::Buffer index;
  uint32_t vertex_size = 0;
  uint32_t index_size = 0;
} PRS; // Px-Render-State

void ImGui_Impl_pxrender_Init(px_render::RenderContext *ctx) {
  using namespace px_render;
  assert(PRS.ctx == nullptr &&  "ImGUI_Impl_pxrender_Init() called twice");
  PRS.ctx = ctx;

  // -- Create Pipeline Object ---------------------------------------------
  px_render::Pipeline::Info pinfo;
  #define GLSL(...) "#version 330\n" #__VA_ARGS__
  pinfo.shader.vertex = GLSL(
    uniform mat4 u_projection;
    in vec2 pos;
    in vec2 uv;
    in vec4 color;
    out vec2 frag_uv;
    out vec4 frag_color;
    void main() {
      frag_uv = uv;
      frag_color = color;
      gl_Position = u_projection*vec4(pos, 0.0, 1.0);
    } 
  );
  pinfo.shader.fragment = GLSL(
    uniform sampler2D u_tex0;
    in vec2 frag_uv;
    in vec4 frag_color;
    out vec4 color;
    void main() {
      color = vec4(frag_color.rgb, texture(u_tex0, frag_uv).r*frag_color.a);
    }
  );
  #undef GLSL
  pinfo.attribs[0] = {"pos", VertexFormat::Float2};
  pinfo.attribs[1] = {"uv", VertexFormat::Float2};
  pinfo.attribs[2] = {"color", VertexFormat::UInt8 | VertexFormat::NumComponents4 | VertexFormat::Normalized};
  pinfo.textures[0] = TextureType::T2D;
  pinfo.blend.enabled = true;
  pinfo.blend.op_rgb = pinfo.blend.op_alpha = BlendOp::Add;
  pinfo.blend.src_rgb = pinfo.blend.src_alpha = BlendFactor::SrcAlpha;
  pinfo.blend.dst_rgb = pinfo.blend.dst_alpha = BlendFactor::OneMinusSrcAlpha;
  pinfo.cull = Cull::Disabled;
  pinfo.depth_func = CompareFunc::Always;
  pinfo.depth_write = false;
  PRS.pipeline = PRS.ctx->createPipeline(pinfo);

  // -- Default font texture -----------------------------------------------
  int font_width, font_height;
  unsigned char *font_pixels;
  ::ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&font_pixels, &font_width, &font_height);
  Texture::Info tinfo;
  tinfo.width = font_width;
  tinfo.height = font_height;
  tinfo.format = TexelsFormat::R_U8;
  PRS.font = PRS.ctx->createTexture(tinfo);
  DisplayList tex_dl;
  tex_dl.fillTextureCommand()
    .set_texture(PRS.font)
    .set_data(font_pixels);
  PRS.ctx->submitDisplayList(std::move(tex_dl));
  ::ImGui::GetIO().Fonts->TexID = (void*)&PRS.font;
}

void ImGui_Impl_pxrender_Shutdown() {
  using namespace px_render;
  DisplayList sdl;
  sdl
    .destroy(PRS.font)
    .destroy(PRS.pipeline)
    .destroy(PRS.vertex)
    .destroy(PRS.index)
    ;
  PRS.ctx->submitDisplayList(std::move(sdl));
  PRS.ctx = nullptr;
  PRS.vertex = 0;
  PRS.index = 0;
}

void ImGui_Impl_pxrender_RenderDrawData(ImDrawData* draw_data, px_render::DisplayList *dl_output) {
  if (!draw_data || !dl_output) return;

  using namespace px_render;
  ImGuiIO& io = ::ImGui::GetIO();
  int fb_width = (int)(draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
  if (fb_width <= 0 || fb_height <= 0) return;
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  float L = draw_data->DisplayPos.x;
  float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
  float T = draw_data->DisplayPos.y;
  float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  const px_render::Mat4 proj =
  {
     2.0f/(R-L),   0.0f,         0.0f,   0.0f ,
     0.0f,         2.0f/(T-B),   0.0f,   0.0f ,
     0.0f,         0.0f,        -1.0f,   0.0f ,
     (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f 
  };
  dl_output->setupViewCommand()
    .set_projection_matrix(proj)
    .set_viewport({0,0, (uint16_t) fb_width, (uint16_t) fb_height});
    ;
  
  ImVec2 pos = draw_data->DisplayPos;

  for(int n = 0; n < draw_data->CmdListsCount; ++n) {
    const ImDrawList *cmd_list = draw_data->CmdLists[n];
    size_t required_vertex_size = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
    if (required_vertex_size > PRS.vertex_size) {
      dl_output->destroy(PRS.vertex);
      PRS.vertex = PRS.ctx->createBuffer({BufferType::Vertex, required_vertex_size, Usage::Stream});
      PRS.vertex_size = required_vertex_size;
    }
    dl_output->fillBufferCommand()
      .set_buffer(PRS.vertex)
      .set_data(cmd_list->VtxBuffer.Data)
      .set_size(required_vertex_size)
      ;
    size_t required_index_size = cmd_list->IdxBuffer.Size *sizeof(ImDrawIdx);
    if (required_index_size > PRS.index_size) {
      dl_output->destroy(PRS.index);
      PRS.index = PRS.ctx->createBuffer({BufferType::Index, required_index_size, Usage::Stream});
      PRS.index_size = required_index_size;
    }
    dl_output->fillBufferCommand()
      .set_buffer(PRS.index)
      .set_data(cmd_list->IdxBuffer.Data)
      .set_size(required_index_size)
      ;
    uint32_t idx_buffer_offset = 0;

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i) {
      const ImDrawCmd *cmd = &cmd_list->CmdBuffer[cmd_i];
      if (cmd->UserCallback) {
        cmd->UserCallback(cmd_list, cmd);
        continue;
      }
    
     Vec4 clip_rect = { cmd->ClipRect.x - pos.x, cmd->ClipRect.y - pos.y, cmd->ClipRect.z - pos.x, cmd->ClipRect.w - pos.y};
     if (clip_rect.v.x < fb_width && clip_rect.v.y < fb_height && clip_rect.v.z >= 0.0f && clip_rect.v.w >= 0.0f) {
      // invert y on scissor
       Vec4 scissor_rect = { clip_rect.f[0], fb_height - clip_rect.f[3], clip_rect.f[2] - clip_rect.f[0], clip_rect.f[3] - clip_rect.f[1]};
       dl_output->setupPipelineCommand()
        .set_pipeline(PRS.pipeline)
        .set_buffer(0, PRS.vertex)
        .set_texture(0, *(Texture*)cmd->TextureId)
        .set_scissor(scissor_rect)
        ;
       dl_output->renderCommand()
        .set_index_buffer(PRS.index)
        .set_offset(sizeof(ImDrawIdx)*idx_buffer_offset)
        .set_count(cmd->ElemCount)
        .set_type(IndexFormat::UInt16)
        ;
       idx_buffer_offset += cmd->ElemCount;
     }
    }
  }
}

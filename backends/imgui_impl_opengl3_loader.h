//-----------------------------------------------------------------------------
// About imgui_impl_opengl3_loader.h:
//
// We embed our own OpenGL loader to not require user to provide their own or to have to use ours,
// which proved to be endless problems for users.
// Our loader is custom-generated, based on gl3w but automatically filtered to only include
// enums/functions that we use in our imgui_impl_opengl3.cpp source file in order to be small.
//
// YOU SHOULD NOT NEED TO INCLUDE/USE THIS DIRECTLY. THIS IS USED BY imgui_impl_opengl3.cpp ONLY.
// THE REST OF YOUR APP SHOULD USE A DIFFERENT GL LOADER: ANY GL LOADER OF YOUR CHOICE.
//
// Regenerate with:
//   python gl3w_gen.py --output ../imgui/backends/imgui_impl_opengl3_loader.h --ref ../imgui/backends/imgui_impl_opengl3.cpp ./extra_symbols.txt
//
// More info:
//   https://github.com/dearimgui/gl3w_stripped
//   https://github.com/ocornut/imgui/issues/4445
//-----------------------------------------------------------------------------

/*
 * This file was generated with gl3w_gen.py, part of imgl3w
 * (hosted at https://github.com/dearimgui/gl3w_stripped)
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __gl3w_h_
#define __gl3w_h_

// Adapted from KHR/khrplatform.h to avoid including entire file.
#ifndef __khrplatform_h_
typedef          float         khronos_float_t;
typedef signed   char          khronos_int8_t;
typedef unsigned char          khronos_uint8_t;
typedef signed   short int     khronos_int16_t;
typedef unsigned short int     khronos_uint16_t;
#ifdef _WIN64
typedef signed   long long int khronos_intptr_t;
typedef signed   long long int khronos_ssize_t;
#else
typedef signed   long  int     khronos_intptr_t;
typedef signed   long  int     khronos_ssize_t;
#endif

#if defined(_MSC_VER) && !defined(__clang__)
typedef signed   __int64       khronos_int64_t;
typedef unsigned __int64       khronos_uint64_t;
#elif (defined(__clang__) || defined(__GNUC__)) && (__cplusplus < 201100)
#include <stdint.h>
typedef          int64_t       khronos_int64_t;
typedef          uint64_t      khronos_uint64_t;
#else
typedef signed   long long     khronos_int64_t;
typedef unsigned long long     khronos_uint64_t;
#endif
#endif  // __khrplatform_h_

#ifndef __gl_glcorearb_h_
#define __gl_glcorearb_h_ 1
#ifdef __cplusplus
extern "C" {
#endif
/*
** Copyright 2013-2020 The Khronos Group Inc.
** SPDX-License-Identifier: MIT
**
** This header is generated from the Khronos OpenGL / OpenGL ES XML
** API Registry. The current version of the Registry, generator scripts
** used to make the header, and the header can be found at
**   https://github.com/KhronosGroup/OpenGL-Registry
*/
#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif
/* glcorearb.h is for use with OpenGL core profile implementations.
** It should should be placed in the same directory as gl.h and
** included as <GL/glcorearb.h>.
**
** glcorearb.h includes only APIs in the latest OpenGL core profile
** implementation together with APIs in newer ARB extensions which 
** can be supported by the core profile. It does not, and never will
** include functionality removed from the core profile, such as
** fixed-function vertex and fragment processing.
**
** Do not #include both <GL/glcorearb.h> and either of <GL/gl.h> or
** <GL/glext.h> in the same source file.
*/
/* Generated C header for:
 * API: gl
 * Profile: core
 * Versions considered: .*
 * Versions emitted: .*
 * Default extensions included: glcore
 * Additional extensions included: _nomatch_^
 * Extensions removed: _nomatch_^
 */
#ifndef GL_VERSION_1_0
typedef void GLvoid;
typedef unsigned int GLenum;

typedef khronos_float_t GLfloat;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef double GLdouble;
typedef unsigned int GLuint;
typedef unsigned char GLboolean;
typedef khronos_uint8_t GLubyte;
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_TRIANGLES                      0x0004
#define GL_ONE                            1
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_FRONT_AND_BACK                 0x0408
#define GL_POLYGON_MODE                   0x0B40
#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_STENCIL_TEST                   0x0B90
#define GL_VIEWPORT                       0x0BA2
#define GL_BLEND                          0x0BE2
#define GL_SCISSOR_BOX                    0x0C10
#define GL_SCISSOR_TEST                   0x0C11
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#define GL_PACK_ALIGNMENT                 0x0D05
#define GL_TEXTURE_2D                     0x0DE1
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_RGBA                           0x1908
#define GL_FILL                           0x1B02
#define GL_VERSION                        0x1F02
#define GL_EXTENSIONS                     0x1F03
#define GL_LINEAR                         0x2601
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
typedef void (APIENTRYP PFNGLPOLYGONMODEPROC) (GLenum face, GLenum mode);
typedef void (APIENTRYP PFNGLSCISSORPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRYP PFNGLCLEARPROC) (GLbitfield mask);
typedef void (APIENTRYP PFNGLCLEARCOLORPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRYP PFNGLDISABLEPROC) (GLenum cap);
typedef void (APIENTRYP PFNGLENABLEPROC) (GLenum cap);
typedef void (APIENTRYP PFNGLPIXELSTOREIPROC) (GLenum pname, GLint param);
typedef void (APIENTRYP PFNGLREADPIXELSPROC) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
typedef GLenum (APIENTRYP PFNGLGETERRORPROC) (void);
typedef void (APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint *data);
typedef const GLubyte *(APIENTRYP PFNGLGETSTRINGPROC) (GLenum name);
typedef GLboolean (APIENTRYP PFNGLISENABLEDPROC) (GLenum cap);
typedef void (APIENTRYP PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glPolygonMode (GLenum face, GLenum mode);
GLAPI void APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height);
GLAPI void APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param);
GLAPI void APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
GLAPI void APIENTRY glClear (GLbitfield mask);
GLAPI void APIENTRY glClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLAPI void APIENTRY glDisable (GLenum cap);
GLAPI void APIENTRY glEnable (GLenum cap);
GLAPI void APIENTRY glPixelStorei (GLenum pname, GLint param);
GLAPI void APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
GLAPI GLenum APIENTRY glGetError (void);
GLAPI void APIENTRY glGetIntegerv (GLenum pname, GLint *data);
GLAPI const GLubyte *APIENTRY glGetString (GLenum name);
GLAPI GLboolean APIENTRY glIsEnabled (GLenum cap);
GLAPI void APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height);
#endif
#endif /* GL_VERSION_1_0 */
#ifndef GL_VERSION_1_1
typedef khronos_float_t GLclampf;
typedef double GLclampd;
#define GL_TEXTURE_BINDING_2D             0x8069
typedef void (APIENTRYP PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (APIENTRYP PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRYP PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (APIENTRYP PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices);
GLAPI void APIENTRY glBindTexture (GLenum target, GLuint texture);
GLAPI void APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures);
GLAPI void APIENTRY glGenTextures (GLsizei n, GLuint *textures);
#endif
#endif /* GL_VERSION_1_1 */
#ifndef GL_VERSION_1_3
#define GL_TEXTURE0                       0x84C0
#define GL_ACTIVE_TEXTURE                 0x84E0
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glActiveTexture (GLenum texture);
#endif
#endif /* GL_VERSION_1_3 */
#ifndef GL_VERSION_1_4
#define GL_BLEND_DST_RGB                  0x80C8
#define GL_BLEND_SRC_RGB                  0x80C9
#define GL_BLEND_DST_ALPHA                0x80CA
#define GL_BLEND_SRC_ALPHA                0x80CB
#define GL_FUNC_ADD                       0x8006
typedef void (APIENTRYP PFNGLBLENDFUNCSEPARATEPROC) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (APIENTRYP PFNGLBLENDEQUATIONPROC) (GLenum mode);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GLAPI void APIENTRY glBlendEquation (GLenum mode);
#endif
#endif /* GL_VERSION_1_4 */
#ifndef GL_VERSION_1_5
typedef khronos_ssize_t GLsizeiptr;
typedef khronos_intptr_t GLintptr;
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_ARRAY_BUFFER_BINDING           0x8894
#define GL_STREAM_DRAW                    0x88E0
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glBindBuffer (GLenum target, GLuint buffer);
GLAPI void APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers);
GLAPI void APIENTRY glGenBuffers (GLsizei n, GLuint *buffers);
GLAPI void APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
#endif
#endif /* GL_VERSION_1_5 */
#ifndef GL_VERSION_2_0
typedef char GLchar;
typedef khronos_int16_t GLshort;
typedef khronos_int8_t GLbyte;
typedef khronos_uint16_t GLushort;
#define GL_BLEND_EQUATION_RGB             0x8009
#define GL_BLEND_EQUATION_ALPHA           0x883D
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_CURRENT_PROGRAM                0x8B8D
#define GL_UPPER_LEFT                     0x8CA2
typedef void (APIENTRYP PFNGLBLENDEQUATIONSEPARATEPROC) (GLenum modeRGB, GLenum modeAlpha);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef GLint (APIENTRYP PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha);
GLAPI void APIENTRY glAttachShader (GLuint program, GLuint shader);
GLAPI void APIENTRY glCompileShader (GLuint shader);
GLAPI GLuint APIENTRY glCreateProgram (void);
GLAPI GLuint APIENTRY glCreateShader (GLenum type);
GLAPI void APIENTRY glDeleteProgram (GLuint program);
GLAPI void APIENTRY glDeleteShader (GLuint shader);
GLAPI void APIENTRY glDetachShader (GLuint program, GLuint shader);
GLAPI void APIENTRY glEnableVertexAttribArray (GLuint index);
GLAPI GLint APIENTRY glGetAttribLocation (GLuint program, const GLchar *name);
GLAPI void APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name);
GLAPI void APIENTRY glLinkProgram (GLuint program);
GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
GLAPI void APIENTRY glUseProgram (GLuint program);
GLAPI void APIENTRY glUniform1i (GLint location, GLint v0);
GLAPI void APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLAPI void APIENTRY glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
#endif
#endif /* GL_VERSION_2_0 */
#ifndef GL_VERSION_3_0
typedef khronos_uint16_t GLhalf;
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_NUM_EXTENSIONS                 0x821D
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_VERTEX_ARRAY_BINDING           0x85B5
typedef void (APIENTRYP PFNGLGETBOOLEANI_VPROC) (GLenum target, GLuint index, GLboolean *data);
typedef void (APIENTRYP PFNGLGETINTEGERI_VPROC) (GLenum target, GLuint index, GLint *data);
typedef const GLubyte *(APIENTRYP PFNGLGETSTRINGIPROC) (GLenum name, GLuint index);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI const GLubyte *APIENTRY glGetStringi (GLenum name, GLuint index);
GLAPI void APIENTRY glBindVertexArray (GLuint array);
GLAPI void APIENTRY glDeleteVertexArrays (GLsizei n, const GLuint *arrays);
GLAPI void APIENTRY glGenVertexArrays (GLsizei n, GLuint *arrays);
#endif
#endif /* GL_VERSION_3_0 */
#ifndef GL_VERSION_3_1
#define GL_VERSION_3_1 1
#define GL_PRIMITIVE_RESTART              0x8F9D
#endif /* GL_VERSION_3_1 */
#ifndef GL_VERSION_3_2
#define GL_VERSION_3_2 1
typedef struct __GLsync *GLsync;
typedef khronos_uint64_t GLuint64;
typedef khronos_int64_t GLint64;
typedef void (APIENTRYP PFNGLDRAWELEMENTSBASEVERTEXPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
typedef void (APIENTRYP PFNGLGETINTEGER64I_VPROC) (GLenum target, GLuint index, GLint64 *data);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glDrawElementsBaseVertex (GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
#endif
#endif /* GL_VERSION_3_2 */
#ifndef GL_VERSION_3_3
#define GL_VERSION_3_3 1
#define GL_SAMPLER_BINDING                0x8919
typedef void (APIENTRYP PFNGLBINDSAMPLERPROC) (GLuint unit, GLuint sampler);
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glBindSampler (GLuint unit, GLuint sampler);
#endif
#endif /* GL_VERSION_3_3 */
#ifndef GL_VERSION_4_1
typedef void (APIENTRYP PFNGLGETFLOATI_VPROC) (GLenum target, GLuint index, GLfloat *data);
typedef void (APIENTRYP PFNGLGETDOUBLEI_VPROC) (GLenum target, GLuint index, GLdouble *data);
#endif /* GL_VERSION_4_1 */
#ifndef GL_VERSION_4_3
typedef void (APIENTRY  *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
#endif /* GL_VERSION_4_3 */
#ifndef GL_VERSION_4_5
#define GL_CLIP_ORIGIN                    0x935C
typedef void (APIENTRYP PFNGLGETTRANSFORMFEEDBACKI_VPROC) (GLuint xfb, GLenum pname, GLuint index, GLint *param);
typedef void (APIENTRYP PFNGLGETTRANSFORMFEEDBACKI64_VPROC) (GLuint xfb, GLenum pname, GLuint index, GLint64 *param);
#endif /* GL_VERSION_4_5 */
#ifndef GL_ARB_bindless_texture
typedef khronos_uint64_t GLuint64EXT;
#endif /* GL_ARB_bindless_texture */
#ifndef GL_ARB_cl_event
struct _cl_context;
struct _cl_event;
#endif /* GL_ARB_cl_event */
#ifndef GL_ARB_clip_control
#define GL_ARB_clip_control 1
#endif /* GL_ARB_clip_control */
#ifndef GL_ARB_debug_output
typedef void (APIENTRY  *GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
#endif /* GL_ARB_debug_output */
#ifndef GL_EXT_EGL_image_storage
typedef void *GLeglImageOES;
#endif /* GL_EXT_EGL_image_storage */
#ifndef GL_EXT_direct_state_access
typedef void (APIENTRYP PFNGLGETFLOATI_VEXTPROC) (GLenum pname, GLuint index, GLfloat *params);
typedef void (APIENTRYP PFNGLGETDOUBLEI_VEXTPROC) (GLenum pname, GLuint index, GLdouble *params);
typedef void (APIENTRYP PFNGLGETPOINTERI_VEXTPROC) (GLenum pname, GLuint index, void **params);
typedef void (APIENTRYP PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC) (GLuint vaobj, GLuint index, GLenum pname, GLint *param);
typedef void (APIENTRYP PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC) (GLuint vaobj, GLuint index, GLenum pname, void **param);
#endif /* GL_EXT_direct_state_access */
#ifndef GL_NV_draw_vulkan_image
typedef void (APIENTRY  *GLVULKANPROCNV)(void);
#endif /* GL_NV_draw_vulkan_image */
#ifndef GL_NV_gpu_shader5
typedef khronos_int64_t GLint64EXT;
#endif /* GL_NV_gpu_shader5 */
#ifndef GL_NV_vertex_buffer_unified_memory
typedef void (APIENTRYP PFNGLGETINTEGERUI64I_VNVPROC) (GLenum value, GLuint index, GLuint64EXT *result);
#endif /* GL_NV_vertex_buffer_unified_memory */
#ifdef __cplusplus
}
#endif
#endif

#ifndef GL3W_API
#define GL3W_API
#endif

#ifndef __gl_h_
#define __gl_h_
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GL3W_OK 0
#define GL3W_ERROR_INIT -1
#define GL3W_ERROR_LIBRARY_OPEN -2
#define GL3W_ERROR_OPENGL_VERSION -3

typedef void (*GL3WglProc)(void);
typedef GL3WglProc (*GL3WGetProcAddressProc)(const char *proc);

/* gl3w api */
GL3W_API int imgl3wInit(void);
GL3W_API int imgl3wInit2(GL3WGetProcAddressProc proc);
GL3W_API int imgl3wIsSupported(int major, int minor);
GL3W_API GL3WglProc imgl3wGetProcAddress(const char *proc);

/* gl3w internal state */
union GL3WProcs {
    GL3WglProc ptr[53];
    struct {
        PFNGLACTIVETEXTUREPROC           ActiveTexture;
        PFNGLATTACHSHADERPROC            AttachShader;
        PFNGLBINDBUFFERPROC              BindBuffer;
        PFNGLBINDSAMPLERPROC             BindSampler;
        PFNGLBINDTEXTUREPROC             BindTexture;
        PFNGLBINDVERTEXARRAYPROC         BindVertexArray;
        PFNGLBLENDEQUATIONPROC           BlendEquation;
        PFNGLBLENDEQUATIONSEPARATEPROC   BlendEquationSeparate;
        PFNGLBLENDFUNCSEPARATEPROC       BlendFuncSeparate;
        PFNGLBUFFERDATAPROC              BufferData;
        PFNGLCLEARPROC                   Clear;
        PFNGLCLEARCOLORPROC              ClearColor;
        PFNGLCOMPILESHADERPROC           CompileShader;
        PFNGLCREATEPROGRAMPROC           CreateProgram;
        PFNGLCREATESHADERPROC            CreateShader;
        PFNGLDELETEBUFFERSPROC           DeleteBuffers;
        PFNGLDELETEPROGRAMPROC           DeleteProgram;
        PFNGLDELETESHADERPROC            DeleteShader;
        PFNGLDELETETEXTURESPROC          DeleteTextures;
        PFNGLDELETEVERTEXARRAYSPROC      DeleteVertexArrays;
        PFNGLDETACHSHADERPROC            DetachShader;
        PFNGLDISABLEPROC                 Disable;
        PFNGLDRAWELEMENTSPROC            DrawElements;
        PFNGLDRAWELEMENTSBASEVERTEXPROC  DrawElementsBaseVertex;
        PFNGLENABLEPROC                  Enable;
        PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
        PFNGLGENBUFFERSPROC              GenBuffers;
        PFNGLGENTEXTURESPROC             GenTextures;
        PFNGLGENVERTEXARRAYSPROC         GenVertexArrays;
        PFNGLGETATTRIBLOCATIONPROC       GetAttribLocation;
        PFNGLGETERRORPROC                GetError;
        PFNGLGETINTEGERVPROC             GetIntegerv;
        PFNGLGETPROGRAMINFOLOGPROC       GetProgramInfoLog;
        PFNGLGETPROGRAMIVPROC            GetProgramiv;
        PFNGLGETSHADERINFOLOGPROC        GetShaderInfoLog;
        PFNGLGETSHADERIVPROC             GetShaderiv;
        PFNGLGETSTRINGPROC               GetString;
        PFNGLGETSTRINGIPROC              GetStringi;
        PFNGLGETUNIFORMLOCATIONPROC      GetUniformLocation;
        PFNGLISENABLEDPROC               IsEnabled;
        PFNGLLINKPROGRAMPROC             LinkProgram;
        PFNGLPIXELSTOREIPROC             PixelStorei;
        PFNGLPOLYGONMODEPROC             PolygonMode;
        PFNGLREADPIXELSPROC              ReadPixels;
        PFNGLSCISSORPROC                 Scissor;
        PFNGLSHADERSOURCEPROC            ShaderSource;
        PFNGLTEXIMAGE2DPROC              TexImage2D;
        PFNGLTEXPARAMETERIPROC           TexParameteri;
        PFNGLUNIFORM1IPROC               Uniform1i;
        PFNGLUNIFORMMATRIX4FVPROC        UniformMatrix4fv;
        PFNGLUSEPROGRAMPROC              UseProgram;
        PFNGLVERTEXATTRIBPOINTERPROC     VertexAttribPointer;
        PFNGLVIEWPORTPROC                Viewport;
    } gl;
};

GL3W_API extern union GL3WProcs imgl3wProcs;

/* OpenGL functions */
#define glActiveTexture                  imgl3wProcs.gl.ActiveTexture
#define glAttachShader                   imgl3wProcs.gl.AttachShader
#define glBindBuffer                     imgl3wProcs.gl.BindBuffer
#define glBindSampler                    imgl3wProcs.gl.BindSampler
#define glBindTexture                    imgl3wProcs.gl.BindTexture
#define glBindVertexArray                imgl3wProcs.gl.BindVertexArray
#define glBlendEquation                  imgl3wProcs.gl.BlendEquation
#define glBlendEquationSeparate          imgl3wProcs.gl.BlendEquationSeparate
#define glBlendFuncSeparate              imgl3wProcs.gl.BlendFuncSeparate
#define glBufferData                     imgl3wProcs.gl.BufferData
#define glClear                          imgl3wProcs.gl.Clear
#define glClearColor                     imgl3wProcs.gl.ClearColor
#define glCompileShader                  imgl3wProcs.gl.CompileShader
#define glCreateProgram                  imgl3wProcs.gl.CreateProgram
#define glCreateShader                   imgl3wProcs.gl.CreateShader
#define glDeleteBuffers                  imgl3wProcs.gl.DeleteBuffers
#define glDeleteProgram                  imgl3wProcs.gl.DeleteProgram
#define glDeleteShader                   imgl3wProcs.gl.DeleteShader
#define glDeleteTextures                 imgl3wProcs.gl.DeleteTextures
#define glDeleteVertexArrays             imgl3wProcs.gl.DeleteVertexArrays
#define glDetachShader                   imgl3wProcs.gl.DetachShader
#define glDisable                        imgl3wProcs.gl.Disable
#define glDrawElements                   imgl3wProcs.gl.DrawElements
#define glDrawElementsBaseVertex         imgl3wProcs.gl.DrawElementsBaseVertex
#define glEnable                         imgl3wProcs.gl.Enable
#define glEnableVertexAttribArray        imgl3wProcs.gl.EnableVertexAttribArray
#define glGenBuffers                     imgl3wProcs.gl.GenBuffers
#define glGenTextures                    imgl3wProcs.gl.GenTextures
#define glGenVertexArrays                imgl3wProcs.gl.GenVertexArrays
#define glGetAttribLocation              imgl3wProcs.gl.GetAttribLocation
#define glGetError                       imgl3wProcs.gl.GetError
#define glGetIntegerv                    imgl3wProcs.gl.GetIntegerv
#define glGetProgramInfoLog              imgl3wProcs.gl.GetProgramInfoLog
#define glGetProgramiv                   imgl3wProcs.gl.GetProgramiv
#define glGetShaderInfoLog               imgl3wProcs.gl.GetShaderInfoLog
#define glGetShaderiv                    imgl3wProcs.gl.GetShaderiv
#define glGetString                      imgl3wProcs.gl.GetString
#define glGetStringi                     imgl3wProcs.gl.GetStringi
#define glGetUniformLocation             imgl3wProcs.gl.GetUniformLocation
#define glIsEnabled                      imgl3wProcs.gl.IsEnabled
#define glLinkProgram                    imgl3wProcs.gl.LinkProgram
#define glPixelStorei                    imgl3wProcs.gl.PixelStorei
#define glPolygonMode                    imgl3wProcs.gl.PolygonMode
#define glReadPixels                     imgl3wProcs.gl.ReadPixels
#define glScissor                        imgl3wProcs.gl.Scissor
#define glShaderSource                   imgl3wProcs.gl.ShaderSource
#define glTexImage2D                     imgl3wProcs.gl.TexImage2D
#define glTexParameteri                  imgl3wProcs.gl.TexParameteri
#define glUniform1i                      imgl3wProcs.gl.Uniform1i
#define glUniformMatrix4fv               imgl3wProcs.gl.UniformMatrix4fv
#define glUseProgram                     imgl3wProcs.gl.UseProgram
#define glVertexAttribPointer            imgl3wProcs.gl.VertexAttribPointer
#define glViewport                       imgl3wProcs.gl.Viewport

#ifdef __cplusplus
}
#endif

#endif

#ifdef IMGL3W_IMPL
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

static HMODULE libgl;
typedef PROC(__stdcall* GL3WglGetProcAddr)(LPCSTR);
static GL3WglGetProcAddr wgl_get_proc_address;

static int open_libgl(void)
{
    libgl = LoadLibraryA("opengl32.dll");
    if (!libgl)
        return GL3W_ERROR_LIBRARY_OPEN;
    wgl_get_proc_address = (GL3WglGetProcAddr)GetProcAddress(libgl, "wglGetProcAddress");
    return GL3W_OK;
}

static void close_libgl(void) { FreeLibrary(libgl); }
static GL3WglProc get_proc(const char *proc)
{
    GL3WglProc res;
    res = (GL3WglProc)wgl_get_proc_address(proc);
    if (!res)
        res = (GL3WglProc)GetProcAddress(libgl, proc);
    return res;
}
#elif defined(__APPLE__)
#include <dlfcn.h>

static void *libgl;
static int open_libgl(void)
{
    libgl = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY | RTLD_LOCAL);
    if (!libgl)
        return GL3W_ERROR_LIBRARY_OPEN;
    return GL3W_OK;
}

static void close_libgl(void) { dlclose(libgl); }

static GL3WglProc get_proc(const char *proc)
{
    GL3WglProc res;
    *(void **)(&res) = dlsym(libgl, proc);
    return res;
}
#else
#include <dlfcn.h>

static void *libgl;
static GL3WglProc (*glx_get_proc_address)(const GLubyte *);

static int open_libgl(void)
{
    libgl = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (!libgl)
        return GL3W_ERROR_LIBRARY_OPEN;
    *(void **)(&glx_get_proc_address) = dlsym(libgl, "glXGetProcAddressARB");
    return GL3W_OK;
}

static void close_libgl(void) { dlclose(libgl); }

static GL3WglProc get_proc(const char *proc)
{
    GL3WglProc res;
    res = glx_get_proc_address((const GLubyte *)proc);
    if (!res)
        *(void **)(&res) = dlsym(libgl, proc);
    return res;
}
#endif

static struct { int major, minor; } version;

static int parse_version(void)
{
    if (!glGetIntegerv)
        return GL3W_ERROR_INIT;
    glGetIntegerv(GL_MAJOR_VERSION, &version.major);
    glGetIntegerv(GL_MINOR_VERSION, &version.minor);
    if (version.major < 3)
        return GL3W_ERROR_OPENGL_VERSION;
    return GL3W_OK;
}

static void load_procs(GL3WGetProcAddressProc proc);

int imgl3wInit(void)
{
    int res = open_libgl();
    if (res)
        return res;
    atexit(close_libgl);
    return imgl3wInit2(get_proc);
}

int imgl3wInit2(GL3WGetProcAddressProc proc)
{
    load_procs(proc);
    return parse_version();
}

int imgl3wIsSupported(int major, int minor)
{
    if (major < 3)
        return 0;
    if (version.major == major)
        return version.minor >= minor;
    return version.major >= major;
}

GL3WglProc imgl3wGetProcAddress(const char *proc) { return get_proc(proc); }

static const char *proc_names[] = {
    "glActiveTexture",
    "glAttachShader",
    "glBindBuffer",
    "glBindSampler",
    "glBindTexture",
    "glBindVertexArray",
    "glBlendEquation",
    "glBlendEquationSeparate",
    "glBlendFuncSeparate",
    "glBufferData",
    "glClear",
    "glClearColor",
    "glCompileShader",
    "glCreateProgram",
    "glCreateShader",
    "glDeleteBuffers",
    "glDeleteProgram",
    "glDeleteShader",
    "glDeleteTextures",
    "glDeleteVertexArrays",
    "glDetachShader",
    "glDisable",
    "glDrawElements",
    "glDrawElementsBaseVertex",
    "glEnable",
    "glEnableVertexAttribArray",
    "glGenBuffers",
    "glGenTextures",
    "glGenVertexArrays",
    "glGetAttribLocation",
    "glGetError",
    "glGetIntegerv",
    "glGetProgramInfoLog",
    "glGetProgramiv",
    "glGetShaderInfoLog",
    "glGetShaderiv",
    "glGetString",
    "glGetStringi",
    "glGetUniformLocation",
    "glIsEnabled",
    "glLinkProgram",
    "glPixelStorei",
    "glPolygonMode",
    "glReadPixels",
    "glScissor",
    "glShaderSource",
    "glTexImage2D",
    "glTexParameteri",
    "glUniform1i",
    "glUniformMatrix4fv",
    "glUseProgram",
    "glVertexAttribPointer",
    "glViewport",
};

GL3W_API union GL3WProcs imgl3wProcs;

static void load_procs(GL3WGetProcAddressProc proc)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(proc_names); i++)
        imgl3wProcs.ptr[i] = proc(proc_names[i]);
}

#ifdef __cplusplus
}
#endif
#endif

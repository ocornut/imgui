//========================================================================
// Simple GLFW+Metal+Imgui example by DanCraft99@github.com
// Based on Simple GLFW+Metal example
// Copyright (c) Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================
//! [code]
#define GLFW_INCLUDE_NONE
#import <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3native.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <simd/simd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_impl_glfw.h"

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}
int main(void)
{
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	if (!device)
    exit(EXIT_FAILURE);

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(1280, 720, "Metal Example", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  NSWindow* nswin = glfwGetCocoaWindow(window);
  CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = layer;
    nswin.contentView.wantsLayer = YES;
    MTLCompileOptions* compileOptions = [MTLCompileOptions new];
    compileOptions.languageVersion = MTLLanguageVersion1_1;
    NSError* compileError;
    id<MTLLibrary> lib = [device newLibraryWithSource:
       @"#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "vertex float4 v_simple(\n"
        "    constant float4* in  [[buffer(0)]],\n"
        "    uint             vid [[vertex_id]])\n"
        "{\n"
        "    return in[vid];\n"
        "}\n"
        "fragment float4 f_simple(\n"
        "    float4 in [[stage_in]])\n"
        "{\n"
        "    return float4(1, 0, 0, 1);\n"
        "}\n"
       options:compileOptions error:&compileError];
    if (!lib)
    {
        NSLog(@"can't create library: %@", compileError);
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    id<MTLFunction> vs = [lib newFunctionWithName:@"v_simple"];
    assert(vs);
    id<MTLFunction> fs = [lib newFunctionWithName:@"f_simple"];
    assert(fs);
    id<MTLCommandQueue> cq = [device newCommandQueue];
    assert(cq);
    MTLRenderPipelineDescriptor* rpd = [MTLRenderPipelineDescriptor new];
    rpd.vertexFunction = vs;
    rpd.fragmentFunction = fs;
    rpd.colorAttachments[0].pixelFormat = layer.pixelFormat;
    id<MTLRenderPipelineState> rps = [device newRenderPipelineStateWithDescriptor:rpd error:NULL];
    assert(rps);
    glfwSetKeyCallback(window, key_callback);

    // initWithView
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplMetal_Init(device);
    ImGui_ImplGlfw_InitForMetal(window, true);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float ratio;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;
        layer.drawableSize = CGSizeMake(width, height);
        id<CAMetalDrawable> drawable = [layer nextDrawable];
        assert(drawable);
        id<MTLCommandBuffer> cb = [cq commandBuffer];
        MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor new];
        MTLRenderPassColorAttachmentDescriptor* cd = rpd.colorAttachments[0];
        cd.texture = drawable.texture;
        cd.loadAction = MTLLoadActionClear;
        cd.clearColor = MTLClearColorMake(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        cd.storeAction = MTLStoreActionStore;
        id<MTLRenderCommandEncoder> rce = [cb renderCommandEncoderWithDescriptor:rpd];
        [rce setRenderPipelineState:rps];
        // [rce setVertexBytes:(vector_float4[]){
        //     { 0, 0, 0, 1 },
        //     { -1, 1, 0, 1 },
        //     { 1, 1, 0, 1 },
        // } length:3 * sizeof(vector_float4) atIndex:0];
        // [rce drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

        ////
        ImGui_ImplMetal_NewFrame(rpd);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
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

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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

        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cb, rce);

        [rce endEncoding];
        [cb presentDrawable:drawable];
        [cb commit];
    }

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
//! [code]

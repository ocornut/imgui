cmake_minimum_required(VERSION 3.5)

project(imgui CXX)

option(IMGUI_BACKEND_ALLEGRO5 "Allegro Gaming Library")
option(IMGUI_BACKEND_ANDROID "Android native API")
option(IMGUI_BACKEND_DX9 "DirectX9")
option(IMGUI_BACKEND_DX10 "DirectX10")
option(IMGUI_BACKEND_DX11 "DirectX11")
option(IMGUI_BACKEND_DX12 "DirectX12")
option(IMGUI_BACKEND_GLFW "GLFW (Windows, macOS, Linux, etc.)")
option(IMGUI_BACKEND_GLUT "GLUT/FreeGLUT (legacy, not recommended today)")
option(IMGUI_BACKEND_MARMALADE "Marmalade SDK")
option(IMGUI_BACKEND_METAL "Metal (with ObjC)")
option(IMGUI_BACKEND_OPENGL2 "OpenGL 2 (legacy, not recommended today)")
option(IMGUI_BACKEND_OPENGL3 "OpenGL 3/4")
option(IMGUI_BACKEND_OSX "macOS native API")
option(IMGUI_BACKEND_SDL "SDL2 (Windows, macOS, Linux, iOS, Android)")
option(IMGUI_BACKEND_SDL_RENDERER "SDL2 renderer")
option(IMGUI_BACKEND_VULKAN "Vulkan")
option(IMGUI_BACKEND_WGPU "WebGPU")
option(IMGUI_BACKEND_WINAPI "Win32 native API")

add_library(${PROJECT_NAME}
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    $<$<BOOL:${IMGUI_BACKEND_ALLEGRO5}>:backends/imgui_impl_allegro5.cpp>
    $<$<BOOL:${IMGUI_BACKEND_ANDROID}>:backends/imgui_impl_android.cpp>
    $<$<BOOL:${IMGUI_BACKEND_DX9}>:backends/imgui_impl_dx9.cpp>
    $<$<BOOL:${IMGUI_BACKEND_DX10}>:backends/imgui_impl_dx10.cpp>
    $<$<BOOL:${IMGUI_BACKEND_DX11}>:backends/imgui_impl_dx11.cpp>
    $<$<BOOL:${IMGUI_BACKEND_DX12}>:backends/imgui_impl_dx12.cpp>
    $<$<BOOL:${IMGUI_BACKEND_GLFW}>:backends/imgui_impl_glfw.cpp>
    $<$<BOOL:${IMGUI_BACKEND_GLUT}>:backends/imgui_impl_glut.cpp>
    $<$<BOOL:${IMGUI_BACKEND_MARMALADE}>:backends/imgui_impl_marmalade.cpp>
    $<$<BOOL:${IMGUI_BACKEND_METAL}>:backends/imgui_impl_metal.mm>
    $<$<BOOL:${IMGUI_BACKEND_OPENGL2}>:backends/imgui_impl_opengl2.cpp>
    $<$<BOOL:${IMGUI_BACKEND_OPENGL3}>:backends/imgui_impl_opengl3.cpp>
    $<$<BOOL:${IMGUI_BACKEND_OSX}>:backends/imgui_impl_osx.mm>
    $<$<BOOL:${IMGUI_BACKEND_SDL}>:backends/imgui_impl_sdl.cpp>
    $<$<BOOL:${IMGUI_BACKEND_SDL_RENDERER}>:backends/imgui_impl_sdlrenderer.cpp>
    $<$<BOOL:${IMGUI_BACKEND_VULKAN}>:backends/imgui_impl_vulkan.cpp>
    $<$<BOOL:${IMGUI_BACKEND_WGPU}>:backends/imgui_impl_wgpu.cpp>
    $<$<BOOL:${IMGUI_BACKEND_WINAPI}>:backends/imgui_impl_win32.cpp>
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        .
        backends
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${IMGUI_BACKEND_ANDROID}>:android>
        $<$<BOOL:${IMGUI_BACKEND_DX9}>:d3d9.lib>
        $<$<BOOL:${IMGUI_BACKEND_DX10}>:d3d10.lib;d3dcompiler.lib>
        $<$<BOOL:${IMGUI_BACKEND_DX11}>:d3d11.lib;d3dcompiler.lib>
        $<$<BOOL:${IMGUI_BACKEND_DX12}>:d3d12.lib;d3dcompiler.lib;dxgi.lib>
        $<$<BOOL:${IMGUI_BACKEND_GLFW}>:glfw>
        $<$<BOOL:${IMGUI_BACKEND_GLUT}>:glut>
        $<$<BOOL:${IMGUI_BACKEND_METAL}>:"-framework Metal -framework MetalKit -framework QuartzCore">
        $<$<BOOL:${IMGUI_BACKEND_OSX}>:"-framework Cocoa">
        $<$<OR:$<BOOL:${IMGUI_BACKEND_SDL}>,$<BOOL:${IMGUI_BACKEND_SDL_RENDERER}>>:SDL2::SDL2main>
        $<$<OR:$<BOOL:${IMGUI_BACKEND_SDL}>,$<BOOL:${IMGUI_BACKEND_SDL_RENDERER}>>:$<IF:$<BOOL:${SDL_STATIC_ENABLED_BY_DEFAULT}>,SDL2::SDL2-static,SDL2::SDL2>>
        $<$<BOOL:${IMGUI_BACKEND_VULKAN}>:Vulkan::Vulkan>
        $<$<AND:$<BOOL:${IMGUI_BACKEND_OPENGL3}>,$<BOOL:${OpenGL_FOUND}>>:OpenGL::OpenGL>
        $<$<AND:$<BOOL:${IMGUI_BACKEND_OPENGL2}>,$<BOOL:${OpenGL_FOUND}>>:OpenGL::OpenGL>
)

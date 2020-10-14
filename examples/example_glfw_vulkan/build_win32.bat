@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

mkdir Debug
cl /nologo /Zi /MD /I .. /I ..\.. /I ..\libs\glfw\include /I %VULKAN_SDK%\include *.cpp ..\..\backends\imgui_impl_vulkan.cpp ..\..\backends\imgui_impl_glfw.cpp ..\..\imgui*.cpp /FeDebug/example_glfw_vulkan.exe /FoDebug/ /link /LIBPATH:..\libs\glfw\lib-vc2010-32 /libpath:%VULKAN_SDK%\lib32 glfw3.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib

mkdir Release
cl /nologo /Zi /MD /Ox /Oi /I .. /I ..\.. /I ..\libs\glfw\include /I %VULKAN_SDK%\include *.cpp ..\..\backends\imgui_impl_vulkan.cpp ..\..\backends\imgui_impl_glfw.cpp ..\..\imgui*.cpp /FeRelease/example_glfw_vulkan.exe /FoRelease/ /link /LIBPATH:..\libs\glfw\lib-vc2010-32 /libpath:%VULKAN_SDK%\lib32 glfw3.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib

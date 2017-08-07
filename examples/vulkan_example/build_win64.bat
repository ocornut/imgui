@REM Build for Visual Studio compiler. Run your copy of amd64/vcvars32.bat to setup 64-bit command-line compiler.

mkdir Debug
cl /nologo /Zi /MD /I ..\.. /I ..\libs\glfw\include /I %VULKAN_SDK%\include *.cpp ..\..\*.cpp /FeDebug/vulkan_example.exe /FoDebug/ /link /LIBPATH:..\libs\glfw\lib-vc2010-64 /libpath:%VULKAN_SDK%\lib glfw3.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib

mkdir Release
cl /nologo /Zi /MD /Ox /Oi /I ..\.. /I ..\libs\glfw\include /I %VULKAN_SDK%\include *.cpp ..\..\*.cpp /FeRelease/vulkan_example.exe /FoRelease/ /link /LIBPATH:..\libs\glfw\lib-vc2010-64 /libpath:%VULKAN_SDK%\lib glfw3.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib

@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I .. /I ..\.. /I ..\libs\glfw\include *.cpp ..\imgui_impl_opengl2.cpp ..\imgui_impl_glfw.cpp ..\..\imgui*.cpp /FeDebug/example_glfw_opengl2.exe /FoDebug/ /link /LIBPATH:..\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib

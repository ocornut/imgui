@REM Build for Visual Studio compiler
mkdir Debug
cl /nologo /MD /I ..\.. -I ..\libs\glfw\include *.cpp ..\..\*.cpp /FeDebug/opengl_example.exe /FoDebug/ /link /LIBPATH:..\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib

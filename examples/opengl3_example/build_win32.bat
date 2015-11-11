@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I ..\.. -I ..\libs\glfw\include -I ..\libs\gl3w *.cpp ..\..\*.cpp ..\libs\gl3w\GL\gl3w.c /FeDebug/opengl_example3.exe /FoDebug/ /link /LIBPATH:..\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib

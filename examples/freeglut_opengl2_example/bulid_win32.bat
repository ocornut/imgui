
@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /EHsc /nologo /Zi /MD /I ..\.. /I ..\libs\freeglut3.0.0\include\GL *.cpp ..\..\*.cpp /FeDebug/opengl2_example.exe /FoDebug/ /link /LIBPATH:..\libs\freeglut3.0.0\lib freeglut.lib opengl32.lib gdi32.lib shell32.lib
@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I ..\.. %* *.cpp ..\..\*.cpp /FeDebug/example_null.exe /FoDebug/ /link gdi32.lib shell32.lib

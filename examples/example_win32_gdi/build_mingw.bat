@REM Build for MINGW64 or 32 from MSYS2.
@set OUT_DIR=Debug
@set OUT_EXE=example_win32_gdi
@set INCLUDES=-I../.. -I../../backends
@set SOURCES=main.cpp ../../backends/imgui_impl_gdi.cpp ../../backends/imgui_impl_win32.cpp ../../imgui*.cpp
@set LIBS=-lgdi32 -ldwmapi
mkdir %OUT_DIR%
g++ -DUNICODE %INCLUDES% %SOURCES% -o %OUT_DIR%/%OUT_EXE%.exe --static -mwindows %LIBS% %LIBS%

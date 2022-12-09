rmdir /S /Q build

cmake -S . -B build/ -G "Visual Studio 16 2019" -A x64
cmake --build build/ --config Debug -v
cmake --build build/ --config Release -v

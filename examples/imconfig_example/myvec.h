#pragma once

struct MyVec2
{
    float x, y;
    MyVec2() { x = y = 0.0f; }
    MyVec2(float _x, float _y) { x = _x; y = _y; }
};

struct MyVec4
{
    float x, y, z, w;
    MyVec4() { x = y = z = w = 0.0f; }
    MyVec4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
};
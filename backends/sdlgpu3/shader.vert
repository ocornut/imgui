#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

layout(set=1,binding=0) uniform UBO
{
    vec2 uScale;
    vec2 uTranslate;
} ubo;

layout(location = 0) out struct
{
    vec4 Color;
    vec2 UV;
} Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * ubo.uScale + ubo.uTranslate, 0, 1);
    gl_Position.y *= -1.0f;
}

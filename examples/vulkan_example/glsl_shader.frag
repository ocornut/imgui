#version 450 core
layout(location = 0, index = 0) out vec4 fColor;

layout(set=0, binding=0) uniform sampler2D sTexture;

in block{
    vec4 Color;
    vec2 UV;
} In;

void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}

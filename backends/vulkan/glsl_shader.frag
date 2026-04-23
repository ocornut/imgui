#version 450 core
layout(location = 0) out vec4 fColor;

layout(set=0, binding=0) uniform texture2D _Texture;
layout(set=1, binding=0) uniform sampler _Sampler;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

void main()
{
    fColor = In.Color * texture(sampler2D(_Texture, _Sampler), In.UV.st);
}

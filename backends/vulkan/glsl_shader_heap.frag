#version 460 core
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require
layout(location = 0) out vec4 fColor;

layout(descriptor_heap) uniform texture2D _Textures[];
layout(descriptor_heap) uniform sampler _Samplers[];

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; uvec2 uID; } pc;
void main()
{
    fColor = In.Color * texture(sampler2D(_Textures[pc.uID.x], _Samplers[pc.uID.y]), In.UV.st);
}

#version 450 core

#ifndef USE_SPEC_CONSTANT_PARAMS
#define USE_SPEC_CONSTANT_PARAMS 0
#endif

layout(location = 0) out vec4 fColor;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

layout(constant_id = 0) const int color_correction_method = 0;
#if USE_SPEC_CONSTANT_PARAMS
layout(constant_id = 1) const float color_correction_param1 = 1.0f;
layout(constant_id = 2) const float color_correction_param2 = 1.0f;
layout(constant_id = 3) const float color_correction_param3 = 1.0f;
layout(constant_id = 4) const float color_correction_param4 = 1.0f;
#else
layout(push_constant) uniform uPushConstant {
    layout(offset = 16 + 4 * 0) float color_correction_param1;
    layout(offset = 16 + 4 * 1) float color_correction_param2;
    layout(offset = 16 + 4 * 2) float color_correction_param3;
    layout(offset = 16 + 4 * 3) float color_correction_param4;
};
#endif

vec4 ApplyColorCorrection(vec4 src)
{
    vec4 res = src;
    if(color_correction_method == 1 || color_correction_method == 2)
    {
        const float gamma = color_correction_param1;
        const float exposure = color_correction_param2;
        res.rgb = exposure * pow(src.rgb, gamma.xxx);
        if(color_correction_method == 2)
        {
            const float alpha_gamma = color_correction_param3;
            res.a = pow(src.a, alpha_gamma);
        }
    }
    return res;
}

void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
    if(color_correction_method != 0)
    {
        fColor = ApplyColorCorrection(fColor);
    }
}

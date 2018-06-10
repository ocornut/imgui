//
//  Shader.vsh
//  imguiex

attribute vec4 position;
attribute vec3 normal;

varying lowp vec4 colorVarying;

uniform vec3 diffuseColor;
uniform mat4 modelViewProjectionMatrix;
uniform mat3 normalMatrix;

void main()
{
    vec3 eyeNormal = normalize(normalMatrix * normal);
    vec3 lightPosition = vec3(0.0, 0.0, 1.0);
    
    float nDotVP = max(0.0, dot(eyeNormal, normalize(lightPosition)));
    
    vec3 colorLit = diffuseColor * nDotVP;
    colorVarying = vec4( colorLit.x, colorLit.y, colorLit.z, 1.0 );
    
    gl_Position = modelViewProjectionMatrix * position;
}

//
//  Shader.fsh
//  imguiex

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}

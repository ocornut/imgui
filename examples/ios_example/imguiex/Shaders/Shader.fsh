//
//  Shader.fsh
//  imguiex
//
//  Created by Joel Davis on 6/14/15.
//  Copyright (c) 2015 Joel Davis. All rights reserved.
//

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}

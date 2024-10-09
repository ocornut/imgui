#!/bin/bash
## -V: create SPIR-V binary
## -x: save binary output as text-based 32-bit hexadecimal numbers
## -o: output file
glslangValidator -V -x -DUSE_SPEC_CONSTANT_PARAMS=1 -o glsl_shader_static.frag.u32  glsl_shader.frag
glslangValidator -V -x -DUSE_SPEC_CONSTANT_PARAMS=0 -o glsl_shader_dynamic.frag.u32 glsl_shader.frag
glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert

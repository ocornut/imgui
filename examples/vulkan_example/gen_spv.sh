#!/bin/bash
glslangValidator -V -o glsl_shader.frag.spv glsl_shader.frag
glslangValidator -V -o glsl_shader.vert.spv glsl_shader.vert
spirv-remap --map all --dce all --strip-all --input glsl_shader.frag.spv glsl_shader.vert.spv --output ./

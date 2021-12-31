/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* VERTEX SHADER ** STAGE 1
   -- vertex preparation-- non tesselation -- pass though from given array 
   https://learnopengl.com/#!Advanced-OpenGL/Advanced-GLSL
*/

#include "g3d-allshader-uniforms.glsl"

#define POSITION		0
#define TEXCOORD		1

layout(std140, column_major) uniform;

layout(location = POSITION) in vec4 position;
layout(location = TEXCOORD) in vec2 texcoord;

out block
{
        vec3 Vertex;
        vec3 VertexEye;
        vec2 TexCoord;
} Out;

float height_transform(float y)
{
        return height_scale * (y-0.5) + height_offset;  
}

void main(void) {
        vec4 pos = vec4(position.x,  position.y, position.z, 1.);
        Out.Vertex    = vec3 (pos.xyz);
        Out.VertexEye = vec3 (ModelView * pos);  // eye space
        Out.TexCoord = texcoord;
        gl_Position = ModelViewProjection * pos;
}

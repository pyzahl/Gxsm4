/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* FRAGMENT SHADER ++ NON TESS ** STAGE2
   -- color/light computation / Lambertian Reflection (Ambient+Diffuse+Specular)
*/

#include "g3d-allshader-uniforms.glsl"

layout(std140, column_major) uniform;

in block
{
        vec3 Vertex;
        vec3 VertexEye;
        vec2 TexCoord;
} In;

out vec4 FragColor;

void main(void) {
        FragColor = vec4(1, 1, 1, texture2D(textTexture, In.TexCoord).r) * textColor;
}

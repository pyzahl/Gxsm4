/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* Ico-Vertex ** 1 */

#include "g3d-allshader-uniforms.glsl"

#define POSITION		0
#define LATTICE                 1

layout(std140, column_major) uniform;

layout(location = POSITION) in vec4 Position;
layout(location = LATTICE)  in vec4 LatPos;

out block
{
        vec3 Position;
        vec3 LatPos;
} Out;

uniform vec4 IcoPositionS;

void main()
{
    Out.Position = Position.xyz;
    Out.LatPos = LatPos.xyz;
}

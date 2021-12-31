/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* Ico-TessControl stage 2 */

#include "g3d-allshader-uniforms.glsl"

#define ID gl_InvocationID

layout(vertices = 3) out;

uniform float TessLevelInner;
uniform float TessLevelOuter;

in block
{
        vec3 Position;
        vec3 LatPos;
} In[];

out block
{
        vec3 Position;
        vec3 LatPos;
} Out[];

void main()
{
    Out[ID].Position = In[ID].Position;
    Out[ID].LatPos   = In[ID].LatPos;
    if (ID == 0) {
        gl_TessLevelInner[0] = TessLevelInner;
        gl_TessLevelOuter[0] = TessLevelOuter;
        gl_TessLevelOuter[1] = TessLevelOuter;
        gl_TessLevelOuter[2] = TessLevelOuter;
    }
}

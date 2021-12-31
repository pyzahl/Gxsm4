/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* Ico-TessEval 3 */

#include "g3d-allshader-uniforms.glsl"

layout(triangles, equal_spacing, cw) in;

uniform vec4 IcoPosition;
uniform vec4 IcoScale;

in block
{
        vec3 Position;
        vec3 LatPos;
} In[];

out block
{
        vec3 Position;
        vec3 PatchDistance;
        vec3 VertexEye;
        vec3 LatPos;
} Out;

float height_transform(float y)
{
        return height_scale * y + height_offset;  
}


void main()
{
        vec3 p0 = gl_TessCoord.x * In[0].Position;
        vec3 p1 = gl_TessCoord.y * In[1].Position;
        vec3 p2 = gl_TessCoord.z * In[2].Position;
        Out.LatPos = In[0].LatPos;
        Out.PatchDistance = gl_TessCoord;
        Out.Position =  IcoPosition.xyz + IcoScale.xyz * (In[0].LatPos + normalize(p0 + p1 + p2));
        Out.Position.y = height_transform (Out.Position.y);
        Out.Position *= aspect.xzy;
        Out.VertexEye   = (ModelView * vec4(Out.Position, 1)).xyz;  // eye space
        gl_Position = ModelViewProjection * vec4(Out.Position, 1);
}

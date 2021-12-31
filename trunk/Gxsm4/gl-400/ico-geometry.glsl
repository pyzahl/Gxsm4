/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* Ico-Geometry  4 */

#include "g3d-allshader-uniforms.glsl"

layout(std140, column_major) uniform;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in block
{
        vec3 Position;
        vec3 PatchDistance;
        vec3 VertexEye;
        vec3 LatPos;
} In[3];

out block
{
        vec3 Position;
        vec3 FacetNormal;
        vec3 PatchDistance;
        vec3 TriDistance;
        vec3 VertexEye;
        vec3 LatPos;
} Out;


void main()
{
    vec3 A = In[2].Position - In[0].Position;
    vec3 B = In[1].Position - In[0].Position;
    Out.FacetNormal = (ModelViewProjection * vec4(normalize(cross(A, B)),1)).xyz;
    
    Out.VertexEye = In[0].VertexEye;
    Out.Position = gl_in[0].gl_Position.xyz;
    Out.PatchDistance = In[0].PatchDistance;
    Out.TriDistance = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position;
    Out.LatPos = In[0].LatPos;
    EmitVertex();

    Out.VertexEye = In[1].VertexEye;
    Out.Position = gl_in[1].gl_Position.xyz;
    Out.PatchDistance = In[1].PatchDistance;
    Out.TriDistance = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position;
    Out.LatPos = In[1].LatPos;
    EmitVertex();

    Out.VertexEye = In[2].VertexEye;
    Out.Position = gl_in[2].gl_Position.xyz;
    Out.PatchDistance = In[2].PatchDistance;
    Out.TriDistance = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position;
    Out.LatPos = In[2].LatPos;
    EmitVertex();

    EndPrimitive();
}

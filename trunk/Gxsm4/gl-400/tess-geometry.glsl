/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* GEOMETRY SHADER ** STAGE 4
   -- Geometry manipulation, instancing, etc. -- pass through here 
*/

#include "g3d-allshader-uniforms.glsl"

layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 4) out;

in block {
        vec3 Vertex;
        vec3 VertexEye;
        vec3 Normal;
        vec4 Color;
} In[];

out block {
        vec3 Vertex;
        vec3 VertexEye;
        vec3 Normal;
        vec4 Color;
} Out;

void main()
{	
	for(int i = 0; i < gl_in.length(); ++i)
	{
		gl_Position = gl_in[i].gl_Position;
		Out.Vertex = In[i].Vertex;
		Out.Normal = In[i].Normal;
		Out.Color = In[i].Color;
		EmitVertex();
	}
	EndPrimitive();
}


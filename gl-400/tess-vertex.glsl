/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* VERTEX SHADER ** STAGE 1
   -- vertex preparation -- pass though from given array 
*/

#include "g3d-allshader-uniforms.glsl"

#define POSITION		0

layout(location = POSITION) in vec2 PositionXZ;  // XZ in GL is XY in plane

out block
{
        vec3 Vertex;
} Out;



float height_transform(float y)
{
        return height_scale * (y-cCenter.y) + height_offset;  
}

vec2 terraincoord(vec2 position){
        return vec2 (cCenter.x - position.x, -(cCenter.y - (-position.y)/aspect.y));
}

float vertex_height_flat(vec2 position)
{
        return height_transform (cCenter.z);
}

float vertex_height_direct(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).a);
}

float vertex_height_simple_z(vec2 position)
{
        vec4 zz = texture (Surf3D_Z_Data, terraincoord(position));
        return height_transform (zz.a);
}

float vertex_height_x(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).x);
}

float vertex_height_y(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).y);
}

float vertex_height_z(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).z);
}

float vertex_plane_at(vec2 position)
{
        return height_transform (plane_at);
}

float vertex_height(vec2 position)
{
        switch (vertexModel){
        case HEIGHT_FLAT:     return vertex_height_flat (position);
        case HEIGHT_SIMPLE_Z: return vertex_height_simple_z (position);
        case HEIGHT_VIEW_Z:   return vertex_height_z (position);
        case HEIGHT_Y:        return vertex_height_y (position);
        case HEIGHT_X:        return vertex_height_x (position);
        case HEIGHT_PLANE_AT: return vertex_plane_at (position);
        default:              return vertex_height_direct (position);
        }
}

void main()
{
        Out.Vertex = vec3 (PositionXZ.x, vertex_height (PositionXZ), -PositionXZ.y);
        gl_Position = vec4 (Out.Vertex, 1.0);
}

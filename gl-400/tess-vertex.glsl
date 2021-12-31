/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* VERTEX SHADER ** STAGE 1
   -- vertex preparation -- pass though from given array 
*/

#include "g3d-allshader-uniforms.glsl"

#define POSITION		0

layout(std140, column_major) uniform;

layout(location = POSITION) in vec2 PositionXZ;  // XZ in GL is XY in plane

out block
{
        vec3 Vertex;
} Out;


subroutine float vertexModelType(vec2 position);
subroutine uniform vertexModelType vertexModel;

float height_transform(float y)
{
        return height_scale * (y-cCenter.y) + height_offset;  
}

vec2 terraincoord(vec2 position){
        return vec2 (cCenter.x - position.x, -(cCenter.y - (-position.y)/aspect.y));
}

subroutine( vertexModelType )
float vertex_height_flat(vec2 position)
{
        return height_transform (cCenter.z);
}

subroutine( vertexModelType )
float vertex_height_direct(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).a);
}

subroutine( vertexModelType )
float vertex_height_x(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).x);
}

subroutine( vertexModelType )
float vertex_height_y(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).y);
}

subroutine( vertexModelType )
float vertex_height_z(vec2 position)
{
        return height_transform (texture (Surf3D_Z_Data, terraincoord(position)).z);
}

subroutine( vertexModelType )
float vertex_plane_at(vec2 position)
{
        return height_transform (plane_at);
}

void main()
{
        Out.Vertex = vec3 (PositionXZ.x, vertexModel (PositionXZ), -PositionXZ.y);
        gl_Position = vec4 (Out.Vertex, 1.0);
}

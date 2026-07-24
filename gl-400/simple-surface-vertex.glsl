/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* SIMPLE SURFACE VERTEX SHADER */

#include "g3d-allshader-uniforms.glsl"

#define POSITION 0

layout(location = POSITION) in vec2 PositionXZ;

out vec3 vColor;

float height_transform(float y)
{
        return height_scale * (y-cCenter.y) + height_offset;
}

vec2 terraincoord(vec2 position){
        return vec2 (cCenter.x - position.x, -(cCenter.y - (-position.y)/aspect.y));
}

void main()
{
        vec4 zz = texture(Surf3D_Z_Data, terraincoord(PositionXZ));
        float height = height_transform(zz.a);
        vec4 pos = vec4(PositionXZ.x, height, -PositionXZ.y, 1.0);

        gl_Position = ModelViewProjection * pos;
        vColor = vec3(0.25 + 0.5 * clamp(zz.r, 0.0, 1.0),
                      0.25 + 0.5 * clamp(zz.g, 0.0, 1.0),
                      0.35 + 0.45 * clamp(zz.b, 0.0, 1.0));
}

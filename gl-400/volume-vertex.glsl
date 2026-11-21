/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* VOLUME RAYCAST VERTEX SHADER
   -- one full screen triangle, no vertex buffer (gl_VertexID 0,1,2).
      Every pixel runs the ray marcher in volume-fragment.glsl exactly once,
      independent of view direction and also with the camera inside the volume.
*/

#include "g3d-allshader-uniforms.glsl"

out vec2 NDC;

void main()
{
        // (-1,-1), (3,-1), (-1,3): covers the whole [-1,1]^2 viewport
        vec2 p = vec2 (float ((gl_VertexID & 1) << 2) - 1.0,
                       float ((gl_VertexID & 2) << 1) - 1.0);
        NDC = p;
        gl_Position = vec4 (p, 0.0, 1.0);
}

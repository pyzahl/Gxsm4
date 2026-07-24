/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* SIMPLE SURFACE FRAGMENT SHADER */

#include "g3d-allshader-uniforms.glsl"

in vec3 vColor;
out vec4 FragColor;

void main()
{
        FragColor = vec4(vColor, 1.0);
}

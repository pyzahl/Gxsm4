/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/*
  GLOBAL INCLUDE FOR ALL SHADERS
  DEFINES DEFAULT SETTINGS AND SHARED UNIFORMS
*/

#version 400 core

precision highp float;
precision highp int;
layout(std140, column_major) uniform;

// Model View and Projection Matrices
layout (std140) uniform ModelViewMatrices
{
        uniform mat4 ModelView;
        uniform mat4 ViewAlignedModelView;
        uniform mat4 ModelViewProjection;
};

// Geometry
layout (std140) uniform SurfaceGeometry
{
        uniform vec4  aspect;
        uniform float height_scale;
        uniform float height_offset;
        uniform float plane_at;
        uniform vec4  cCenter;
        uniform vec2  delta; // must align 4f
};

// Light and Shading
layout (std140) uniform FragmentShading
{
        uniform vec4 lightDirWorld;
        uniform vec4 eyePosWorld;

        uniform vec4 sunColor; // = vec3(1.0, 1.0, 0.7);
        uniform vec4 specularColor; // = vec3(1.0, 1.0, 0.7)*1.5;
        uniform vec4 ambientColor; // = vec3(1.0, 1.0, 0.7)*1.5;
        uniform vec4 diffuseColor; // = vec3(1.0, 1.0, 0.7)*1.5;
        uniform vec4 fogColor; // = vec3(0.7, 0.8, 1.0)*0.7;
        uniform vec4 materialColor;
        uniform vec4 backsideColor;
        uniform vec4 color_offset;

        uniform float fogExp; // = 0.1;

        uniform float shininess; // = 100.0;
        uniform float lightness;
        uniform float light_attenuation;

        uniform float transparency;
        uniform float transparency_offset;
        
        uniform float wrap;
        uniform uint debug_color_source;     // only for debug shader
};


// Sampler2D for GXSM Surface_Z_Data : vec4[][]
uniform sampler2D Surf3D_Z_Data;
uniform sampler1D GXSM_Palette;
uniform sampler3D Volume3D_Z_Data;

// text shading
uniform sampler2D textTexture;
uniform vec4 textColor;

// Tip
uniform vec4 TipPosition;
uniform vec4 TipColor;
































/* END OF INCLUDE: KEEP THIS AT LINE 100 -- OFFSET FOR ERROR END GLSL INCLUDE */

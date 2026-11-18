/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/*
  GLOBAL INCLUDE FOR ALL SHADERS
  DEFINES DEFAULT SETTINGS AND SHARED UNIFORMS
*/

#version 400 core

/* Desktop GLSL: precision qualifiers are GLSL ES only; removed. */


// Model View and Projection Matrices
layout (std140) uniform ModelViewMatrices
{
        mat4 ModelView;
        mat4 ViewAlignedModelView;
        mat4 ModelViewProjection;
};

// Geometry
layout (std140) uniform SurfaceGeometry
{
        vec4  aspect;
        float height_scale;
        float height_offset;
        float plane_at;
        vec4  cCenter;
        vec2  delta; // must align 4f
};

// Light and Shading
layout (std140) uniform FragmentShading
{
        vec4 lightDirWorld;
        vec4 eyePosWorld;

        vec4 sunColor; // = vec3(1.0, 1.0, 0.7);
        vec4 specularColor; // = vec3(1.0, 1.0, 0.7)*1.5;
        vec4 ambientColor; // = vec3(1.0, 1.0, 0.7)*1.5;
        vec4 diffuseColor; // = vec3(1.0, 1.0, 0.7)*1.5;
        vec4 fogColor; // = vec3(0.7, 0.8, 1.0)*0.7;
        vec4 materialColor;
        vec4 backsideColor;
        vec4 color_offset;

        float fogExp; // = 0.1;

        float shininess; // = 100.0;
        float lightness;
        float light_attenuation;

        float transparency;
        float transparency_offset;
        
        float wrap;
        uint debug_color_source;     // only for debug shader
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

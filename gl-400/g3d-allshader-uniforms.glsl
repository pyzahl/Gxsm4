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

// Mode selectors (plain int uniforms, replacing GLSL subroutines for portability).
// KEEP IN SYNC with enum gl_shader_modes in core-source/vsurf3d.cpp
#define HEIGHT_FLAT      0
#define HEIGHT_DIRECT    1
#define HEIGHT_SIMPLE_Z  2
#define HEIGHT_VIEW_Z    3
#define HEIGHT_Y         4
#define HEIGHT_X         5
#define HEIGHT_PLANE_AT  6
#define COLOR_FLAT       0
#define COLOR_DIRECT     1
#define COLOR_VIEW       2
#define COLOR_X          3
#define COLOR_Y          4
#define PLANE_XZ         0
#define PLANE_XY         1
#define PLANE_ZY         2
#define PLANE_M          3
#define SHADE_LAMBERTIAN           0
#define SHADE_TERRAIN              1
#define SHADE_MATERIAL_COLOR       2
#define SHADE_MATERIAL_COLOR_FOG   3
#define SHADE_LAMBERTIAN_XCOLOR    4
#define SHADE_VOLUME               5
#define SHADE_DEBUG                6
uniform int vertexModel;      // HEIGHT_*  (vertex stage)
uniform int heightModel;      // HEIGHT_*  (tess evaluation stage)
uniform int colorModel;       // COLOR_*
uniform int planeModel;       // PLANE_*
uniform int shadeModel;       // SHADE_*

/* END OF INCLUDE: KEEP THIS AT LINE 100 -- OFFSET FOR ERROR END GLSL INCLUDE */

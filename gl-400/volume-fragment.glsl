/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* VOLUME RAYCAST FRAGMENT SHADER
   -- GPU ray marching through the 3D texture Volume3D_Z_Data with a transfer function:
      value  v = view mode data (.z channel, 0..1)
      color    = GXSM palette (ColorOffset, ColorSat)       -- as surface / slice volume
      opacity  a = clamp (TransparencyOffset + Transparency * v, 0, 1)
      extinction per model unit length = VolumeDensity * a  (Beer-Lambert, step size independent)
   Modes: 0 = front-to-back emission/absorption compositing, 1 = maximum intensity projection (MIP)
   Output is premultiplied alpha: blend with glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
*/

#include "g3d-allshader-uniforms.glsl"

in vec2 NDC;
out vec4 FragColor;

uniform mat4  InvModelViewProjection; // NDC -> model space
uniform vec3  VolumeHalf;             // half extents of the full volume box in model space (GL x, y=up, z)
uniform vec3  VolumeLo;               // clip box (SliceLimiter start/stop on the layer axis), model space
uniform vec3  VolumeHi;
uniform vec2  VolumeLayer;            // layer axis: layer fraction f = VolumeLayer.x + y * VolumeLayer.y
                                      // (start layer at y = 0, Hskl = selected layer range height in scan widths)
uniform int   VolumeMode;             // 0: composite, 1: MIP
uniform float VolumeSteps;            // samples along the box diagonal
uniform float VolumeDensity;          // optical depth per model unit at opacity 1

#define VOLUME_COMPOSITE 0
#define VOLUME_MIP       1

vec3 VolN; // 3D texture size (nx, ny, nv), set in main()

// model space position -> 3D texture coordinate.
// u: fractions 0..1 of the index ranges (x, z orientation as the surface / shadeVolume),
// mapped onto texel CENTERS: 0 -> first pixel/layer, 1 -> last pixel/layer. Mapping 0..1 to the
// texture edges would blend half a voxel with the border color at all faces (fading first/last layer).
vec3 volume_texcoord (vec3 p)
{
        vec3 u = vec3 (0.5 - p.x/aspect.x, 0.5 - p.z/aspect.y, VolumeLayer.x + p.y*VolumeLayer.y);
        return (u*(VolN - 1.0) + 0.5) / VolN;
}

float volume_value (vec3 p)
{
        return clamp (textureLod (Volume3D_Z_Data, volume_texcoord (p), 0.0).z, 0.0, 1.0);
}

vec3 volume_color (float v)
{
        return lightness * textureLod (GXSM_Palette, clamp (color_offset.x + color_offset.y*v, 0.0, 1.0), 0.0).rgb;
}

float volume_opacity (float v)
{
        return clamp (transparency_offset + transparency*v, 0.0, 1.0);
}

// per pixel hash in [0,1) to jitter the ray start (removes slice-like banding)
float hash12 (vec2 p)
{
        vec3 p3 = fract (vec3 (p.xyx) * 0.1031);
        p3 += dot (p3, p3.yzx + 33.33);
        return fract ((p3.x + p3.y) * p3.z);
}

void main()
{
        VolN = vec3 (textureSize (Volume3D_Z_Data, 0));

        // ray from near to far plane through this pixel, in model space (works for perspective and ortho)
        vec4 pn = InvModelViewProjection * vec4 (NDC, -1.0, 1.0);
        vec4 pf = InvModelViewProjection * vec4 (NDC,  1.0, 1.0);
        vec3 ro = pn.xyz / pn.w;
        vec3 rd = pf.xyz / pf.w - ro;   // t in [0,1] spans near..far

        // ray / box intersection (slab method), avoid division by 0
        vec3 rds = vec3 (abs (rd.x) < 1e-12 ? 1e-12 : rd.x,
                         abs (rd.y) < 1e-12 ? 1e-12 : rd.y,
                         abs (rd.z) < 1e-12 ? 1e-12 : rd.z);
        vec3 t0 = (VolumeLo - ro) / rds;
        vec3 t1 = (VolumeHi - ro) / rds;
        vec3 tlo = min (t0, t1);
        vec3 thi = max (t0, t1);
        float tmin = max (max (max (tlo.x, tlo.y), tlo.z), 0.0);
        float tmax = min (min (min (thi.x, thi.y), thi.z), 1.0);
        if (tmax <= tmin)
                discard;

        // fixed step in model units: full box diagonal / VolumeSteps (independent of clipping)
        float rlen  = length (rd);
        float ds    = 2.0*length (VolumeHalf) / max (VolumeSteps, 1.0);
        float dt    = ds / rlen;
        int   n     = int (ceil ((tmax - tmin) / dt));
        float t     = tmin + dt * hash12 (gl_FragCoord.xy);

        if (VolumeMode == VOLUME_MIP){
                float vmax = 0.0;
                for (int i=0; i<n && t < tmax; ++i, t += dt)
                        vmax = max (vmax, volume_value (ro + t*rd));
                float a = volume_opacity (vmax);
                if (a <= 0.0)
                        discard;
                FragColor = vec4 (a*volume_color (vmax), a);
                return;
        }

        // front-to-back emission / absorption compositing
        vec3  C = vec3 (0.0);
        float A = 0.0;
        for (int i=0; i<n && t < tmax; ++i, t += dt){
                float v = volume_value (ro + t*rd);
                float a = 1.0 - exp (-VolumeDensity * volume_opacity (v) * ds); // opacity of this step
                C += (1.0 - A) * a * volume_color (v);
                A += (1.0 - A) * a;
                if (A > 0.995)
                        break; // early ray termination
        }
        if (A <= 0.0)
                discard;
        FragColor = vec4 (C, A);
}

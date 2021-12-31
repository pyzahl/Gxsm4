/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* Ico-Fragment 5 */

#include "g3d-allshader-uniforms.glsl"

layout(std140, column_major) uniform;

in block
{
        vec3 Position;
        vec3 FacetNormal;
        vec3 PatchDistance;
        vec3 TriDistance;
        vec3 VertexEye;
        vec3 LatPos;
} In;

out vec4 FragColor;

in float gPrimitive;

uniform vec4 IcoColor[4];

vec4 shadeLambertian(vec3 vertex, vec3 vertexEye, 
                     vec3 normal, vec4 color)
{
        // world-space
        vec3 viewDir = normalize(eyePosWorld.xyz - vertex);
        vec3 h = normalize(-lightDirWorld.xyz + viewDir);
        vec3 n = normalize(normal);

        //float diffuse = saturate( dot(n, -lightDir));
        float diffuse = clamp( (dot(n, -lightDirWorld.xyz) + wrap) / (1.0 + wrap), 0., 1.);   // wrap
        //float diffuse = dot(n, -lightDir)*0.5+0.5;
        float specular = pow( clamp (dot(h, n), 0.,1.), shininess);

        vec3 finalColor = (ambientColor.xyz+specular*specularColor.xyz+diffuse*diffuseColor.xyz)*color.xyz;

#if 1
        return vec4 (lightness*finalColor, 1.);
#else
        vec3 SpecularColor = specular*specularColor.xyz;
        vec3 DiffuseColor  = diffuse*diffuseColor.xyz;
        vec3 LambertianColor = ambientColor.xyz + SpecularColor + DiffuseColor;
        vec3 finalColor = LambertianColor * color.xyz;

        switch (debug_color_source){
        case 1: return vec4 (lightness*DiffuseColor, 1.);
        case 2: return vec4 (lightness*SpecularColor, 1.);
        case 3: return vec4 (lightness*LambertianColor, 1.);
        case 4: return lightness*color;
        //case 5: finalColor = applyFog (finalColor, dist, viewDir); return vec4(lightness*finalColor, 1.);
        //case 6: return vec4 (mix (finalColor.xyz, nois, 0.2), 1.);
        //case 7: return vec4 (nois, 1.);
        case 8: return lightness*color;
        case 9: return color;
        case 10: return vec4(normal*0.5+0.5, 1.0);
        case 11: return vec4(normal.y*0.5+0.5, 0.,0., 1.0);
        case 12: return vec4(normal.x*0.5+0.5, 0.,0., 1.0);
        case 13: return specular*color;
        case 14: return vec4(vec3(specular,normal.y,diffuse),1.0);
        case 15: return vec4(vec3(dist/100.), 1.0);
        case 16: return specular*lightness*vec4(1);
        case 17: return vec4(vec3(lightness*(gl_FragCoord.z+transparency_offset)), 1.0f);
        case 18: return vec4(vec3(lightness*(gl_FragCoord.w+transparency_offset)), 1);
        case 19: return vec4(lightness*gl_FragCoord.w+vec4(transparency_offset));
        default: return vec4 (lightness*finalColor, 1.);
        }
#endif
        
}

void main()
{
        vec3 N = normalize(In.FacetNormal);
        int i=0;
        if (In.LatPos.y > 0.1)
                i=3;
        FragColor = shadeLambertian(In.Position, In.VertexEye, N, IcoColor[i]);
}

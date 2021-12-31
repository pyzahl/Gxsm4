/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/* FRAGMENT SHADER ** STAGE 5
   -- color/light computation / Lambertian Reflection (Ambient+Diffuse+Specular)
*/

#include "g3d-allshader-uniforms.glsl"

// WARNING in/out "blocks" are not part of namespace, members must be unique in code! Thumb Rule: use capital initials for In/Out.
in block {
        vec3 Vertex;
        vec3 VertexEye;
        vec3 Normal;
        vec4 Color;
} In;

out vec4 FragColor;


subroutine vec4 shadeModelType(vec3 vertex, vec3 vertexEye, 
                               vec3 normal, vec4 color);

subroutine uniform shadeModelType shadeModel;

float saturate(float v) {
    return clamp( v, 0.0, 1.0);
}

// cheaper than smoothstep
float linearstep(float a, float b, float x)
{
    return saturate((x - a) / (b - a));
}

#define smoothstep linearstep


// terrain defaults
const vec3 CsunColor = vec3(1.0, 1.0, 0.7);
const vec3 ClightColor = vec3(1.0, 1.0, 0.7)*1.5;
const vec3 CfogColor = vec3(0.7, 0.8, 1.0)*0.7;
const float CfogExp = 0.1;
#if 0
vec3 applyFog(vec3 col, float dist)
{
        float fogAmount = exp (-dist*fogExp);
        return mix (fogColor.xyz, col.xyz, fogAmount);
}
#endif
// fog with scattering effect
// http://www.iquilezles.org/www/articles/fog/fog.htm
vec3 applyFog(vec3 col, float dist, vec3 viewDir)
{
        float fogAmount = exp (-dist*fogExp);
        float sunAmount = max (dot(viewDir, lightDirWorld.xyz), 0.0);
        sunAmount = pow (sunAmount, 32.0);
        vec3 fogCol = mix (fogColor.xyz, sunColor.xyz, sunAmount);
        return mix (fogCol.xyz, col.xyz, fogAmount);
}


vec3 map_xyz_values(vec2 position)
{
        vec2 terraincoord = vec2 (cCenter.x - position.x, cCenter.y - position.y/aspect.y); // swap
        return texture (Surf3D_Z_Data, terraincoord).xyz;
}


// terrain level shader
subroutine( shadeModelType )
vec4 shadeTerrain(vec3 vertex, vec3 vertexEye, 
                  vec3 normal, vec4 color)
{
        const float Cshininess = 100.0;
        const vec3  CambientColor = vec3(0.05, 0.05, 0.15 );
        const float Cwrap = 0.3;
        
        vec3 rockColor = vec3(0.4, 0.4, 0.4 );
        vec3 snowColor = vec3(0.9, 0.9, 1.0 );
        vec3 grassColor = vec3(77.0 / 255.0, 100.0 / 255.0, 42.0 / 255.0 );
        vec3 brownColor = vec3(82.0 / 255.0, 70.0 / 255.0, 30.0 / 255.0 );
        vec3 waterColor = vec3(0.2, 0.4, 0.5 );
        vec3 treeColor = vec3(0.0, 0.2, 0.0 );

        //float height = vertex.y*100.-50.;
  
        //vec3 noisePos = vertex.xyz + vec3(translate.x, 0.0, translate.y);
        vec3 noisePos = vertex.xyz;
        float nois = length(noise2(noisePos.xz))*0.5+0.5;

        float height = vertex.y;

        // snow
        float snowLine = 0.7;
        //float snowLine = 0.6 + nois*0.1;
        float isSnow = smoothstep(snowLine, snowLine+0.1, height * (0.5+0.5*normal.y));

        // Lambertian light model

        // world-space
        vec3 viewDir = normalize(eyePosWorld.xyz - vertexEye);
        vec3 h = normalize(-lightDirWorld.xyz + viewDir);
        vec3 n = normalize(normal);

        //float diffuse = saturate( dot(n, -lightDir));
        float diffuse = saturate( (dot(n, -lightDirWorld.xyz) + Cwrap) / (1.0 + Cwrap));   // wrap
        //float diffuse = dot(n, -lightDir)*0.5+0.5;
        float specular = pow( saturate(dot(h, n)), shininess);

#if 1
        // add some noise variation to colors
        grassColor = mix(grassColor*0.5, grassColor*1.5, nois);
        brownColor = mix(brownColor*0.25, brownColor*1.5, nois);
#endif
        
        // choose material color based on height and normal

        vec3 matColor;
        matColor = mix(rockColor, grassColor, smoothstep(0.6, 0.8, normal.y));
        matColor = mix(matColor, brownColor, smoothstep(0.9, 1.0, normal.y ));
        // snow
        matColor = mix(matColor, snowColor, isSnow);

        float isWater = smoothstep(0.05, 0.0, height);
        matColor = mix(matColor, waterColor, isWater);

        vec3 finalColor = ambientColor.xyz*matColor + diffuse*matColor*specularColor.xyz + specular*specularColor.xyz*isWater;
        // fog
        //float dist = length(vertexEye);
        float dist = length(eyePosWorld.xyz);
        //finalColor = applyFog(finalColor, dist);
        finalColor = applyFog(finalColor, dist, viewDir);
        //return vec4(finalColor, 1.);

        return vec4 (lightness*finalColor, 1.0);

#if 0
        // debug stuff
        //return vec4(vec3(specular,normal.z,diffuse),1.0);
        //return vec4(specular*sunColor+diffuse*specularColor, 1.); // digitalisch
        //return vec4(ambientColor*matColor, 1.); // digitalisch
        //return vec4(diffuse*matColor*specularColor, 1.); // digitalisch
        //return vec4(vec3(height)*0.5+0.5, 1.);
        //return vec4(dist);
        //return vec4(dnoise(vertex.xz).z*0.5+0.5);
        //return vec4(normal*0.5+0.5, 1.0); //ok
        //return vec4(n*0.5+0.5, 1.0); //ok
        //return vec4(vec3(normal.z*0.5+0.5),1.); // ok
        //return vec4(fogCoord);
        //return vec4(vec3(diffuse),1.);
        //return vec4(vec3(specular), 1.);
        //return diffuse*matColor + specular.xxx
        //return matColor;
        //return vec4(occ);
        //return vec4(sun)*sunColor;
        //return noise2*0.5+0.5;
#endif
}

// vertex,... surface color
subroutine( shadeModelType )
vec4 shadeDebugMode(vec3 vertex, vec3 vertexEye, 
                    vec3 normal, vec4 color)
{
        // Lambertian light model

        // world-space

        // viewDir:  eyePosWork - vertex (for model rot invariant light) -- or  - vertexEye
        vec3 viewDir = normalize(eyePosWorld.xyz - vertexEye); 
        vec3 h = normalize(-lightDirWorld.xyz + viewDir);
        vec3 n = normalize(normal);

        //float diffuse = saturate( dot(n, -lightDir));
        float diffuse = saturate( (dot(n, -lightDirWorld.xyz) + wrap) / (1.0 + wrap));   // wrap
        //float diffuse = dot(n, -lightDir)*0.5+0.5;
        float specular = pow( saturate(dot(h, n)), shininess);

        vec3 SpecularColor = specular*specularColor.xyz;
        vec3 DiffuseColor  = diffuse*diffuseColor.xyz;
        vec3 LambertianColor = ambientColor.xyz + SpecularColor + DiffuseColor;

        vec3 nois = noise3(100.*vertex.xz)*0.5+vec3(0.5);
        float dist = length (eyePosWorld.xyz);

        // if spot light -- not for far far away sun
        //float dist_light = length (sunPos);
        //float attenuation = 1.0 / (1.0 + light_attenuation * dist_light*dist_light);

        vec3 finalColor = LambertianColor * color.xyz;

        switch (debug_color_source){
        case 1: return vec4 (lightness*DiffuseColor, 1.);
        case 2: return vec4 (lightness*SpecularColor, 1.);
        case 3: return vec4 (lightness*LambertianColor, 1.);
        case 4: return lightness*color;
        case 5: finalColor = applyFog (finalColor, dist, viewDir); return vec4(lightness*finalColor, 1.);
        case 6: return vec4 (mix (finalColor.xyz, nois, 0.2), 1.);
        case 7: return vec4 (nois, 1.);
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
}


// Lambertian light model for surface
subroutine( shadeModelType )
vec4 shadeLambertian(vec3 vertex, vec3 vertexEye, 
                     vec3 normal, vec4 color)
{
        // world-space
        vec3 viewDir = normalize(eyePosWorld.xyz - vertexEye);
        vec3 h = normalize(-lightDirWorld.xyz + viewDir);
        vec3 n = normalize(normal);

        //float diffuse = saturate( dot(n, -lightDir));
        float diffuse = saturate( (dot(n, -lightDirWorld.xyz) + wrap) / (1.0 + wrap));   // wrap
        //float diffuse = dot(n, -lightDir)*0.5+0.5;
        float specular = pow( saturate(dot(h, n)), shininess);

        vec3 LambertianColor = ambientColor.xyz + specular*specularColor.xyz + diffuse*diffuseColor.xyz;

        return vec4 (lightness*LambertianColor * color.xyz, materialColor.a);
}

// Lambertian light model for surface
subroutine( shadeModelType )
vec4 shadeLambertianXColor(vec3 vertex, vec3 vertexEye, 
                           vec3 normal, vec4 color)
{
        // world-space
        vec3 viewDir = normalize(eyePosWorld.xyz - vertexEye);
        vec3 h = normalize(-lightDirWorld.xyz + viewDir);
        vec3 n = normalize(normal);

        //float diffuse = saturate( dot(n, -lightDir));
        float diffuse = saturate( (dot(n, -lightDirWorld.xyz) + wrap) / (1.0 + wrap));   // wrap
        //float diffuse = dot(n, -lightDir)*0.5+0.5;
        float specular = pow( saturate(dot(h, n)), shininess);

        vec3 LambertianColor = ambientColor.xyz + specular*specularColor.xyz + diffuse*diffuseColor.xyz;

        //vec3 xyz_values = map_xyz_values (vertex.xz);
        
        return vec4 (lightness * LambertianColor * color.xyz, materialColor.a);
}

// Lambertian light model for surface, use homogenious surface color = material_color
subroutine( shadeModelType )
vec4 shadeLambertianMaterialColor(vec3 vertex, vec3 vertexEye, 
                                  vec3 normal, vec4 color)
{
        // world-space
        vec3 viewDir = normalize(eyePosWorld.xyz - vertexEye);
        vec3 h = normalize(-lightDirWorld.xyz + viewDir);
        vec3 n = normalize(normal);

        //float diffuse = saturate( dot(n, -lightDir));
        float diffuse = saturate( (dot(n, -lightDirWorld.xyz) + wrap) / (1.0 + wrap));   // wrap
        //float diffuse = dot(n, -lightDir)*0.5+0.5;
        float specular = pow (saturate(dot(h, n)), shininess);
        vec3 LambertianColor = ambientColor.xyz + specular*specularColor.xyz + diffuse*diffuseColor.xyz;

        return vec4 (lightness * LambertianColor * materialColor.xyz, materialColor.a);
}

// Lambertian light model for surface, use homogenious surface color = material_color
subroutine( shadeModelType )
vec4 shadeLambertianMaterialColorFog(vec3 vertex, vec3 vertexEye, 
                                     vec3 normal, vec4 color)
{
        // world-space
        vec3 viewDir = normalize(eyePosWorld.xyz - vertexEye);
        vec3 h = normalize(-lightDirWorld.xyz + viewDir);
        vec3 n = normalize(normal);

        //float diffuse = saturate( dot(n, -lightDir));
        float diffuse = saturate( (dot(n, -lightDirWorld.xyz) + wrap) / (1.0 + wrap));   // wrap
        //float diffuse = dot(n, -lightDir)*0.5+0.5;
        float specular = pow( saturate(dot(h, n)), shininess);
        vec3 LambertianColor = ambientColor.xyz + specular*specularColor.xyz + diffuse*diffuseColor.xyz;
        vec3 finalColor = LambertianColor * materialColor.xyz;

        //float dist = length(vertexEye);
        float dist = length(eyePosWorld.xyz);
        //finalColor = applyFog(finalColor, dist);
        finalColor = applyFog(finalColor, dist, viewDir);

        return vec4 (lightness*finalColor, materialColor.a);
}

vec3 volumecoord(vec3 position){
        return vec3 (0.5 - position.x,  0.5 - position.z, 0.5 - position.y);
}

// Volume shade model for 3D cube interior -- tracing eye vector
subroutine( shadeModelType )
vec4 shadeVolume(vec3 vertex, vec3 vertexEye, 
                 vec3 normal, vec4 color)
{
        vec3 vertex_normalized = vec3 (vertex.xyz/aspect.xzy);
        vec3 vc  = volumecoord (vertex_normalized);
        vec4 zz  = texture (Volume3D_Z_Data, vc);

        //return vec4(vec3(zz.z), transparency_offset+zz.z*transparency);
        
        //float value = clamp (zz.a, 0, 1); // use direct data
        float value = clamp (zz.z, 0, 1); // use view mode data

        if (zz.a == 0. && zz.x == 1.) // outside volume data cube?
                return vec4 (1.,0.,0.,0.);

        return vec4 (lightness * vec3( texture (GXSM_Palette, clamp(color_offset.x+color_offset.y*value, 0., 1.))),
                     transparency_offset+value*transparency);
        

}

void main()
{
        if (gl_FrontFacing) {
                // shade fragment, use model as selected via shaderFunction pointer
                FragColor = shadeModel (In.Vertex, In.VertexEye,
                                        In.Normal, In.Color);
        } else {
                FragColor = shadeModel (In.Vertex, In.VertexEye,
                                        In.Normal, backsideColor);
        }
}

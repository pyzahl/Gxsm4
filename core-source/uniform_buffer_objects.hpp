/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_precision.hpp>

namespace ubo
{
	struct uniform_model_view{
		uniform_model_view
		(
                 glm::mat4 const & ModelView,
                 glm::mat4 const & ViewAlignedModelView,
                 glm::mat4 const & ModelViewProjection
                 ) :
			ModelView(ModelView),
			ViewAlignedModelView(ViewAlignedModelView),
			ModelViewProjection(ModelViewProjection)
		{}

		glm::mat4 ModelView;
		glm::mat4 ViewAlignedModelView;
		glm::mat4 ModelViewProjection;
	};
        
        struct uniform_surface_geometry{
                uniform_surface_geometry
                (
                 glm::vec4 const & aspect,
                 GLfloat const & height_scale,
                 GLfloat const & height_offset,
                 GLfloat const & plane_at,
                 glm::vec4 const & cCenter,
                 glm::vec2 const & delta
                 ) :
                        aspect(aspect),
                        height_scale(height_scale),
                        height_offset(height_offset),
                        plane_at(plane_at),
                        cCenter(cCenter),
                        delta(delta)
                {}
                glm::vec4 aspect;
                GLfloat height_scale;
                GLfloat height_offset;                
                GLfloat plane_at;
                glm::vec4 cCenter;
                glm::vec2 delta;
        };

        struct uniform_fragment_shading{
                uniform_fragment_shading
                (
                 glm::vec4 const & lightDirWorld,
                 glm::vec4 const & eyePosWorld,
                 glm::vec4 const & sunColor,
                 glm::vec4 const & specularColor,
                 glm::vec4 const & ambientColor,
                 glm::vec4 const & diffuseColor,
                 glm::vec4 const & fogColor,
                 glm::vec4  const & materialColor,
                 glm::vec4  const & backsideColor,
                 glm::vec4  const & color_offset,
                 GLfloat const & fogExp,
                 GLfloat const & shininess,
                 GLfloat const & lightness,
                 GLfloat const & light_attenuation,
                 GLfloat const & transparency,
                 GLfloat const & transparency_offset,
                 GLfloat const & wrap,
                 GLuint const & debug_color_source
                 ) :
                        lightDirWorld(lightDirWorld),
                        eyePosWorld(eyePosWorld),
                        sunColor(sunColor),
                        specularColor(specularColor),
                        ambientColor(ambientColor),
                        diffuseColor(diffuseColor),
                        fogColor(fogColor),
                        materialColor(materialColor),
                        backsideColor(backsideColor),
                        color_offset(color_offset),
                        fogExp(fogExp),
                        shininess(shininess),
                        lightness(lightness),
                        light_attenuation(light_attenuation),
                        transparency(transparency),
                        transparency_offset(transparency_offset),
                        wrap(wrap),
                        debug_color_source(debug_color_source)
                {}
                glm::vec4 lightDirWorld;
                glm::vec4 eyePosWorld;
                glm::vec4 sunColor; // = vec3(1.0, 1.0, 0.7);
                glm::vec4 specularColor; // = vec3(1.0, 1.0, 0.7)*1.5;
                glm::vec4 ambientColor; // = vec3(1.0, 1.0, 0.7)*1.5;
                glm::vec4 diffuseColor; // = vec3(1.0, 1.0, 0.7)*1.5;
                glm::vec4 fogColor; // = vec3(0.7, 0.8, 1.0)*0.7;
                glm::vec4 materialColor;
                glm::vec4 backsideColor;
                glm::vec4 color_offset;
                GLfloat fogExp; // = 0.1;
                GLfloat shininess; // = 100.0;
                GLfloat lightness;
                GLfloat light_attenuation;
                GLfloat transparency;
                GLfloat transparency_offset;
                GLfloat wrap;
                GLuint debug_color_source;     // only for debug shader
        };

}//namespace ubo

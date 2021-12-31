/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_precision.hpp>

namespace glf
{
	struct vertex_v1i
	{
		vertex_v1i
		(
                 //			glm::highp_uvec1_t const & indices
			glm::highp_u32vec1 const & indices
		) :
			indices(indices)
		{}

                //glm::highp_uvec1_t indices;
                glm::highp_u32vec1 indices;
	};
	struct vertex_v4f
	{
		vertex_v4f
		(
                        glm::vec4 const & position
		) :
			position(position)
		{}

		glm::vec4 position;
	};
	struct vertex_v3f
	{
		vertex_v3f
		(
			glm::vec3 const & normals
		) :
			normals(normals)
		{}

		glm::vec3 normals;
	};

	struct vertex_v2fv2f
	{
		vertex_v2fv2f
		(
			glm::vec2 const & Position,
			glm::vec2 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec2 Position;
		glm::vec2 Texcoord;
	};

	struct vertex_v3fv2f
	{
		vertex_v3fv2f
		(
			glm::vec3 const & Position,
			glm::vec2 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec3 Position;
		glm::vec2 Texcoord;
	};

	struct vertex_v3fv4u8
	{
		vertex_v3fv4u8
		(
			glm::vec3 const & Position,
			glm::u8vec4 const & Color
		) :
			Position(Position),
			Color(Color)
		{}

		glm::vec3 Position;
		glm::u8vec4 Color;
	};

	struct vertex_v2fv3f
	{
		vertex_v2fv3f
		(
			glm::vec2 const & Position,
			glm::vec3 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec2 Position;
		glm::vec3 Texcoord;
	};

	struct vertex_v3fv3f
	{
		vertex_v3fv3f
		(
			glm::vec3 const & Position,
			glm::vec3 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec3 Position;
		glm::vec3 Texcoord;
	};

	struct vertex_v3fn3fc4f
	{
		vertex_v3fn3fc4f
		(
			glm::vec3 const & Position,
			glm::vec3 const & Normals,
			glm::vec4 const & Color
		) :
			Position(Position),
			Normals(Normals),
                        Color(Color)
		{}

		glm::vec3 Position;
		glm::vec3 Normals;
                glm::vec4 Color;
	};

	struct vertex_v3fn3f
	{
		vertex_v3fn3f
		(
			glm::vec3 const & Position,
			glm::vec3 const & Normals
		) :
			Position(Position),
			Normals(Normals)
		{}

		glm::vec3 Position;
		glm::vec3 Normals;
	};

	struct vertex_v3ft3f
	{
		vertex_v3ft3f
		(
			glm::vec3 const & Position,
			glm::vec3 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec3 Position;
		glm::vec3 Texcoord;
	};

	struct vertex_v3fv3fv1i
	{
		vertex_v3fv3fv1i
		(
			glm::vec3 const & Position,
			glm::vec3 const & Texcoord,
			int const & DrawID
		) :
			Position(Position),
			Texcoord(Texcoord),
			DrawID(DrawID)
		{}

		glm::vec3 Position;
		glm::vec3 Texcoord;
		int DrawID;
	};

	struct vertex_v4fv2f
	{
		vertex_v4fv2f
		(
			glm::vec4 const & Position,
			glm::vec2 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec4 Position;
		glm::vec2 Texcoord;
	};

	struct vertex_v2fc4f
	{
		vertex_v2fc4f
		(
			glm::vec2 const & Position,
			glm::vec4 const & Color
		) :
			Position(Position),
			Color(Color)
		{}

		glm::vec2 Position;
		glm::vec4 Color;
	};

	struct vertex_v3fc4f
	{
		vertex_v3fc4f
		(
			glm::vec3 const & Position,
			glm::vec4 const & Color
		) :
			Position(Position),
			Color(Color)
		{}

		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct vertex_v2fc4d
	{
		vertex_v2fc4d
		(
			glm::vec2 const & Position,
			glm::dvec4 const & Color
		) :
			Position(Position),
			Color(Color)
		{}

		glm::vec2 Position;
		glm::dvec4 Color;
	};

	struct vertex_v4fc4f
	{
		vertex_v4fc4f
		(
			glm::vec4 const & Position,
			glm::vec4 const & Color
		) :
			Position(Position),
			Color(Color)
		{}

		glm::vec4 Position;
		glm::vec4 Color;
	};

	struct vertex_v2fc4ub
	{
		vertex_v2fc4ub
		(
			glm::vec2 const & Position,
			glm::u8vec4 const & Color
		) :
			Position(Position),
			Color(Color)
		{}

		glm::vec2 Position;
		glm::u8vec4 Color;
	};

	struct vertex_v2fv2fv4ub
	{
		vertex_v2fv2fv4ub
		(
			glm::vec2 const & Position,
			glm::vec2 const & Texcoord,
			glm::u8vec4 const & Color
		) :
			Position(Position),
			Texcoord(Texcoord),
			Color(Color)
		{}

		glm::vec2 Position;
		glm::vec2 Texcoord;
		glm::u8vec4 Color;
	};

	struct vertex_v2fv2fv4f
	{
		vertex_v2fv2fv4f
		(
			glm::vec2 const & Position,
			glm::vec2 const & Texcoord,
			glm::vec4 const & Color
		) :
			Position(Position),
			Texcoord(Texcoord),
			Color(Color)
		{}

		glm::vec2 Position;
		glm::vec2 Texcoord;
		glm::vec4 Color;
	};

	struct vertex_v4fv4f
	{
		vertex_v4fv4f
		(
			glm::vec4 const & Position,
			glm::vec4 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec4 Position;
		glm::vec4 Texcoord;
	};

	struct vertex_v4fv4fv4f
	{
		vertex_v4fv4fv4f
		(
			glm::vec4 const & Position,
			glm::vec4 const & Texcoord,
			glm::vec4 const & Color
		) :
			Position(Position),
			Texcoord(Texcoord),
			Color(Color)
		{}

		glm::vec4 Position;
		glm::vec4 Texcoord;
		glm::vec4 Color;
	};

}//namespace glf

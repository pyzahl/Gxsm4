#pragma once

#include <stdint.h>

#include "csv.hpp"
#include "compiler.hpp"
#include "sementics.hpp"
#include "vertex.hpp"
#include "uniform_buffer_objects.hpp"
#include "error.hpp"
#include "caps.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtc/round.hpp>
//#include <glm/gtx/color_space.hpp>
//#include <glm/gtx/integer.hpp>
//#include <glm/gtx/fast_trigonometry.hpp>

#include <memory>
#include <array>

#if (GLM_COMPILER & GLM_COMPILER_VC) && (GLM_COMPILER < GLM_COMPILER_VC12)
#	error "The OpenGL based GXSM 3D view support requires at least Visual C++ 2013 and the GLM (GLSL GPU code compiler)."
#endif//

#if (GLM_COMPILER & GLM_COMPILER_GCC) && (GLM_COMPILER < GLM_COMPILER_GCC47)
#	error "The OpenGL based GXSM 3D view support requires at least GCC 4.7 and the GLM (GLSL GPU code compiler)."
#endif//

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

struct vertexattrib
{
	vertexattrib() :
		Enabled(GL_FALSE),
		Binding(0),
		Size(4),
		Stride(0),
		Type(GL_FLOAT),
		Normalized(GL_FALSE),
		Integer(GL_FALSE),
		Long(GL_FALSE),
		Divisor(0),
		Pointer(NULL)
	{}

	vertexattrib
	(
		GLint Enabled,
		GLint Binding,
		GLint Size,
		GLint Stride,
		GLint Type,
		GLint Normalized,
		GLint Integer,
		GLint Long,
		GLint Divisor,
		GLvoid* Pointer
	) :
		Enabled(Enabled),
		Binding(Binding),
		Size(Size),
		Stride(Stride),
		Type(Type),
		Normalized(Normalized),
		Integer(Integer),
		Long(Long),
		Divisor(Divisor),
		Pointer(Pointer)
	{}

	GLint Enabled;
	GLint Binding;
	GLint Size;
	GLint Stride;
	GLint Type;
	GLint Normalized;
	GLint Integer;
	GLint Long;
	GLint Divisor;
	GLvoid* Pointer;
};

inline bool operator== (vertexattrib const & A, vertexattrib const & B)
{
	return A.Enabled == B.Enabled && 
		A.Size == B.Size && 
		A.Stride == B.Stride && 
		A.Type == B.Type && 
		A.Normalized == B.Normalized && 
		A.Integer == B.Integer && 
		A.Long == B.Long;
}

inline bool operator!= (vertexattrib const & A, vertexattrib const & B)
{
	return !(A == B);
}


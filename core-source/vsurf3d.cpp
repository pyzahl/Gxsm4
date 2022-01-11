/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/*
  https://github.com/NVIDIAGameWorks/GraphicsSamples/blob/master/samples/es3aep-kepler/TerrainTessellation/TerrainTessellation.cpp/
*/

#include <locale.h>
#include <libintl.h>

#include <GL/glew.h>
#include <GL/gl.h>

// C String
#include <cstdio>
#include <cstring>
#include <cassert>

// FreeType headers
#include <ft2build.h>
#include FT_FREETYPE_H

// ICU headers
#include <unicode/ustring.h>

// libpng support utils
#include "writepng.h"    /* typedefs, common macros, public prototypes */

#include "config.h"
#include "gnome-res.h"

#include "view.h"
#include "mem2d.h"
#include "xsmmasks.h"
#include "glbvars.h"

#include "bench.h"
#include "util.h"

#include "app_v3dcontrol.h"

#include <gtk/gtk.h>
#include "action_id.h"

#include "vsurf3d_pref.cpp"
#include "surface.h"

#include "xsmdebug.h"


#include "glbvars.h"


//#define __GXSM_PY_DEVEL
#ifdef __GXSM_PY_DEVEL
#define GLSL_DEV_DIR "/home/pzahl/SVN/Gxsm-4.0/gl-400/"
//#define GLSL_DEV_DIR "/home/percy/SVN/Gxsm-4.0/gl-400/"
#endif

#if ENABLE_3DVIEW_HAVE_GL_GLEW

// ------------------------------------------------------------
// glm, gli, GL support includes
// ------------------------------------------------------------
#include "ogl_framework.hpp"

#define GL_DEBUG_L2 0
#define GL_DEBUG_L3 1

#endif // HAVE_GLEW

// ------------------------------------------------------------
// gschema creator for from internal recources
// ------------------------------------------------------------
void surf3d_write_schema (){
	GnomeResPreferences *gl_pref = gnome_res_preferences_new (v3dControl_pref_def_const, GXSM_RES_GL_PATH);
	gchar *gschema = gnome_res_write_gschema (gl_pref);
                
	std::ofstream f;
	f.open (GXSM_RES_BASE_PATH_DOT ".gl.gschema.xml", std::ios::out);
	f << gschema
	  << std::endl;
	f.close ();

	g_free (gschema);
	
	gnome_res_destroy (gl_pref);
}


#if ENABLE_3DVIEW_HAVE_GL_GLEW

// ------------------------------------------------------------
// glsl data and code locations
// ------------------------------------------------------------
std::string getDataDirectory()
{
#ifdef __GXSM_PY_DEVEL
        return std::string(GLSL_DEV_DIR);
#else
	return std::string(PACKAGE_GL400_DIR) + "/";
#endif
}

std::string getBinaryDirectory()
{
#ifdef __GXSM_PY_DEVEL
        return std::string(GLSL_DEV_DIR);
#else
	return std::string(PACKAGE_GL400_DIR) + "/";
#endif
}

// ------------------------------------------------------------
// GL 4.0 required -- GL 3D support with GPU tesselation
// globally used shader program rescources
// ------------------------------------------------------------
namespace
{
        // define global include file -- included by compiler
	std::string const CMD_ARGS_FOR_SHADERS("");

        // surface terrain mode tesselation shaders
	std::string const TESSELATION_VERTEX_SHADER("tess-vertex.glsl");
	std::string const TESSELATION_CONTROL_SHADER("tess-control.glsl");
	std::string const TESSELATION_EVALUATION_SHADER("tess-evaluation.glsl");
	std::string const TESSELATION_GEOMETRY_SHADER("tess-geometry.glsl");
	std::string const TESSELATION_FRAGMENT_SHADER("tess-fragment.glsl");
        GLuint ProgramName_RefCount(0);
	GLuint Tesselation_ProgramName(0);

        // simple text shaders
        std::string const S3D_VERTEX_SHADER("s3d-vertex.glsl");
	std::string const S3D_FRAGMENT_SHADER("s3d-fragment.glsl");
	GLuint S3D_ProgramName(0);

        // generic vertex tesselation shaders
	std::string const ICO_TESS_VERTEX_SHADER("ico-vertex.glsl");
	std::string const ICO_TESS_CONTROL_SHADER("ico-control.glsl");
	std::string const ICO_TESS_EVALUATION_SHADER("ico-evaluation.glsl");
	std::string const ICO_TESS_GEOMETRY_SHADER("ico-geometry.glsl");
	std::string const ICO_TESS_FRAGMENT_SHADER("ico-fragment.glsl");
	GLuint IcoTess_ProgramName(0);
        // make UBO for tranformation, fragment/lights, geometry, textures
        
	GLuint Uniform_screen_size(0); // vec2
	GLuint Uniform_tess_level(0); // float
	GLuint Uniform_lod_factor(0); // float

        // Model View and Projection
        GLuint Uniform_ubo_list[3];
        GLuint const ModelViewMat_block(0); // => 0
        ubo::uniform_model_view Block_ModelViewMat(glm::mat4(1.), glm::mat4(1.), glm::mat4(1.));

        GLuint const SurfaceGeometry_block(1); // => 1
        ubo::uniform_surface_geometry Block_SurfaceGeometry( glm::vec4 (1.f,1.f,1.f,1.f),0.1,0.,0., glm::vec4 (0.5f,0.5f,0.5f,0.5f), glm::vec2(0.1f));
        
        GLuint const FragmentShading_block(2); // => 2
        ubo::uniform_fragment_shading Block_FragmentShading
        (
         glm::vec4(-0.2,0.6,-1.0,0.0), // light dir
         glm::vec4(0.,-1.,-70.,0.0), // eye/cam

         glm::vec4(1.), // sun color
         glm::vec4(1.), // spekular
         glm::vec4(0.1), // ambient
         glm::vec4(0.3), // diffuse
         glm::vec4(0.5), // fog color
         glm::vec4(1.0), // material color
         glm::vec4(1.0), // backside color
         glm::vec4(0.), // color offset

         0.01, // fog exp

         20.0, // shinyness
         1.5, // lightness
         0., // attn.
         1.0, 0.0, // transparency and offset
         
         0.3, // wrap
         0 // debug
         );

        // vertex shader Function references
        GLuint Uniform_vertex_setup[1];
        GLuint Uniform_vertexFlat(0);
        GLuint Uniform_vertexDirect(0);
        GLuint Uniform_vertexViewMode(0);
        GLuint Uniform_vertexY(0);
        GLuint Uniform_vertexXChannel(0);
        GLuint Uniform_vertex_plane_at(0);

        //-- evaluation shader Function references
        GLuint Uniform_evaluation_setup[3];
        GLuint Uniform_evaluationVertexFlat(0); // Vertex == match Vertex Mode above 
        GLuint Uniform_evaluationVertexDirect(0);
        GLuint Uniform_evaluationVertexViewMode(0);
        GLuint Uniform_evaluationVertexY(0);
        GLuint Uniform_evaluationVertexXChannel(0);
        GLuint Uniform_evaluationColorFlat(0); // Color
        GLuint Uniform_evaluationColorDirect(0);
        GLuint Uniform_evaluationColorViewMode(0);
        GLuint Uniform_evaluationColorXChannel(0);
        GLuint Uniform_evaluationColorY(0);
        GLuint Uniform_evaluation_vertex_XZplane(0);
        GLuint Uniform_evaluation_vertex_XYplane(0);
        GLuint Uniform_evaluation_vertex_ZYplane(0);
        GLuint Uniform_evaluation_vertex_Mplane(0);

        // fragment shader Function references
        GLuint Uniform_shadeTerrain(0);
        GLuint Uniform_shadeDebugMode(0);
        GLuint Uniform_shadeLambertian(0);
        GLuint Uniform_shadeLambertianXColor(0);
        GLuint Uniform_shadeLambertianMaterialColor(0);
        GLuint Uniform_shadeLambertianMaterialColorFog(0);
        GLuint Uniform_shadeVolume(0);

#if 0        //
        GLuint Uniform_S3D_vertexDirect(0); // vertex
        GLuint Uniform_S3D_vertexSurface(0);
        GLuint Uniform_S3D_vertexHScaled(0);
        GLuint Uniform_S3D_vertexText(0);

        GLuint Uniform_S3D_shadeLambertian(0);
        GLuint Uniform_S3D_shadeText(0);
#endif

        GLuint Uniform_IcoTess_TessLevelInner(0);
        GLuint Uniform_IcoTess_TessLevelOuter(0);
        GLuint Uniform_IcoTess_IcoPosition(0);
        GLuint Uniform_IcoTess_IcoScale(0);
        GLuint Uniform_IcoTess_IcoColor(0);

        GLuint Uniform_TipPosition(0);
        GLuint Uniform_TipColor(0);
        
} //namespace

class base_plane{
public:
        base_plane (int nx, int ny, int nv, glm::vec4 *displacement_data, glm::vec4 *palette_data, GLsizei num_pal_entries=GXSM_GPU_PALETTE_ENTRIES, int w=128, double aspect=1.0, GLint lod=0){
                Validated = true;
                BaseGridW = w;
                BaseGridH = w; // adjusted by make_plane_vbo using aspect
                ArrayBufferName = 0;
                VertexArrayName = 0;
                TesselationTextureCount = 0;
                numx = nx; numy = ny; numv = nv;

                vertex  = NULL;
                indices = NULL;
                
                make_plane_vbo (aspect);

                Validated = init_buffer ();
                Validated = init_vao ();
                Validated = init_textures (displacement_data, palette_data, num_pal_entries, lod);
        };
        ~base_plane (){
		if (Validated){
                        g_free (indices);
                        g_free (vertex);
                        glDeleteTextures(TesselationTextureCount, TesselationTextureName);
                        glDeleteVertexArrays(1, &VertexArrayName);
                        glDeleteBuffers(1, &IndexBufferName);
                        glDeleteBuffers(1, &ArrayBufferName);
                        Surf3d::checkError("make_plane::~delete");
                }
        };
        gboolean init_buffer (){
                Surf3d::checkError("make_plane:: init_buffer");
		if (!Validated) return false;

                glGenBuffers(1, &ArrayBufferName);
                glBindBuffer(GL_ARRAY_BUFFER, ArrayBufferName);
                //glBufferData(GL_ARRAY_BUFFER, VertexObjectSize, vertex, GL_DYNAMIC_DRAW);
                glBufferData(GL_ARRAY_BUFFER, VertexObjectSize, vertex, GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glGenBuffers(1, &IndexBufferName);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferName);
                //glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndicesObjectSize, indices, GL_DYNAMIC_DRAW);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndicesObjectSize, indices, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                return Validated && Surf3d::checkError("make_plane:: init_buffer");
        };

        gboolean update_vertex_buffer (){
                glBindBuffer (GL_ARRAY_BUFFER, ArrayBufferName);
                glBufferSubData (GL_ARRAY_BUFFER, 0, VertexObjectSize, vertex);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                return Validated && Surf3d::checkError("make_plane:: update_vertex_buffer");
        };
        
        gboolean init_vao (){
                g_message ("base_plane init_vao");
                Surf3d::checkError("make_plane::init_vao");
		if (!Validated) return false;

                // Build a vertex array object
                glGenVertexArrays(1, &VertexArrayName);

                glBindVertexArray(VertexArrayName);
                glBindBuffer(GL_ARRAY_BUFFER, ArrayBufferName);

                glVertexAttribPointer(semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), BUFFER_OFFSET(0));

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                g_message ("base_plane init_vao end");

                return Validated && Surf3d::checkError("make_plane::init_vao");
        };
        gboolean init_textures(glm::vec4 *displacement_data, glm::vec4 *palette_data, GLsizei num_pal_entries=GXSM_GPU_PALETTE_ENTRIES, GLint lod=0) {
		if (!Validated) return false;

                // sampler2D diffuse
                glUseProgram (Tesselation_ProgramName);
                TesselationTextureName[2] = 0;
                
                if (numv > 1)
                        TesselationTextureCount = 3;
                else
                        TesselationTextureCount = 2;
                
                glGenTextures (TesselationTextureCount, TesselationTextureName);

                // sampler2D Surf3D_Z-Data vec4[][]
                glActiveTexture (GL_TEXTURE0);
                glBindTexture (GL_TEXTURE_2D, TesselationTextureName[0]);

                glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, numx, numy, 0, GL_RGBA, GL_FLOAT, displacement_data); // Surf3D_Zdata -- 1st layer only
                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glGenerateMipmap (GL_TEXTURE_2D);
                glUniform1i (glGetUniformLocation (Tesselation_ProgramName, "Surf3D_Z_Data"), 0);

                // sampler1D GXSM_Palette vec4[][]
                glActiveTexture (GL_TEXTURE1);
                glBindTexture (GL_TEXTURE_1D, TesselationTextureName[1]);

                glTexImage1D (GL_TEXTURE_1D, 0, GL_RGBA32F, num_pal_entries, 0, GL_RGBA, GL_FLOAT, palette_data); // Surf3D_Palette
                // GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, GL_CLAMP_TO_BORDER
                glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glGenerateMipmap (GL_TEXTURE_1D);
                glUniform1i (glGetUniformLocation (Tesselation_ProgramName, "GXSM_Palette"), 1);

                if (numv > 1){
                        // sampler3D Volume3D_Z-Data vec4[][]
                        glActiveTexture (GL_TEXTURE2);
                        glBindTexture (GL_TEXTURE_3D, TesselationTextureName[2]);

                        glTexImage3D (GL_TEXTURE_3D, lod, GL_RGBA32F, numx, numy, numv, 0, GL_RGBA, GL_FLOAT, displacement_data); // Surf3D_Zdata

                        // GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, GL_CLAMP_TO_BORDER
                        glm::vec4 outside_color = glm::vec4 (1.f,0.f,0.f,0.f);
                        glTexParameterfv (GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, &(outside_color.x));
                        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

                        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glGenerateMipmap (GL_TEXTURE_3D);
                        glUniform1i (glGetUniformLocation (Tesselation_ProgramName, "Volume3D_Z_Data"), 2);
                }
                // unbind
                glBindTexture (GL_TEXTURE_2D, 0);

                return Validated && Surf3d::checkError("make_plane::init_textures");
        };
        // update height mapping data
        gboolean update_displacement_data (glm::vec4 *displacement_data, GLint line, GLsizei num_lines=1) { 
		if (!Validated) return false;

                glTextureSubImage2D (TesselationTextureName[0], 0, 0, line, numx, num_lines, GL_RGBA, GL_FLOAT, &displacement_data[line*numx]);

		return Surf3d::checkError("make_plane::update_textures displacement");
        };

        // update volume data
        gboolean update_volume_data (glm::vec4 *displacement_data, GLint line, GLint v, GLsizei num_lines=1) { 
		if (!Validated) return false;

                if (numv > 1 && v < numv)
                        glTextureSubImage3D (TesselationTextureName[2], 0,  0, line, v, numx, num_lines, 1, GL_RGBA, GL_FLOAT, &displacement_data[line*numx + numx*numy*v]);
                
		return Surf3d::checkError("make_plane::update_textures volume");
        };

        // update palette lookup data
        gboolean update_palette_lookup (glm::vec4 *palette_data, GLsizei num_pal_entries=GXSM_GPU_PALETTE_ENTRIES) { 
		if (!Validated) return false;

                glTextureSubImage1D (TesselationTextureName[1], 0, 0, num_pal_entries, GL_RGBA, GL_FLOAT, &palette_data[0]);

		return Surf3d::checkError("make_plane::update_texture palette");
        };
        
        gboolean draw (){
		if (!Validated) return false;
                
                glBindVertexArray (VertexArrayName);
                glEnableVertexAttribArray (semantic::attr::POSITION);

                glBindBuffer (GL_ARRAY_BUFFER, ArrayBufferName);
                glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, IndexBufferName);
                glPatchParameteri (GL_PATCH_VERTICES, 4);
                Surf3d::checkError("make_plane::draw bindbuff");

                glBindTexture (GL_TEXTURE_2D, TesselationTextureName[0]);
                glBindTexture (GL_TEXTURE_1D, TesselationTextureName[1]);
                Surf3d::checkError("make_plane::draw tex0,1");
                if (numv>1)
                        glBindTexture(GL_TEXTURE_3D, TesselationTextureName[2]);
                Surf3d::checkError("make_plane::draw tex2");
                
                glDrawElements (GL_PATCHES, IndicesCount, GL_UNSIGNED_INT, 0);

                glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
                glBindBuffer (GL_ARRAY_BUFFER, 0);
                glBindVertexArray (0);

                return Validated && Surf3d::checkError("make_plane::draw");
        };
        
        // make plane
        void make_plane_vbo (double aspect=1.0, double oversize = 1.0){
                // Surface Object Vertices
                BaseGridH = (GLuint)round((double)BaseGridW * aspect);
                
                g_message ("make_plane");
                VertexCount =  BaseGridW*BaseGridH;
                VertexObjectSize = VertexCount * sizeof(glm::vec2);
               
                g_message ("mkplane w=%d h=%d  nvertices=%d", BaseGridW, BaseGridH, VertexCount);
                if (!vertex)
                        vertex = g_new (glm::vec2, VertexCount);
                double s_factor = 1.0/(BaseGridW-1);
                double t_factor = 1.0/(BaseGridH-1);
                int offset;
                // surface vertices
                for (guint y=0; y<BaseGridH; ++y){
                        for (guint x=0; x<BaseGridW; ++x){
                                offset = y*BaseGridW+x;
                                double xd = x * s_factor; // 0..1
                                double yd = y * t_factor; // 0..1

                                vertex[offset] = glm::vec2 (oversize * (xd-0.5), oversize * (yd-0.5)*aspect);
                                //g_message ("mkplane vtx -- x=%d y=TEX z=%d  [%d]",x,y, offset);
                        }
                }
                g_message ("mkplane -- Vertex Count=%d", offset);

                if (!indices){
                        // Patch Indices
                        int i_width = BaseGridW-1;
                        int i_height = BaseGridH-1;
                        IndicesCount = (((BaseGridW-1)*(BaseGridH-1))*4); // +4 -- plus close up botton patch (no workign w terrain)
                        IndicesObjectSize = IndicesCount * sizeof(glf::vertex_v1i);

                        indices = g_new (glf::vertex_v1i, IndicesCount);
                        // surface patches
                        int ii=0;
                        for (int y=0; y<i_height; ++y){
                                for (int x=0; x<i_width; ++x){
                                        int p1 = x+y*BaseGridW;
                                        int p2 = p1+BaseGridW;
                                        int p4 = p1+1;
                                        int p3 = p2+1;
                                        indices[ii++].indices.x = p1;
                                        indices[ii++].indices.x = p2;
                                        indices[ii++].indices.x = p3;
                                        indices[ii++].indices.x = p4;
                                }
                        }
                        g_message ("mkplane -- Indices Count=%d of %d", ii, IndicesCount);
                }
        };

private:
        bool Validated;
	int numx, numy, numv;
        glm::vec2 *vertex;
        glf::vertex_v1i *indices;
        GLuint BaseGridW;
        GLuint BaseGridH;
	GLuint ArrayBufferName;
	GLuint IndexBufferName;
	GLuint VertexArrayName;
	GLsizei VertexCount;
	GLsizei IndicesCount;
	GLsizeiptr VertexObjectSize;
	GLsizeiptr IndicesObjectSize;
        GLsizei TesselationTextureCount;
	GLuint TesselationTextureName[3];
};

class text_plane{
public:
        text_plane (){
                Validated = true;
                ArrayBufferName = 0;
                VertexArrayName = 0;

                g_message ("text_plane:: init object");

                if (Validated && FT_Init_FreeType(&ft)) {
                        g_warning ("Could not init freetype library. -- No GL text rendering.");
                        Validated = false;
                }

                // fix me -- find out how to get path or install own??
                if (Validated && FT_New_Face(ft, "/usr/share/fonts/truetype/freefont/FreeSans.ttf", 0, &face)) {
                        g_warning ("Could not open font /usr/share/fonts/truetype/freefont/FreeSans.ttf. -- No GL text rendering.");
                        main_get_gapp ()->warning ("Could not open font /usr/share/fonts/truetype/freefont/FreeSans.ttf.\n"
                                       "No OpenGL text rendering via FreeType library.\n"
                                       "Please install the 'fonts-freefont-ttf' package.");
                        Validated = false;
                }

                if (Validated){
                        FT_Set_Pixel_Sizes (face, 0, 48);
                        g = face->glyph;

                        Validated = init_vao ();
                        Validated = init_texture ();
                }
                
                Surf3d::checkError("text_plane::init");
                g_message ("text_plane:: init object completed");
        };
        ~text_plane (){
		if (Validated){
                        glDeleteTextures(1, &TextTextureName);
                        glDeleteVertexArrays(1, &VertexArrayName);
                        Surf3d::checkError("text_plane::~delete");
                }
        };
        gboolean init_texture (){
                Surf3d::checkError("text_plane:: init_buffer");
		if (!Validated) return false;

                glUseProgram (S3D_ProgramName);

                glActiveTexture (GL_TEXTURE3); // dedicated for text
                glGenTextures (1, &TextTextureName); // done globally
                glBindTexture (GL_TEXTURE_2D, TextTextureName);
                glUniform1i (glGetUniformLocation (S3D_ProgramName, "textTexture"), 3);

                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
                glBindTexture (GL_TEXTURE_2D, 0);
               
                return Validated && Surf3d::checkError("text_plane:: init_buffer completed");
        };
        gboolean init_vao (){
                g_message ("base_plane init_vao");
                Surf3d::checkError("make_plane::init_vao");
		if (!Validated) return false;

                // Build a vertex array object
                glGenVertexArrays(1, &VertexArrayName);
                glBindVertexArray(VertexArrayName);

                glGenBuffers (1, &ArrayBufferName);
                glBindBuffer (GL_ARRAY_BUFFER, ArrayBufferName);
                glVertexAttribPointer(semantic::s3d_text::POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4)+sizeof(glm::vec2), BUFFER_OFFSET(0));
                glVertexAttribPointer(semantic::s3d_text::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4)+sizeof(glm::vec2), BUFFER_OFFSET(sizeof(glm::vec4)));
                glBindBuffer (GL_ARRAY_BUFFER, 0);

                return Validated && Surf3d::checkError("make_plane::init_vao");
        };
        gboolean draw (const char *text, glm::vec3 pos, glm::vec3 ex, glm::vec3 ey, glm::vec4 color=glm::vec4(1.0,0.1,0.5,0.7)) {
 		if (!Validated) return false;

                //g_message ("text_plane::draw  %s", text);

                GLfloat glyp_size = 48.;
                
                ex /= -glyp_size;
                ey /= glyp_size;
                
                glDisable (GL_CULL_FACE);
                glEnable (GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                glUseProgram (S3D_ProgramName);

                glUniform4fv (glGetUniformLocation (S3D_ProgramName, "textColor"), 1, &color.x);

                glBindTexture (GL_TEXTURE_2D, TextTextureName);

                glBindVertexArray(VertexArrayName);
                glBindBuffer (GL_ARRAY_BUFFER, ArrayBufferName);
                glEnableVertexAttribArray (semantic::s3d_text::POSITION);
                glEnableVertexAttribArray (semantic::s3d_text::TEXCOORD);

                Surf3d::checkError("make_plane::draw start");

                // FreeType uses Unicode as glyph index; so we have to convert string from UTF8 to Unicode(UTF32)
                int utf16_buf_size = strlen(text) + 1; // +1 for the last '\0'
                std::unique_ptr<UChar[]> utf16_str(new UChar[utf16_buf_size]);
                UErrorCode err = U_ZERO_ERROR;
                int utf16_length;
                u_strFromUTF8(utf16_str.get(), utf16_buf_size, &utf16_length, text, strlen(text), &err);
                if (err != U_ZERO_ERROR) {
                        fprintf(stderr, "u_strFromUTF8() failed: %s\n", u_errorName(err));
                        return false;
                }

                int utf32_buf_size = utf16_length + 1; // +1 for the last '\0'
                std::unique_ptr<UChar32[]> utf32_str(new UChar32[utf32_buf_size]);
                int utf32_length;
                u_strToUTF32(utf32_str.get(), utf32_buf_size, &utf32_length, utf16_str.get(), utf16_length, &err);
                if (err != U_ZERO_ERROR) {
                        fprintf(stderr, "u_strToUTF32() failed: %s\n", u_errorName(err));
                        return false;
                }

                for (int i = 0; i < utf32_length; i++) {
                        FT_UInt glyph_index = FT_Get_Char_Index(face, utf32_str[i]);
                        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)){
                                g_warning ("FT_Load_Glyph() failed for <%c>.\n", utf32_str[i]);
                                continue;
                        }
                
                        glTexImage2D (
                                      GL_TEXTURE_2D,
                                      0,
                                      GL_RED,
                                      g->bitmap.width,
                                      g->bitmap.rows,
                                      0,
                                      GL_RED,
                                      GL_UNSIGNED_BYTE,
                                      g->bitmap.buffer
                                      );

                        glm::vec3 pg = pos + (GLfloat)g->bitmap_left*ex + (GLfloat)g->bitmap_top*ey;
                        GLfloat w = g->bitmap.width;
                        GLfloat h = g->bitmap.rows;
                        typedef struct {
                                glm::vec4 p;
                                glm::vec2 t;
                        } glypvert;

                        glypvert box[4] = { // X,Y,Z,r,  Tx,Ty
                                { glm::vec4 (pg,               1), glm::vec2(0,0) },
                                { glm::vec4 (pg + w*ex,        1), glm::vec2(1,0) },
                                { glm::vec4 (pg - h*ey,        1), glm::vec2(0,1) },
                                { glm::vec4 (pg + w*ex - h*ey, 1), glm::vec2(1,1) }
                        };

                        glBindBuffer (GL_ARRAY_BUFFER, ArrayBufferName);
                        glBufferData (GL_ARRAY_BUFFER, sizeof (box), box, GL_DYNAMIC_DRAW);

                        glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
                        glBindBuffer (GL_ARRAY_BUFFER, 0);

                        pos += (GLfloat)(g->advance.x/64)*ex + (GLfloat)(g->advance.y/64)*ey;
                }
                glBindBuffer (GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);

                glDisable (GL_BLEND);
                
                return Validated && Surf3d::checkError("text_plane::draw end");
        };
private:        
        bool Validated;
        FT_Library ft;
        FT_Face face;
        FT_GlyphSlot g;
	GLuint ArrayBufferName;
	GLuint VertexArrayName;
        GLuint TextTextureName;
	GLsizei VertexCount;
	GLsizeiptr VertexObjectSize;
	GLsizeiptr IndicesObjectSize;
};

class icosahedron{
public:
        icosahedron (GLsizei na=5){
                Validated = true;
                VertexArrayName = 0;
                ArrayBufferName = 0;
                IndexBufferName = 0;

                indices = NULL;
	        if (na > 1000000)
	               na = 10000;
	        LatticeCount = na;
                
                g_message ("icosahedron:: init object count=%d", na);

                Validated = init_vao ();
                Validated = init_buffer ();
                Validated = init_texture ();

                Surf3d::checkError("icosahedron::init");
                g_message ("icosahedron:: init object completed");
        };
        ~icosahedron (){
                g_free (indices);
		if (Validated){
                        glDeleteVertexArrays(1, &VertexArrayName);
                        glDeleteBuffers(1, &IndexBufferName);
                        glDeleteBuffers(1, &ArrayBufferName);
                        glDeleteBuffers(1, &LatticeBufferName);
                        //glDeleteTextures(1, &TextTextureName);
                        Surf3d::checkError("icosahedron::~delete");
                }
        };
        gboolean init_buffer (){
                Surf3d::checkError("make_plane:: init_buffer");
		if (!Validated) return false;

                VertexCount =  12;
                IndicesCount = 3*20;
                indices = g_new (glf::vertex_v1i, IndicesCount);
 
                // 20 Faces
                const int Faces[3*20] = {
                        2, 1, 0,
                        3, 2, 0,
                        4, 3, 0,
                        5, 4, 0,
                        1, 5, 0,
                        
                        11, 6,  7,
                        11, 7,  8,
                        11, 8,  9,
                        11, 9,  10,
                        11, 10, 6,
                        
                        1, 2, 6,
                        2, 3, 7,
                        3, 4, 8,
                        4, 5, 9,
                        5, 1, 10,
                        
                        2,  7, 6,
                        3,  8, 7,
                        4,  9, 8,
                        5, 10, 9,
                        1, 6, 10 };

                // copy into for sure GL compatible memory
                for (int i=0; i<IndicesCount; ++i)
                        indices[i].indices.x = Faces[i];
                
                const glm::vec4 Verts[12] = {
                        glm::vec4 (0.000f,  0.000f,  1.000f, 1),
                        
                        glm::vec4 (0.894f,  0.000f,  0.447f, 1),
                        glm::vec4 (0.276f,  0.851f,  0.447f, 1),
                        glm::vec4 (-0.724f,  0.526f,  0.447f, 1),
                        glm::vec4 (-0.724f, -0.526f,  0.447f, 1),
                        glm::vec4 (0.276f, -0.851f,  0.447f, 1),
                        
                        glm::vec4 (0.724f,  0.526f, -0.447f, 1),
                        glm::vec4 (-0.276f,  0.851f, -0.447f, 1),
                        glm::vec4 (-0.894f,  0.000f, -0.447f, 1),
                        glm::vec4 (-0.276f, -0.851f, -0.447f, 1),
                        glm::vec4 (0.724f, -0.526f, -0.447f, 1),
                        
                        glm::vec4 (0.000f,  0.000f, -1.000f, 1)
                };

                glGenBuffers(1, &ArrayBufferName);
                glBindBuffer(GL_ARRAY_BUFFER, ArrayBufferName);
                glBufferData(GL_ARRAY_BUFFER, VertexCount*sizeof(glm::vec4), Verts, GL_STATIC_DRAW);
                glEnableVertexAttribArray (semantic::ico::POSITION);
                glVertexAttribPointer (semantic::ico::POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), BUFFER_OFFSET(0));
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glGenBuffers(1, &IndexBufferName);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferName);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndicesCount*sizeof(glf::vertex_v1i), indices, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                const GLfloat rr = 1.27805/1.6; // normalize to d=3.2
                const GLfloat zz = 2.004/1.6;
                const glm::vec4 BaseLattice[5] = {
                        glm::vec4 ( 0.0f,  0.0f,  0.0f, 0),
                        glm::vec4 ( 0.0f,    zz,  0.0f, 0),
                        glm::vec4 ( -rr,  zz+rr,     rr, 0),
                        glm::vec4 (  rr,  zz+rr,     rr, 0),
                        glm::vec4 ( 0.0f, zz+rr,    -rr, 0)
                };
                glm::vec4 *lattice = g_new (glm::vec4, LatticeCount);
                for (int i=0; i<LatticeCount; ){
                        if (i<5){
                                lattice[i] = BaseLattice[i];
                                ++i;
                        }else{
                                for (int layer=2; i<LatticeCount; ++layer)
                                        for (int r=layer-2; r<=layer && i<LatticeCount; ++r)
                                                for (int a=-r+layer%2; a<=r && i<LatticeCount; a+=2)
                                                        for (int b=-r+layer%2; b<=r && i<LatticeCount; b+=2){
                                                                int aabb = a*a + b*b;
                                                                if (aabb <= layer*layer && aabb > (layer-1)*(layer-1)){
                                                                        if (i<LatticeCount){
                                                                                lattice[i] = glm::vec4(a*rr, zz+rr*layer, b*rr, 0);
                                                                                // g_message ("Tip: [%d] %d :  %d %d", i, layer,a,b);
                                                                        }
                                                                        ++i;
                                                                }
                                                        }
                        }
                }
                glGenBuffers(1, &LatticeBufferName);
                glBindBuffer(GL_ARRAY_BUFFER, LatticeBufferName);
                glBufferData(GL_ARRAY_BUFFER, LatticeCount*sizeof(glm::vec4), lattice, GL_STATIC_DRAW);
                g_free (lattice);
                glEnableVertexAttribArray (semantic::ico::LATTICE);
                glVertexAttribPointer (semantic::ico::LATTICE, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), BUFFER_OFFSET(0));
                glVertexAttribDivisor (semantic::ico::LATTICE, 1); // instancing
                glBindBuffer(GL_ARRAY_BUFFER, 0);
 
                return Validated && Surf3d::checkError("make_plane:: init_buffer");
        };
        gboolean init_texture (){
                Surf3d::checkError("icosahedron:: init_texture");
		if (!Validated) return false;

                //glUseProgram (S3D_ProgramName);
              
                return Validated && Surf3d::checkError("text_plane:: init_buffer completed");
        };
        gboolean init_vao (){
                g_message ("icosahedron init_vao");
                Surf3d::checkError("icosahedron::init_vao");
		if (!Validated) return false;

                // Build a vertex array object
                glGenVertexArrays(1, &VertexArrayName);
                glBindVertexArray(VertexArrayName);

                return Validated && Surf3d::checkError("make_plane::init_vao");
        };
        // vec4: (x,y,z, scale)
        gboolean draw (glm::vec4 pos, glm::vec4 scale, glm::vec4 color[4], gfloat tesslevel=2.) {
 		if (!Validated) return false;

                g_message ("icosahedron draw");

                glUseProgram (IcoTess_ProgramName);

                glUniform1f (Uniform_IcoTess_TessLevelInner, tesslevel);
                glUniform1f (Uniform_IcoTess_TessLevelOuter, tesslevel);  
                glUniform4fv (Uniform_IcoTess_IcoPosition, 1, &(pos.x));
                glUniform4fv (Uniform_IcoTess_IcoScale, 1, &(scale.x));
                glUniform4fv (Uniform_IcoTess_IcoColor, 4, &(color[0].x));

                glBindVertexArray (VertexArrayName); // VAO
                glBindBuffer (GL_ARRAY_BUFFER, ArrayBufferName);
                glBindBuffer (GL_ARRAY_BUFFER, LatticeBufferName);
                glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, IndexBufferName);
                glEnableVertexAttribArray (semantic::ico::POSITION);
                glEnableVertexAttribArray (semantic::ico::LATTICE);
                glVertexAttribDivisor (semantic::ico::LATTICE, 1); // instancing

                glPatchParameteri (GL_PATCH_VERTICES, 3);
                glDrawElementsInstanced (GL_PATCHES, IndicesCount, GL_UNSIGNED_INT, 0, LatticeCount);
 
                glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
                glBindBuffer (GL_ARRAY_BUFFER, 0);
                glBindVertexArray (0);

                return Validated && Surf3d::checkError("icosahedron::draw end");
        };
private:
        glf::vertex_v1i *indices;
        bool Validated;
	GLuint ArrayBufferName;
	GLuint IndexBufferName;
	GLuint VertexArrayName;
	GLuint LatticeBufferName;
	GLsizei LatticeCount;
	GLsizei VertexCount;
	GLsizei IndicesCount;
};



// ------------------------------------------------------------
// core GL configuration management
// ------------------------------------------------------------
class gl_400_3D_visualization{
public:
        gl_400_3D_visualization (GtkGLArea *area, Surf3d *surf){
                Validated = true;
                glarea = area;
                s = surf;
                Major=4;
                Minor=0;
                numx = numy = numv = 0;
                oversized_plane = 1.0;
                Surf3D_Z_Data = NULL;
                Surf3D_Palette = NULL;

                MouseOrigin  = glm::ivec2(0, 0);
                MouseCurrent = glm::ivec2(0, 0);

                double aspect = (s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx;

                TranslationOrigin  = glm::vec2(0, 0*aspect);
              	TranslationCurrent = TranslationOrigin;
                Translation3axis = glm::vec3(0.0f,0.0f,0.0f);

                DistanceOrigin  = glm::vec3(-60., 0., 0.);
              	DistanceCurrent = DistanceOrigin;


                RotationOrigin = glm::vec2(0.75,0);
                RotationCurrent = RotationOrigin;
                Rotation3axis = glm::vec3(0.0f,0.0f,0.0f);

                WindowSize  = glm::ivec2(500, 500);
                surface_plane = NULL;
                text_vao = NULL;
                ico_vao = NULL;
        };
        ~gl_400_3D_visualization(){
                //end(); // too late, glarea reference is gone! 
                if (surface_plane)
                        delete surface_plane;
                if (text_vao)
                        delete text_vao;
                if (ico_vao)
                        delete ico_vao;
        };

private:
        
        glm::vec3 cameraPosition() const {
                glm::vec3 R = lookAtPosition() + this->DistanceCurrent;
                return R;
        };
        glm::vec3 lookAtPosition() const {
                switch (s->GLv_data.look_at[0]){
                case 'C': return glm::vec3(0,0,0);
                case 'T':
                        {
                                GLfloat r[3];
                                s->GetXYZNormalized (r, s->GLv_data.height_scale_mode[0] == 'A');
                                glm::vec3 R = glm::vec3 (glm::vec4 (r[0], r[2], r[1], 1.0f) * glm::inverse (this->modelView()));
                                //glm::vec3 R = glm::vec3 (glm::vec4 (glm::vec3 (r[0], r[2], r[1]) + modelPosition(), 1.0f) * modelView());
                                return R;
                        }
                case 'M': return glm::vec3(0,0,1.0);
                }
                return glm::vec3(0,0,0);
        };
#if 0
        glm::vec3 modelPosition() const {
                return glm::vec3(-this->TranslationCurrent.x, 0.0, -this->TranslationCurrent.y);
        };
#endif
        glm::mat4 modelView() const {
                // rotate model 1st around it's origin
                glm::mat4 ModelRotateX = glm::rotate(glm::mat4(1.0f), -this->RotationCurrent.y, glm::vec3(1.f, 0.f, 0.f)); // X
                glm::mat4 ModelRotateY = glm::rotate(ModelRotateX, this->RotationCurrent.x, glm::vec3(0.f, 0.f, 1.f)); // GL Z is Screen depth = surface Y
                glm::mat4 ModelRotateZ = glm::rotate(ModelRotateY, this->Rotation3axis.z, glm::vec3(0.f, 1.f, 0.f)); // GL Y is Screen Y = surface Z (I hate it)
                return ModelRotateZ;
                // then translate
                //glm::mat4 ModelTranslate = glm::translate(ModelRotateZ,  modelPosition());
                // final ModelView
                //return ModelTranslate;
        };

	bool initProgram() {

                // load and init shader program if not yet created -- only once, may shared!
#if 0
                if (Validated && ProgramName_RefCount > 0){
                        g_message ("Shader Program is already initiated.");
                        ++ProgramName_RefCount;
                        return Validated;
                }
                
                g_message ("Loading, Compiling and Initializing Shader Programs.");
#endif

                if (Validated){
                        compiler Compiler;
                        GLuint VertexShader     = Compiler.create (GL_VERTEX_SHADER, getDataDirectory() + TESSELATION_VERTEX_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint ControlShader    = Compiler.create (GL_TESS_CONTROL_SHADER, getDataDirectory() + TESSELATION_CONTROL_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint EvaluationShader = Compiler.create (GL_TESS_EVALUATION_SHADER, getDataDirectory() + TESSELATION_EVALUATION_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint GeometryShader   = Compiler.create (GL_GEOMETRY_SHADER, getDataDirectory() + TESSELATION_GEOMETRY_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint FragmentShader   = Compiler.create (GL_FRAGMENT_SHADER, getDataDirectory() + TESSELATION_FRAGMENT_SHADER, CMD_ARGS_FOR_SHADERS);

                        if (!VertexShader || !ControlShader || !EvaluationShader || !GeometryShader || !FragmentShader)
                                Validated = false;
                        else {
                                Tesselation_ProgramName = glCreateProgram ();
                                glAttachShader (Tesselation_ProgramName, VertexShader);
                                glAttachShader (Tesselation_ProgramName, ControlShader);
                                glAttachShader (Tesselation_ProgramName, EvaluationShader);
                                glAttachShader (Tesselation_ProgramName, GeometryShader);
                                glAttachShader (Tesselation_ProgramName, FragmentShader);
                                glLinkProgram (Tesselation_ProgramName);
                                ++ProgramName_RefCount;
                        }
                        
			Validated = Validated && Compiler.check();
			Validated = Validated && Compiler.check_program(Tesselation_ProgramName);
		}

 		if (Validated){
                        compiler Compiler;
                        GLuint VertexShader     = Compiler.create (GL_VERTEX_SHADER, getDataDirectory() + S3D_VERTEX_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint FragmentShader   = Compiler.create (GL_FRAGMENT_SHADER, getDataDirectory() + S3D_FRAGMENT_SHADER, CMD_ARGS_FOR_SHADERS);

                        if (!VertexShader || !FragmentShader)
                                Validated = false;
                        else {
                                S3D_ProgramName = glCreateProgram ();
                                glAttachShader (S3D_ProgramName, VertexShader);
                                glAttachShader (S3D_ProgramName, FragmentShader);
                                glLinkProgram (S3D_ProgramName);
                        }
                        
			Validated = Validated && Compiler.check();
			Validated = Validated && Compiler.check_program(S3D_ProgramName);
		}

		if (Validated){
                        compiler Compiler;
                        GLuint VertexShader     = Compiler.create (GL_VERTEX_SHADER, getDataDirectory() + ICO_TESS_VERTEX_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint ControlShader    = Compiler.create (GL_TESS_CONTROL_SHADER, getDataDirectory() + ICO_TESS_CONTROL_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint EvaluationShader = Compiler.create (GL_TESS_EVALUATION_SHADER, getDataDirectory() + ICO_TESS_EVALUATION_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint GeometryShader   = Compiler.create (GL_GEOMETRY_SHADER, getDataDirectory() + ICO_TESS_GEOMETRY_SHADER, CMD_ARGS_FOR_SHADERS);
                        GLuint FragmentShader   = Compiler.create (GL_FRAGMENT_SHADER, getDataDirectory() + ICO_TESS_FRAGMENT_SHADER, CMD_ARGS_FOR_SHADERS);

                        if (!VertexShader || !ControlShader || !EvaluationShader || !GeometryShader || !FragmentShader)
                                Validated = false;
                        else {
                                IcoTess_ProgramName = glCreateProgram ();
                                glAttachShader (IcoTess_ProgramName, VertexShader);
                                glAttachShader (IcoTess_ProgramName, ControlShader);
                                glAttachShader (IcoTess_ProgramName, EvaluationShader);
                                glAttachShader (IcoTess_ProgramName, GeometryShader);
                                glAttachShader (IcoTess_ProgramName, FragmentShader);
                                glLinkProgram (IcoTess_ProgramName);
                        }
                        
			Validated = Validated && Compiler.check();
			Validated = Validated && Compiler.check_program(IcoTess_ProgramName);
		}

                

		if(Validated){
                        Uniform_screen_size   = glGetUniformLocation (Tesselation_ProgramName, "screen_size"); // vec2
                        Uniform_lod_factor    = glGetUniformLocation (Tesselation_ProgramName, "lod_factor");  // float
                        Uniform_tess_level    = glGetUniformLocation (Tesselation_ProgramName, "tess_level");  // float

                        Surf3d::checkError("initProgram -- Tesselation_ProgramName: get uniform variable references...");

                        Uniform_IcoTess_TessLevelInner = glGetUniformLocation (IcoTess_ProgramName, "TessLevelInner");
                        Uniform_IcoTess_TessLevelOuter = glGetUniformLocation (IcoTess_ProgramName, "TessLevelOuter");
                        Uniform_IcoTess_IcoPosition    = glGetUniformLocation (IcoTess_ProgramName, "IcoPosition");
                        Uniform_IcoTess_IcoScale       = glGetUniformLocation (IcoTess_ProgramName, "IcoScale");
                        Uniform_IcoTess_IcoColor       = glGetUniformLocation (IcoTess_ProgramName, "IcoColor");

                        Uniform_TipPosition  = glGetUniformLocation (IcoTess_ProgramName, "TipPosition");
                        Uniform_TipColor     = glGetUniformLocation (IcoTess_ProgramName, "TipColor");

                        Surf3d::checkError("initProgram -- IcoTess_ProgramName: get uniform variable references...");

                        // get shaderFunction references
                        // Specifies the shader stage from which to query for subroutine uniform index. shadertype must be one of GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER.

                        Uniform_vertexFlat        = glGetSubroutineIndex (Tesselation_ProgramName, GL_VERTEX_SHADER, "vertex_height_flat" );
                        Uniform_vertexDirect      = glGetSubroutineIndex (Tesselation_ProgramName, GL_VERTEX_SHADER, "vertex_height_direct" );
                        Uniform_vertexViewMode    = glGetSubroutineIndex (Tesselation_ProgramName, GL_VERTEX_SHADER, "vertex_height_z" );
                        Uniform_vertexY           = glGetSubroutineIndex (Tesselation_ProgramName, GL_VERTEX_SHADER, "vertex_height_y" );
                        Uniform_vertexXChannel    = glGetSubroutineIndex (Tesselation_ProgramName, GL_VERTEX_SHADER, "vertex_height_x" );
                        Uniform_vertex_plane_at   = glGetSubroutineIndex (Tesselation_ProgramName, GL_VERTEX_SHADER, "vertex_plane_at" );

                        Uniform_evaluationVertexFlat      = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "height_flat" );
                        Uniform_evaluationVertexDirect    = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "height_direct" );
                        Uniform_evaluationVertexXChannel  = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "height_z" );
                        Uniform_evaluationVertexY         = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "height_y" );
                        Uniform_evaluationVertexViewMode  = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "height_x" );
                        
                        Uniform_evaluationColorFlat       = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "colorFlat" );
                        Uniform_evaluationColorDirect     = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "colorDirect" );
                        Uniform_evaluationColorViewMode   = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "colorViewMode" );
                        Uniform_evaluationColorXChannel   = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "colorXChannel" );
                        Uniform_evaluationColorY          = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "colorY" );
                        Uniform_evaluation_vertex_XZplane = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "eval_vertex_XZ_plane" );
                        Uniform_evaluation_vertex_XYplane = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "eval_vertex_XY_plane" );
                        Uniform_evaluation_vertex_ZYplane = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "eval_vertex_ZY_plane" );
                        Uniform_evaluation_vertex_Mplane = glGetSubroutineIndex (Tesselation_ProgramName, GL_TESS_EVALUATION_SHADER, "eval_vertex_M_plane" );

                        Uniform_shadeTerrain      = glGetSubroutineIndex (Tesselation_ProgramName, GL_FRAGMENT_SHADER, "shadeTerrain" );
                        Uniform_shadeDebugMode    = glGetSubroutineIndex (Tesselation_ProgramName, GL_FRAGMENT_SHADER, "shadeDebugMode" );
                        Uniform_shadeLambertian   = glGetSubroutineIndex (Tesselation_ProgramName, GL_FRAGMENT_SHADER, "shadeLambertian" );
                        Uniform_shadeLambertianXColor  = glGetSubroutineIndex (Tesselation_ProgramName, GL_FRAGMENT_SHADER, "shadeLambertianXColor" );
                        Uniform_shadeLambertianMaterialColor = glGetSubroutineIndex (Tesselation_ProgramName, GL_FRAGMENT_SHADER, "shadeLambertianMaterialColor" );
                        Uniform_shadeLambertianMaterialColorFog = glGetSubroutineIndex (Tesselation_ProgramName, GL_FRAGMENT_SHADER, "shadeLambertianMaterialColorFog" );
                        Uniform_shadeVolume       = glGetSubroutineIndex (Tesselation_ProgramName, GL_FRAGMENT_SHADER, "shadeVolume" );

                        Surf3d::checkError("initProgram -- get uniform subroutine references...");
		}

                if (Validated)
                        return Validated && Surf3d::checkError("initProgram");

		return Validated;
	};

        void updateModelViewM(){
                glBindBuffer (GL_UNIFORM_BUFFER, Uniform_ubo_list[ModelViewMat_block]);
                glBufferData (GL_UNIFORM_BUFFER, sizeof (ubo::uniform_model_view), &Block_ModelViewMat, GL_STATIC_DRAW);
                glBindBuffer (GL_UNIFORM_BUFFER, 0);
        };
        
        void updateSurfaceGeometry(){
                glBindBuffer (GL_UNIFORM_BUFFER,  Uniform_ubo_list[SurfaceGeometry_block]);
                glBufferData (GL_UNIFORM_BUFFER, sizeof (ubo::uniform_surface_geometry), &Block_SurfaceGeometry, GL_STATIC_DRAW);
                glBindBuffer (GL_UNIFORM_BUFFER, 0);
        };
        
        void updateFragmentShading(){
                glBindBuffer (GL_UNIFORM_BUFFER,  Uniform_ubo_list[FragmentShading_block]);
                glBufferData (GL_UNIFORM_BUFFER, sizeof (ubo::uniform_fragment_shading), &Block_FragmentShading, GL_STATIC_DRAW);
                glBindBuffer (GL_UNIFORM_BUFFER, 0);
        };
        
	void bind_block (GLuint program, GLuint block_id, const gchar* block_name, GLsizei block_size){
                GLuint uniformBlockIndexProg = glGetUniformBlockIndex (program, block_name);
                glUniformBlockBinding (program, uniformBlockIndexProg, block_id);
                glBindBufferRange(GL_UNIFORM_BUFFER, block_id, Uniform_ubo_list[block_id], 0, block_size);
                g_message ("UBO[%d] (%s) = %d <=> %d  {%u}", block_id,  block_name, uniformBlockIndexProg, Uniform_ubo_list[block_id], block_size);
        };

	bool initBuffer() {
		if (!Validated) return false;

                // setup common UBOs
                
                glGenBuffers(3, &Uniform_ubo_list[0]);
                updateModelViewM ();
                updateSurfaceGeometry ();
                updateFragmentShading ();

                // interlink UBOs
                bind_block (Tesselation_ProgramName, ModelViewMat_block, "ModelViewMatrices", sizeof(ubo::uniform_model_view));
                bind_block (Tesselation_ProgramName, SurfaceGeometry_block, "SurfaceGeometry", sizeof(ubo::uniform_surface_geometry));
                bind_block (Tesselation_ProgramName, FragmentShading_block, "FragmentShading", sizeof(ubo::uniform_fragment_shading));

                bind_block (S3D_ProgramName, ModelViewMat_block, "ModelViewMatrices", sizeof(ubo::uniform_model_view));
                bind_block (S3D_ProgramName, SurfaceGeometry_block, "SurfaceGeometry", sizeof(ubo::uniform_surface_geometry));
                bind_block (S3D_ProgramName, FragmentShading_block, "FragmentShading", sizeof(ubo::uniform_fragment_shading));

                bind_block (IcoTess_ProgramName, ModelViewMat_block, "ModelViewMatrices", sizeof(ubo::uniform_model_view));
                bind_block (IcoTess_ProgramName, SurfaceGeometry_block, "SurfaceGeometry", sizeof(ubo::uniform_surface_geometry));
                bind_block (IcoTess_ProgramName, FragmentShading_block, "FragmentShading", sizeof(ubo::uniform_fragment_shading));
                
                // create surface base plane
                if (numx > 0 && numy > 0 && numv > 0 && Surf3D_Z_Data && Surf3D_Palette){
                        surface_plane = new base_plane (numx, numy, numv,
                                                        Surf3D_Z_Data, Surf3D_Palette, s->maxcolors,
                                                        (int)s->GLv_data.base_plane_size,
                                                        (s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx,
                                                        0 // (GLint)s->GLv_data.tex3d_lod
                                                        );
                } else {
                        Validated = false;
                        g_warning ("initBuffer -- Uninitialized surface data.");
                }

		return Surf3d::checkError("initBuffer");
	};

        bool initObjects() {
		if (!Validated) return false;

		Surf3d::checkError("initObjects");
		if (!Validated) return false;

                if (!text_vao)
                        text_vao = new text_plane ();

                if (!ico_vao)
                        ico_vao = new icosahedron ((GLsizei)s->GLv_data.probe_atoms);
                
		return Surf3d::checkError("initText Plane VAO");
        };
        
        static void GLMessageHandler (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam) { 
                g_message ("Source : %d; Type: %d; ID : %d; Severity : %d; length : %d\n==> %s",
                           source, type, id, severity, length,
                           message);
        };

        bool initDebugOutput()
        {
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
                glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
                glDebugMessageCallbackARB ((GLDEBUGPROCARB)&GLMessageHandler, NULL);

                return Surf3d::checkError("initDebugOutput");
        };

public:
	int version(int major, int minor) const{ return major * 100 + minor * 10; }
        bool checkExtension(char const * ExtensionName) const {
                GLint ExtensionCount = 0;
                glGetIntegerv(GL_NUM_EXTENSIONS, &ExtensionCount);
                for(GLint i = 0; i < ExtensionCount; ++i)
                        if(std::string((char const*)glGetStringi(GL_EXTENSIONS, i)) == std::string(ExtensionName))
                                return true;
                printf("Failed to find Extension: \"%s\"\n", ExtensionName);
                return false;
        };

        bool checkGLVersion(GLint MajorVersionRequire, GLint MinorVersionRequire) const {
                GLint MajorVersionContext = 0;
                GLint MinorVersionContext = 0;
                glGetIntegerv(GL_MAJOR_VERSION, &MajorVersionContext);
                glGetIntegerv(GL_MINOR_VERSION, &MinorVersionContext);
                printf("OpenGL Version Needed %d.%d ( %d.%d Found )\n",
                       MajorVersionRequire, MinorVersionRequire,
                       MajorVersionContext, MinorVersionContext);
                return version (MajorVersionContext, MinorVersionContext) 
                        >=  version (MajorVersionRequire, MinorVersionRequire);
        };

	bool begin() {
		if (!Validated) return false;

                /* we need to ensure that the GdkGLContext is set before calling GL API */
                gtk_gl_area_make_current (glarea);
 
                glewExperimental = GL_TRUE;
                glewInit();
                        
                glEnable (GL_DEPTH_TEST);
                
		if (version (Major, Minor) >= version (4, 0))
			Validated = checkGLVersion (Major, Minor);
                else
                        Validated = false;
                
                if(Validated)
                        Validated = initProgram();
                if(Validated)
                        Validated = initBuffer();
                if(Validated)
                        Validated = initObjects();
                
                if (Validated)
                        return Validated && Surf3d::checkError("begin");

                return Validated;
	};

	bool end() {
		if (!Validated) return false;

                /* we need to ensure that the GdkGLContext is set before calling GL API */
                gtk_gl_area_make_current (glarea);

                if (surface_plane){
                        delete surface_plane;
                        surface_plane = NULL;
                }
               
                g_message ("end -- check for Shader Programs unload. %d", ProgramName_RefCount);

                --ProgramName_RefCount;
                if (ProgramName_RefCount == 0){
                        g_message ("end -- unloading Shader Programs. %d", ProgramName_RefCount);
                
                        glDeleteProgram(Tesselation_ProgramName);
                        glDeleteProgram(S3D_ProgramName);
                        glDeleteProgram(IcoTess_ProgramName);

                        Tesselation_ProgramName = 0;
                        S3D_ProgramName = 0;
                        IcoTess_ProgramName = 0;
                }
		return Surf3d::checkError("end");
	};

        // used for height mapping
        bool updateTexture (GLint line, GLsizei num_lines=1) { 
		if (!Validated) return false;

                return surface_plane->update_displacement_data (Surf3D_Z_Data, line, num_lines);
        };

        gfloat z_world_to_normalized (double zw, GLfloat GL_hs=1.0f) {
                return zw/(GL_hs*(s->get_scan ())->mem2d->data->zrange*(s->get_scan ())->data.s.dz);
        };
        double z_normalized_to_world (double zn, GLfloat GL_hs=1.0f) {
                return zn/GL_hs*(s->get_scan ())->mem2d->data->zrange*(s->get_scan ())->data.s.dz;
        };
        
	bool render() {
		if (!Validated) return false;
                GLfloat GL_height_scale = 1.;
                
                switch (s->GLv_data.height_scale_mode[0]){
                case 'A':
                        GL_height_scale = s->GLv_data.hskl
                                * (s->get_scan ())->mem2d->data->zrange
                                * (s->get_scan ())->data.s.dz
                                / (s->get_scan ())->data.s.rx;
                        Block_SurfaceGeometry.aspect = glm::vec4 (1.0f,
                                                                  (s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx,
                                                                  s->GLv_data.hskl/GL_height_scale,
                                                                  1.);
                        break;
                case 'R':
                        GL_height_scale = s->GLv_data.hskl;
                        Block_SurfaceGeometry.aspect = glm::vec4 (1.0f,
                                                                  (s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx,
                                                                  1.,
                                                                  1.);
                        break;
                }

                glm::vec3 look_at = lookAtPosition ();
                glm::vec3 camera_position = cameraPosition ();
                
                if (s->GLv_data.vertex_source[0] == 'V'){ // auto override x,y to align view planes for Volume Projection
                        camera_position.x = 0.;
                        camera_position.y = 0.;
                }

                g_message ("Render (GL coord system):\n"
                           " Camera at = (%g, %g, %g)\n"
                           " Look at   = (%g, %g, %g)\n"
                           " Translate = (%g, 0, %g)\n"
                           " GL_height_scale = %g",
                           camera_position.x, camera_position.y, camera_position.z,
                           look_at.x, look_at.y, look_at.z,
                           TranslationCurrent.x, TranslationCurrent.y,
                           GL_height_scale
                           );
                
                glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glClearBufferfv (GL_COLOR, 0, s->GLv_data.clear_color);
                glPolygonMode (GL_FRONT_AND_BACK, s->GLv_data.Mesh ? GL_LINE : GL_FILL);

                if (s->GLv_data.Cull){
                        glEnable (GL_CULL_FACE);
                        glCullFace (GL_BACK);
                } else {
                        glDisable (GL_CULL_FACE);
                }

                float aspect = WindowSize.x/WindowSize.y;
                glm::mat4 Projection;
                if (s->GLv_data.Ortho){
                        GLfloat os=0.02;
                        Projection = glm::ortho (-os*s->GLv_data.fov, os*s->GLv_data.fov,
                                                 -os/aspect*s->GLv_data.fov, os/aspect*s->GLv_data.fov,
                                                 s->GLv_data.fnear, s->GLv_data.ffar);
                } else {
                        Projection = glm::perspective (glm::radians (s->GLv_data.fov), aspect, s->GLv_data.fnear, s->GLv_data.ffar); // near, far frustum
                }
                glm::mat4 Camera = glm::lookAt (camera_position, // cameraPosition, the position of your camera, in world space
                                                look_at, //cameraTarget, where you want to look at, in world space
                                                glm::vec3(0,1,0) // upVector, probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
                                                );
                
		glm::mat4 Model = glm::mat4(1.0f);
		Block_ModelViewMat.ModelView = this->modelView() * Model;
                // camera - lookAt
		//Block_ModelViewMat.ViewAlignedModelView = Camera * glm::mat4(1.0f); //glm::inverse (Block_ModelViewMat.ModelView);
                Block_ModelViewMat.ViewAlignedModelView = glm::rotate ( glm::inverse (this->modelView()), glm::radians(90.0f), glm::vec3(1.f, 0.f, 0.f)) ; 
		Block_ModelViewMat.ModelViewProjection = Projection * Camera * Block_ModelViewMat.ModelView;
                        
                /* clear the viewport; the viewport is automatically resized when
                 * the GtkGLArea gets a new size allocation
                 */

		glUseProgram (Tesselation_ProgramName);

                // Projection
                updateModelViewM ();

                // Geometry control
                if (s->GLv_data.vertex_source[0] == 'V')
                        Block_SurfaceGeometry.aspect = glm::vec4 (1.0f,
                                                                  (s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx,
                                                                  GL_height_scale*(s->get_scan ())->data.s.rz / (s->get_scan ())->data.s.rx,
                                                                  1.);
                // setting GLv_data.hskl to 1 means same x,y and z scale.
                Block_SurfaceGeometry.height_scale  = GL_height_scale;
                Block_SurfaceGeometry.height_offset = s->GLv_data.slice_offset;
                Block_SurfaceGeometry.plane_at = 0.;
                Block_SurfaceGeometry.cCenter = glm::vec4 (0.5f, 0.5f, 0.5f, 0.5f);
                Block_SurfaceGeometry.delta   = glm::vec2 (1.0f/(s->XPM_x-1), Block_SurfaceGeometry.aspect.y/(s->XPM_y-1));
                updateSurfaceGeometry ();

                // Camera, Light and Shading
#define MAKE_GLM_VEC3(V) glm::vec4(V[0],V[1],V[2],0)
#define MAKE_GLM_VEC4(V) glm::vec4(V[0],V[1],V[2],V[3])
#define MAKE_GLM_VEC4A(O,A) glm::vec4(O,O,O,A)
                Block_FragmentShading.lightDirWorld = MAKE_GLM_VEC3(s->GLv_data.light_position[0]);
                Block_FragmentShading.eyePosWorld   = glm::vec4(camera_position,0);

                Block_FragmentShading.sunColor      = MAKE_GLM_VEC3(s->GLv_data.light_specular[0]); // = vec3(1.0, 1.0, 0.7);
                Block_FragmentShading.specularColor = MAKE_GLM_VEC3(s->GLv_data.surf_mat_specular); // = vec3(1.0, 1.0, 0.7)*1.5;
                Block_FragmentShading.ambientColor  = MAKE_GLM_VEC3(s->GLv_data.light_global_ambient); // = vec3(0.05, 0.05, 0.15 );
                Block_FragmentShading.diffuseColor  = MAKE_GLM_VEC3(s->GLv_data.surf_mat_diffuse); // = vec3(1.0, 1.0, 0.7)*1.5;
                Block_FragmentShading.fogColor      = MAKE_GLM_VEC3(s->GLv_data.fog_color); // = vec3(0.7, 0.8, 1.0)*0.7;
                Block_FragmentShading.materialColor = MAKE_GLM_VEC4(s->GLv_data.surf_mat_color); // vec4
                Block_FragmentShading.backsideColor = MAKE_GLM_VEC4(s->GLv_data.surf_mat_backside_color); // vec4
                Block_FragmentShading.color_offset  = glm::vec4 (s->GLv_data.ColorOffset, s->GLv_data.ColorSat, 1., s->GLv_data.transparency_offset);

                Block_FragmentShading.fogExp = s->GLv_data.fog_density/100.; // = 0.1;

                Block_FragmentShading.shininess = 4.*(1.00001-s->GLv_data.surf_mat_shininess[0]/100.);
                Block_FragmentShading.lightness = s->GLv_data.Lightness;
                Block_FragmentShading.light_attenuation = 0.;

                Block_FragmentShading.transparency        = s->GLv_data.transparency;
                Block_FragmentShading.transparency_offset = s->GLv_data.transparency_offset;

                Block_FragmentShading.wrap = 0.3; // = 0.3;
                Block_FragmentShading.debug_color_source = (GLuint)s->GLv_data.shader_mode;
                updateFragmentShading ();

                // Tesseleation control -- lod is not yet used
                glUniform1f (Uniform_lod_factor, 4.0);
                glUniform1f (Uniform_tess_level, s->GLv_data.tess_level);
               
                Surf3d::checkError ("render -- set Uniforms, Blocks");

                // Specifies the shader stage from which to query for subroutine uniform index. shadertype must be one of GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER.

                // configure shaders for surface rendering modes (using tesselation) and select final shading mode

                switch (s->GLv_data.vertex_source[0]){
                case 'F': // Flat Vertex Height 0.5
                        Uniform_vertex_setup[0]     = Uniform_vertexFlat;
                        Uniform_evaluation_setup[0] = Uniform_evaluationVertexFlat;
                        break;
                case 'D': // Direct Vertex Height .a
                        Uniform_vertex_setup[0]     = Uniform_vertexDirect;
                        Uniform_evaluation_setup[0] = Uniform_evaluationVertexDirect;
                        break;
                case 'M': // View Mode Vertex Height .z
                        Uniform_vertex_setup[0]     = Uniform_vertexViewMode;
                        Uniform_evaluation_setup[0] = Uniform_evaluationVertexViewMode;
                        break;
                case 'C': // X-Channel Direct Vertex Height .x
                        Uniform_vertex_setup[0]     = Uniform_vertexXChannel;
                        Uniform_evaluation_setup[0] = Uniform_evaluationVertexXChannel;
                        break;
                case 'y': // "Y-Channel" .y
                        Uniform_vertex_setup[0]     = Uniform_vertexY;
                        Uniform_evaluation_setup[0] = Uniform_evaluationVertexY;
                        break;
                default: // other mode: slicing and volumetric rendering use "Flat Height"
                        Uniform_vertex_setup[0]     = Uniform_vertexFlat;
                        Uniform_evaluation_setup[0] = Uniform_evaluationVertexFlat;
                        break;
                }

                Surf3d::checkError ("render -- configure vertex_source");

                switch (s->GLv_data.ColorSrc[0]){
                case 'F': Uniform_evaluation_setup[1] = Uniform_evaluationColorFlat; break;
                case 'D': Uniform_evaluation_setup[1] = Uniform_evaluationColorDirect; break; // Direct Height Color
                case 'V': Uniform_evaluation_setup[1] = Uniform_evaluationColorViewMode; break; // View Mode Color
                case 'X': Uniform_evaluation_setup[1] = Uniform_evaluationColorXChannel; break; // X-Channel Direct Height Color
                case 'Y': Uniform_evaluation_setup[1] = Uniform_evaluationColorY; break; // X-Channel Direct Height Color
                default:  Uniform_evaluation_setup[1] = Uniform_evaluationColorDirect; break; // direct -- fall back
                }
                Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_XZplane;
                glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);

                Surf3d::checkError ("render -- configure color_source");
                
                switch (s->GLv_data.ShadeModel[0]){
                case 'L': glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeLambertian); break;
		case 'T': glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeTerrain); break;
                case 'M': glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeLambertianMaterialColor); break;
                case 'F': glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeLambertianMaterialColorFog); break;
                case 'R': glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeLambertianXColor); break;
                case 'V': glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeVolume); break;
		case 'D': glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeDebugMode); break;
                default: glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeLambertian); break;
                }
                Surf3d::checkError ("render -- configure shader");


                if (s->GLv_data.TransparentSlices){
                        glEnable (GL_BLEND); 
                        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                } else {
                        glDisable (GL_BLEND); 
                }

                if (s->GLv_data.vertex_source[0] != 'V' && oversized_plane > 1.01){
                        surface_plane->make_plane_vbo ((s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx);
                        surface_plane->update_vertex_buffer ();
                        oversized_plane = 1.0;
                }
                switch (s->GLv_data.vertex_source[0]){
                case 'X':
                        Uniform_vertex_setup[0] = Uniform_vertex_plane_at;
                        Block_SurfaceGeometry.plane_at = 0.5;
                        updateSurfaceGeometry ();
                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_ZYplane;
                        glUniformSubroutinesuiv (GL_VERTEX_SHADER, 1, Uniform_vertex_setup);
                        glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);
                        surface_plane->draw ();
                        break;
                case 'Y':
                        Uniform_vertex_setup[0] = Uniform_vertex_plane_at;
                        Block_SurfaceGeometry.plane_at = 0.5;
                        updateSurfaceGeometry ();
                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_XYplane;
                        glUniformSubroutinesuiv (GL_VERTEX_SHADER, 1, Uniform_vertex_setup);
                        glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);
                        surface_plane->draw ();
                        break;
                case 'S':
                        Uniform_vertex_setup[0] = Uniform_vertex_plane_at;
                        glUniformSubroutinesuiv (GL_VERTEX_SHADER, 1, Uniform_vertex_setup);
                        for (int i=0; i<6; ++i){
                                Block_SurfaceGeometry.height_scale  = 1.0f;
                                Block_SurfaceGeometry.height_offset = 0.0f;
                                Block_SurfaceGeometry.plane_at = (i%2 == 0 ? 0.:1.);
                                updateSurfaceGeometry ();
                                switch (i){
                                case 0:
                                case 1:
                                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_XZplane;
                                        break;
                                case 2:
                                case 3:
                                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_XYplane;
                                        break;
                                case 4:
                                case 5:
                                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_ZYplane;
                                        break;
                                }
                                glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);
                                surface_plane->draw ();
                        }
                        break;

                case 'V':
                        {
                                GLfloat r3  = 1.732f; // srqt(3)
                                GLfloat r3c = r3/2; // srqt(3)/2

                                // no culling, always use blending here
                                glDisable (GL_CULL_FACE);
                                glEnable (GL_BLEND); 
                                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        
                                if (oversized_plane < r3){
                                        surface_plane->make_plane_vbo ((s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx, r3);
                                        surface_plane->update_vertex_buffer ();
                                        oversized_plane = r3;
                                }
                        
                                Uniform_vertex_setup[0] = Uniform_vertex_plane_at;
                                glUniformSubroutinesuiv (GL_VERTEX_SHADER, 1, Uniform_vertex_setup);
                                glDisable (GL_DEPTH_TEST);
                        
                                Block_SurfaceGeometry.height_scale  = 1.0f;
                                Block_SurfaceGeometry.height_offset = 0.0f;
                                Block_SurfaceGeometry.cCenter = glm::vec4 (r3c, r3c, r3c, 0.5f);
                                for (int i=s->GLv_data.slice_start_n[0]; i<s->GLv_data.slice_start_n[1]; i += s->GLv_data.slice_start_n[2]){
                                        Block_SurfaceGeometry.plane_at = 0.001f*i;
                                        updateSurfaceGeometry ();
                                        //if (s->GLv_data.slice_plane_index[0]);
                                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_Mplane;
                                        glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);
                                        surface_plane->draw ();
                                }

                                glEnable (GL_DEPTH_TEST);
                        }
                        break;
                case 'Z':
                default:
                        glUniformSubroutinesuiv (GL_VERTEX_SHADER, 1, Uniform_vertex_setup);
                        surface_plane->draw ();
                        break;
                }

                glDisable (GL_BLEND); 

                Surf3d::checkError ("render -- done data plane(s)");

                
                // Gimmicks ========================================
                // Ico
                GLfloat tz=0.;
                if (ico_vao && s->GLv_data.tip_display[1] == 'n'){
#define MAKE_GLM_VEC4XS(V,X) glm::vec4(V[0],V[2],V[1],X)
                        Surf3d::checkError ("render -- draw ico");
                        glm::vec4 c[4] = {
                                MAKE_GLM_VEC3(s->GLv_data.tip_colors[0]),
                                MAKE_GLM_VEC3(s->GLv_data.tip_colors[1]),
                                MAKE_GLM_VEC3(s->GLv_data.tip_colors[2]),
                                MAKE_GLM_VEC3(s->GLv_data.tip_colors[3])
                        };

                        GLfloat r[3];
                        GLfloat scale[3];
                        // get tip position
                        double zabs = s->GetXYZNormalized (r, s->GLv_data.height_scale_mode[0] == 'A');
                        // get scale
                        s->GetXYZScale (scale, s->GLv_data.height_scale_mode[0] == 'A'); // get scaling factors Ang to GL-XYZ unit box
                        
                        glm::vec4 as       = MAKE_GLM_VEC4XS (scale, 1); // make vector
                        glm::vec4 tip_pos  = MAKE_GLM_VEC4XS (r,1) + as * MAKE_GLM_VEC4XS (s->GLv_data.tip_geometry[0], 0);   // GL coords XYZ: plane is XZ, Y is "up"
                        glm::vec4 tip_scaling = s->GLv_data.tip_geometry[0][3] * 0.5f * MAKE_GLM_VEC4XS (s->GLv_data.tip_geometry[1], 1) * as;

                        ico_vao->draw (tip_pos, tip_scaling, c, 4);
                        tz=tip_pos.y;
                        g_message ("Actual Tip Position = %g %g %g  r.z=%g  z_rel[A]=%g  z_abs[A]=%g", tip_pos.x, tip_pos.y, tip_pos.z, r[2], tz , zabs);
                        g_message ("Actual Tip Scale    = X%g Y%g Z%g   scale: %g %g %g", tip_scaling.x, tip_scaling.y, tip_scaling.z, scale[0], scale[1], scale[2]);
                }

                Surf3d::checkError ("render -- done gimmick ico_vao (tip)");
                
                // Annotations, Labels, ...
#define MAKE_GLM_VEC3X(V) glm::vec3(V[0],V[1],V[2])
                if (text_vao && (s->GLv_data.anno_show_title[1] == 'n' ||
                                 s->GLv_data.anno_show_axis_labels[1] == 'n' ||
                                 s->GLv_data.anno_show_axis_dimensions[1] == 'n' ||
                                 s->GLv_data.anno_show_bearings[1] == 'n')){
                        Surf3d::checkError ("render -- draw text");
                        glm::vec3 ex=glm::vec3 (0.1,0,0);
                        glm::vec3 ey=glm::vec3 (0,0.1,0);
                        glm::vec3 ez=glm::vec3 (0,0,0.1);
                        glm::vec3 exz = 0.5f*(ex+ez);
                        glm::vec3 eyz = -0.5f*(ex-ez);
                        glm::vec4 text_color = MAKE_GLM_VEC4 (s->GLv_data.anno_title_color);

                        if (text_vao && s->GLv_data.anno_show_title[1] == 'n')
                                text_vao->draw (s->GLv_data.anno_title, glm::vec3(0.5, -0.1, -0.75), 1.25f*ex, 1.25f*ez, text_color);

                        text_color = MAKE_GLM_VEC4 (s->GLv_data.anno_label_color);
                        {
                                if (text_vao && s->GLv_data.anno_show_axis_labels[1] == 'n')
                                        text_vao->draw (s->GLv_data.anno_xaxis, glm::vec3(0, 0, -0.6), ex, ez, text_color);
                                if (text_vao && s->GLv_data.anno_show_axis_dimensions[1] == 'n'){
                                        gchar *tmp = g_strdup ((s->get_scan ())->data.Xunit->UsrString ((s->get_scan ())->data.s.rx));
                                        text_vao->draw (tmp, glm::vec3(0.5, 0, -0.6), ex, ez, text_color);
                                        g_free (tmp);
                                }
                        }
                        {
                                if (text_vao && s->GLv_data.anno_show_axis_labels[1] == 'n')
                                        text_vao->draw (s->GLv_data.anno_yaxis, glm::vec3(0.53, 0, 0), -ez, ex, text_color);
                                if (text_vao && s->GLv_data.anno_show_axis_dimensions[1] == 'n'){
                                        gchar *tmp = g_strdup ((s->get_scan ())->data.Yunit->UsrString ((s->get_scan ())->data.s.ry));
                                        text_vao->draw (tmp, glm::vec3(0.53, 0, -0.5), -ez, ex, text_color);
                                        g_free (tmp);
                                }
                        }
                        {
                                if (text_vao && s->GLv_data.anno_show_axis_labels[1] == 'n')
                                        text_vao->draw (s->GLv_data.anno_zaxis, glm::vec3(0.53, 0, -0.6), -ey, 0.5f*(ex+ez), text_color);
                                if (text_vao && s->GLv_data.anno_show_axis_dimensions[1] == 'n'){
                                        gchar *tmp = g_strdup ((s->get_scan ())->data.Zunit->UsrString (0));
                                        text_vao->draw (tmp, glm::vec3(-0.53, 0, -0.53), exz, eyz, text_color);
                                        g_free (tmp);
                                        // Z-box upper
                                        tmp = g_strdup ((s->get_scan ())->data.Zunit->UsrString (z_normalized_to_world (0.5, GL_height_scale)));
                                        text_vao->draw (tmp, glm::vec3(-0.53, 0.5, -0.53), exz, eyz, text_color);
                                        g_free (tmp);
                                        // Z-surface upper
                                        tmp = g_strdup ((s->get_scan ())->data.Zunit->UsrString (z_normalized_to_world (0.5*GL_height_scale, GL_height_scale)));
                                        text_vao->draw (tmp, glm::vec3(-0.53, 0.5*GL_height_scale, -0.53), exz, eyz, text_color);
                                        g_free (tmp);
                                        // Z-surface lower
                                        tmp = g_strdup ((s->get_scan ())->data.Zunit->UsrString (z_normalized_to_world (-0.5*GL_height_scale, GL_height_scale)));
                                        text_vao->draw (tmp, glm::vec3(-0.53, -0.5*GL_height_scale, -0.53), exz, eyz, text_color);
                                        g_free (tmp);
                                        // Z-box lower
                                        tmp = g_strdup ((s->get_scan ())->data.Zunit->UsrString (z_normalized_to_world (-0.5, GL_height_scale)));
                                        text_vao->draw (tmp, glm::vec3(-0.53, -0.5, -0.53), exz, eyz, text_color);
                                        g_free (tmp);
                                        // Z-tip
                                        tmp = g_strdup ((s->get_scan ())->data.Zunit->UsrString (z_normalized_to_world (tz, GL_height_scale)));
                                        text_vao->draw (tmp, glm::vec3(-0.53, tz, -0.53), exz, eyz, text_color);
                                        g_free (tmp);
                                }
                        }
                        if (text_vao && s->GLv_data.anno_show_bearings[1] == 'n'){
                                text_vao->draw ("front", glm::vec3(0, 0, -0.5), ex, ey, text_color);
                                text_vao->draw ("top", glm::vec3(0, 0,  0.5), ex, ey, text_color); 
                                text_vao->draw ("left", glm::vec3(0.5, 0, 0), ez, ey, text_color);
                                text_vao->draw ("right", glm::vec3(-0.5, 0, 0), -ez, ey, text_color);
                        }
                }

                Surf3d::checkError ("render -- done gimmick ico_vao (tip)");

                // draw zero planes
                if (s->GLv_data.anno_show_zero_planes[1] == 'n'){
                        GLfloat r3  = 1.1f;
                        GLfloat r3c = r3/2;
                        glUseProgram (Tesselation_ProgramName);
                        
                        glDisable (GL_CULL_FACE);
                        glEnable (GL_BLEND); 
                        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glEnable (GL_DEPTH_TEST);

                        Block_FragmentShading.materialColor = MAKE_GLM_VEC4(s->GLv_data.anno_zero_plane_color); // vec4
                        Block_FragmentShading.backsideColor = MAKE_GLM_VEC4(s->GLv_data.anno_zero_plane_color); // vec4
                        updateFragmentShading ();                      
                        
                        Block_SurfaceGeometry.height_scale  = 1.0f;
                        Block_SurfaceGeometry.height_offset = 0.01f;
                        Block_SurfaceGeometry.plane_at = r3c;
                        Block_SurfaceGeometry.cCenter = glm::vec4 (r3c, r3c, r3c, 0.5f);
                        updateSurfaceGeometry ();

                        surface_plane->make_plane_vbo ((s->get_scan ())->data.s.ry / (s->get_scan ())->data.s.rx, r3);
                        surface_plane->update_vertex_buffer ();
                        oversized_plane = r3;
                        
                        Uniform_vertex_setup[0] = Uniform_vertex_plane_at;
                        glUniformSubroutinesuiv (GL_VERTEX_SHADER, 1, Uniform_vertex_setup);
                        glUniformSubroutinesuiv (GL_FRAGMENT_SHADER, 1, &Uniform_shadeLambertianMaterialColor);
                        
                        Uniform_evaluation_setup[0] = Uniform_evaluationVertexFlat;
                        Uniform_evaluation_setup[1] = Uniform_evaluationColorFlat;

                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_XZplane;
                        glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);
                        surface_plane->draw ();

                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_XYplane;
                        glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);
                        surface_plane->draw ();

                        Uniform_evaluation_setup[2] = Uniform_evaluation_vertex_ZYplane;
                        glUniformSubroutinesuiv (GL_TESS_EVALUATION_SHADER, 3, Uniform_evaluation_setup);
                        surface_plane->draw ();
                }
               
		return Surf3d::checkError("render done -- zero planes. End render.");
	};

        void resize (gint w, gint h){
                WindowSize  = glm::ivec2(w, h);
        };

        void cursorPositionCallback(int mouse, double x, double y){
                if (mouse == 'Z'){ // incremental
                        DistanceCurrent = (DistanceOrigin += 0.02f*glm::vec3(x, y, 0.));
                        return;
                }
                if (mouse == 'i'){
                        MouseOrigin = glm::ivec2(x, y);
                        return;
                }
                if (mouse == 't'){
                        TranslationOrigin = TranslationCurrent;
                        return;
                }
                if (mouse == 'r'){
                        RotationOrigin = RotationCurrent;
                        return;
                }
                if (mouse == 'm'){
                        DistanceOrigin = DistanceCurrent;
                        return;
                }

                MouseCurrent = glm::ivec2(x, y);
                glm::vec2 mouse_delta = glm::vec2(MouseCurrent - MouseOrigin);
                DistanceCurrent    = mouse == 'M' ? DistanceOrigin + glm::vec3(mouse_delta.x, 0., mouse_delta.y)/100.0f : DistanceOrigin;
                TranslationCurrent = mouse == 'T' ? TranslationOrigin + (mouse_delta / 1000.0f) : TranslationOrigin;
                RotationCurrent    = mouse == 'R' ? RotationOrigin + glm::radians(mouse_delta)/10.0f : RotationOrigin;
        };

        void get_rotation (float *wxyz){
                float dr = 180./M_PI;
                wxyz[0] = dr*RotationCurrent.x;
                wxyz[1] = -dr*RotationCurrent.y;
        };
        void set_rotation (float *wxyz){
                float dr = M_PI/180.;
                Rotation3axis = glm::vec3(wxyz[0]*dr, -wxyz[1]*dr, wxyz[2]*dr);
                RotationOrigin = glm::vec2(wxyz[0]*dr, -wxyz[1]*dr);
                cursorPositionCallback('x',0,0);
                //g_message ("Rx %f Ry %f", RotationOrigin.x,RotationOrigin.y);
        };
        void get_translation (float *rxyz){
                rxyz[0] = TranslationCurrent.x;
                rxyz[1] = TranslationCurrent.y;
        };
        void set_translation (float *rxyz){
                Translation3axis = glm::vec3(rxyz[0], rxyz[1], rxyz[2]);
                TranslationOrigin = glm::vec2(rxyz[0], rxyz[1]);
                cursorPositionCallback('x',0,0);
                //g_message ("Tx %f Ty %f", TranslationOrigin.x,TranslationOrigin.y);
        };
        void get_camera (float *d){
                d[0] = DistanceCurrent.x;
                d[1] = -DistanceCurrent.y;
                d[2] = -DistanceCurrent.z;
        };
        void set_camera (float *d){
                DistanceCurrent = DistanceOrigin = glm::vec3(d[0], -d[1], -d[2]);
        };

        void set_surface_data (glm::vec4 *data, glm::vec4 *palette, int nx, int ny, int nv){
                Surf3D_Z_Data = data;
                Surf3D_Palette = palette;
                numx = nx;
                numy = ny;
                numv = nv;
        };

private:
        bool Validated;
        Surf3d *s;
        GtkGLArea *glarea;
        int Major, Minor; // minimal version needed

        gfloat oversized_plane;
        base_plane *surface_plane;
        text_plane *text_vao;
        icosahedron *ico_vao;

        glm::vec2 WindowSize;
	glm::vec2 MouseOrigin;
	glm::vec2 MouseCurrent;
	glm::vec2 TranslationOrigin;
	glm::vec2 TranslationCurrent;
	glm::vec3 DistanceOrigin;
	glm::vec3 DistanceCurrent;
	glm::vec2 RotationOrigin;
	glm::vec2 RotationCurrent;
	glm::vec3 Rotation3axis;
	glm::vec3 Translation3axis;

        glm::vec4 *Surf3D_Z_Data;
        glm::vec4 *Surf3D_Palette;
	int numx, numy, numv;
};


#endif // HAVE_GLEW

// ------------------------------------------------------------
// Class Surf3d -- derived form generic GXSM view class
// handling 3D visulation modes and data prepartations
// using gtk-glarea provided via GUI part managed by app_v3dcontrol
// ------------------------------------------------------------

Surf3d::Surf3d(Scan *sc, int ChNo):View(sc, ChNo){
	XSM_DEBUG (GL_DEBUG_L2, "Surf3d::Surf3d(sc,ch)");
	v3dcontrol = NULL;
        self = this;
	GLvarinit();
        //	g_free (v3dControl_pref_def);
}

Surf3d::Surf3d():View(){
        XSM_DEBUG (GL_DEBUG_L2, "Surf3d::Surf3d()");
        v3dcontrol = NULL;
        self = this;
        GLvarinit();
}

Surf3d::~Surf3d(){
        XSM_DEBUG (GL_DEBUG_L2, "Surf3d::~");

        if (gl_tess)
                delete gl_tess;

        self = NULL;

        main_get_gapp ()->remove_configure_object_from_remote_list (v3dControl_pref_dlg);

        if (v3dControl_pref_dlg)
                gnome_res_destroy (v3dControl_pref_dlg);
        hide();

        delete_surface_buffer ();
        g_free (v3dControl_pref_def);
}

void  Surf3d::end_gl () {
#if 0
        if (gl_tess)
                delete gl_tess;
        gl_tess = NULL;
#endif
}        

void Surf3d::hide(){
	if (v3dcontrol)
		delete v3dcontrol;
	v3dcontrol = NULL;
	XSM_DEBUG (GL_DEBUG_L2, "Surf3d::hide");
}

void Surf3d::SaveImagePNG(GtkGLArea *glarea, const gchar *fname_png){
	mainprog_info minfo;
	unsigned char **rgb;
        guint32 *pixel_data;

        pixel_data = g_new (guint32, scrwidth*scrheight);
        gtk_gl_area_make_current (glarea);
        glReadPixels(0,0, scrwidth,scrheight,
                     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8,
                     pixel_data);

        minfo.gamma   = 2.2; // may use 2.2 ??
        minfo.width   = scrwidth;
        minfo.height  = scrheight;
        minfo.modtime = time(NULL);
        minfo.infile  = NULL;
        
        if (! (minfo.outfile = fopen(fname_png, "wb"))){
                return;
        }

        rgb = new unsigned char* [minfo.height];
        for (int i=0; i<minfo.height; rgb[i++] = new unsigned char[3*minfo.width]);

        guint32 *rgba = pixel_data;
        int k,j;
        for (int i=minfo.height-1; i>=0; --i)
                for (k=j=0; j<minfo.width; ++j, ++rgba){
                        *(rgb[i] + k++) = (unsigned char)((*rgba>> 8)&0xff);
                        *(rgb[i] + k++) = (unsigned char)((*rgba>>16)&0xff);
                        *(rgb[i] + k++) = (unsigned char)((*rgba>>24)&0xff);
                }

        g_free (pixel_data);
        
        minfo.row_pointers = rgb;
        minfo.title  = g_strdup ("GXSM-PNG-writer-via-glReadPixels-GL400-SCENE");
        minfo.author = g_strdup ("GXSM/Percy Zahl");
        minfo.desc   = g_strdup ("GXSM GL400 SCENE TO PNG export");
        minfo.copyright = g_strdup ("GPL");
        minfo.email   = g_strdup ("zahl@users.sourceforge.net");
        minfo.url     = g_strdup ("http://gxsm.sf.net");
        minfo.have_bg   = FALSE;
        minfo.have_time = FALSE;
        minfo.have_text = FALSE;
        minfo.pnmtype   = 6; // TYPE_RGB
        minfo.sample_depth = 8;
        minfo.interlaced= FALSE;

        writepng_init (&minfo);
        writepng_encode_image (&minfo);
        writepng_encode_finish (&minfo);
        writepng_cleanup (&minfo);
        fclose (minfo.outfile);

        g_free (minfo.title);
        g_free (minfo.author);
        g_free (minfo.desc);
        g_free (minfo.copyright);
        g_free (minfo.email);
        g_free (minfo.url);

        for (int i=0; i<minfo.height; delete[] rgb[i++]);
        delete[] rgb;
}

void Surf3d::GetXYZScale (float *s, gboolean z_scale_abs){
        s[0] = 1./scan->data.s.rx;
        s[1] = 1./scan->data.s.ry;
        if (z_scale_abs)
                s[2] = s[0]; // normalize to x scale
        else
                s[2] = 1./(scan->data.s.dz * scan->mem2d->data->zrange); // relative normalize to zrange
}

// XYZ ormalized to GL box +/- 0.5
double Surf3d::GetXYZNormalized(float *r, gboolean z_scale_abs){
        double x,y,z, zmin; 
        main_get_gapp ()->xsm->hardware->RTQuery ("R", z, x, y); // in Ang, absolute
        main_get_gapp ()->xsm->hardware->invTransform (&x, &y);
        zmin = scan->data.s.dz * scan->mem2d->data->zmin;
        // zmin = main_get_gapp ()->xsm->Inst->Dig2ZA (scan->mem2d->data->zmin);
        // g_message ("GetXYZ RTQuery: Z=%f X=%f Y=%f", z,x,y);
        x /= scan->data.s.rx; // Ang
        y /= scan->data.s.ry;
        r[0] = -x;
        r[1] = y;
        if (z_scale_abs)
                r[2] = (z-zmin) / scan->data.s.rx; // normalize to x scale
        else
                r[2] = (z-zmin) / (scan->data.s.dz * scan->mem2d->data->zrange);
                // r[2] = (z-zmin) / main_get_gapp ()->xsm->Inst->Dig2ZA (scan->mem2d->data->zrange);
        return z;
}

// Current in nA
double Surf3d::GetCurrent(){
        double v1,v2,v3;
        main_get_gapp ()->xsm->hardware->RTQuery ("f", v1, v2, v3);
        return v2;
}
// dF in Hz
double Surf3d::GetForce(){
        double v1,v2,v3;
        main_get_gapp ()->xsm->hardware->RTQuery ("f", v1, v2, v3);
        return v1;
}

void inline Surf3d::PutPointMode(int k, int j, int vi){
	int i;
	GLfloat val; //, xval;
        
	i = k + j*XPM_x + XPM_x*XPM_y*vi;
	if (i >= (int)size) return;

        // check for zero range
        if (! (scan->mem2d->data->zrange > 0.)){
                surface_z_data_buffer[i].x=0.;
                surface_z_data_buffer[i].y=0.;
                surface_z_data_buffer[i].z=0.;
                surface_z_data_buffer[i].w=0.;
                return;
        }

        // W CHANNEL <= surface height raw 0..1 adjusted 
        surface_z_data_buffer[i].w = (scan->mem2d->GetDataPkt(k,j,vi)-scan->mem2d->data->zmin)/scan->mem2d->data->zrange; // W component: Z=Height-value raw, scaled to 0..1

        // Z CHANNEL <= surface height via view transform function
        val = scan->mem2d->GetDataVMode (k,j,vi)/scan->mem2d->GetDataRange ();
        surface_z_data_buffer[i].z = val >= 0. ? val <= 1. ? val : 1. : 0.; // Clamp Z via view transform function

        // normal calculation on GPU based on height map -- obsolete now here
	// scan->mem2d->GetDataPkt_vec_normal_4F (k,j,vi, &surface_z_data_buffer[i], 1.0, XPM_x*GLv_data.hskl/scan->mem2d->data->zrange);

        // X CHANNEL <= UV-Map to index range of GXSM SCAN CHANNEL-X:

        if (mem2d_x){
                if (mem2d_x->GetNx () == scan->mem2d->GetNx () &&
                    mem2d_x->GetNy () == scan->mem2d->GetNy () &&
                    mem2d_x->GetNv () == scan->mem2d->GetNv ())
                        surface_z_data_buffer[i].x = (GLfloat) ((mem2d_x->GetDataPkt(k,j,vi)-mem2d_x->data->zmin)/mem2d_x->data->zrange);
                else {
                        int u = (int) (k * (double)mem2d_x->GetNx () / (double)scan->mem2d->GetNx ());
                        int v = (int) (j * (double)mem2d_x->GetNy () / (double)scan->mem2d->GetNy ());
                        int w = (int) (vi * (double)mem2d_x->GetNv () / (double)scan->mem2d->GetNv ());
                        surface_z_data_buffer[i].x = (GLfloat) (mem2d_x->GetDataPkt (u,v, w)/mem2d_x->GetDataRange ());
                }
        } else {
                surface_z_data_buffer[i].x = 0.5;
        }

        surface_z_data_buffer[i].y = val; // Y-Channel still available

        // push edges to zero?
        if (1){ //GLv_data.TickFrameOptions[0]=='2' || GLv_data.TickFrameOptions[0]=='3' ){
                if (k==0 || j==0 || k == XPM_x-1 || j == XPM_y-1){
                        surface_z_data_buffer[i].w = 0.;
                        surface_z_data_buffer[i].x = 0.;
                        surface_z_data_buffer[i].y = 0.;
                        surface_z_data_buffer[i].z = 0.;
                }
        }
}


void Surf3d::GLvarinit(){
	mem2d_x=NULL;
        gl_tess = NULL;
	
	XPM_x = XPM_y = 0;
	scrwidth=500;
	scrheight=500;
	ZoomFac=1;
	size=0;
        surface_z_data_buffer = NULL;
        GLv_data.camera[0] = 0.; // fixed
        GLv_data.fnear = 0.001;
        GLv_data.ffar  = 100.0;
        
	ReadPalette (xsmres.Palette);

// create preferences table from static table and replace pointers to
// functions with callbacks of this instance
	GnomeResEntryInfoType *res = v3dControl_pref_def_const;
	int n;
	for (n=0; res->type != GNOME_RES_LAST; ++res, ++n); 
	++n; // include last!

	v3dControl_pref_def = g_new (GnomeResEntryInfoType, n);

	res = v3dControl_pref_def_const;
	GnomeResEntryInfoType *rescopy = v3dControl_pref_def;
	for (; res->type != GNOME_RES_LAST; ++res, ++rescopy){
		memcpy (rescopy, res, sizeof (GnomeResEntryInfoType));
		rescopy->var = (void*)((long)res->var + (long)(&GLv_data));
		rescopy->changed_callback = GLupdate;
		rescopy->moreinfo = (gpointer) this;
	}
	memcpy (rescopy, res, sizeof (GnomeResEntryInfoType)); // do last!

	// read user values or make defaults if not present
	v3dControl_pref_dlg = gnome_res_preferences_new (v3dControl_pref_def, GXSM_RES_GL_PATH);
	gnome_res_set_apply_callback (v3dControl_pref_dlg, GLupdate, (gpointer)this);
	gnome_res_set_destroy_on_close (v3dControl_pref_dlg, FALSE);
	gnome_res_set_auto_apply (v3dControl_pref_dlg, TRUE);
	gnome_res_set_height (v3dControl_pref_dlg, 700);
	gnome_res_read_user_config (v3dControl_pref_dlg);
        main_get_gapp ()->add_configure_object_to_remote_list (v3dControl_pref_dlg);
}

void Surf3d::GLupdate (void* data){
        static float prev_index=0.;
        XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::GLUPDATE" << std::flush);

        if (data){
        
                Surf3d *s = (Surf3d *) data;
                if (!s->gl_tess)
                        return;

                s->check_dimension_changed ();

                XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::GLUPDATE rerender" << std::flush);
                if (s->gl_tess){
                        // *ViewPreset_OptionsList[] = { "Manual", "Top", "Front", "Left", "Right", "Areal View Front", "Scan: Auto Tip View", NULL };

                        switch (s->GLv_data.view_preset[0]){
                        case '*':
                        case 'M' : break;
                        case 'T' :
                                s->GLv_data.rot[0] = 0; s->GLv_data.rot[1] = -90; s->GLv_data.rot[2] = 0;
                                s->GLv_data.trans[0] = s->GLv_data.trans[1] = s->GLv_data.trans[2] = 0.;
                                s->UpdateGLv_control(); s->GLv_data.view_preset[0]='*';
                                break;
                        case 'F' :
                                s->GLv_data.rot[0] = 0; s->GLv_data.rot[1] = 0; s->GLv_data.rot[2] = 0;
                                s->GLv_data.trans[0] = s->GLv_data.trans[1] = s->GLv_data.trans[2] = 0.;
                                s->UpdateGLv_control(); s->GLv_data.view_preset[0]='*';
                                break;
                        case 'L' :
                                s->GLv_data.rot[0] = 0; s->GLv_data.rot[1] = 0; s->GLv_data.rot[2] = 90.;
                                s->GLv_data.trans[0] = s->GLv_data.trans[1] = s->GLv_data.trans[2] = 0.;
                                s->UpdateGLv_control(); s->GLv_data.view_preset[0]='*';
                                break;
                        case 'R' :
                                s->GLv_data.rot[0] = 0; s->GLv_data.rot[1] = 0; s->GLv_data.rot[2] = -90.;
                                s->GLv_data.trans[0] = s->GLv_data.trans[1] = s->GLv_data.trans[2] = 0.;
                                s->UpdateGLv_control(); s->GLv_data.view_preset[0]='*';
                                break;
                        case 'A' :
                                s->GLv_data.rot[0] = 0; s->GLv_data.rot[1] = -40; s->GLv_data.rot[2] = 0.;
                                s->GLv_data.trans[0] = s->GLv_data.trans[1] = s->GLv_data.trans[2] = 0.;
                                s->UpdateGLv_control();  s->GLv_data.view_preset[0]='*';
                                break;
                        case 'S':
                                s->GetXYZNormalized (s->GLv_data.trans, s->GLv_data.height_scale_mode[0] == 'A');
                                s->UpdateGLv_control();
                                break;
                        default: break;
                        }
                        // g_message ("GLU: %s %g %g", s->GLv_data.view_preset, s->GLv_data.rot[0],s->GLv_data.rot[1]);
                        s->gl_tess->set_rotation (s->GLv_data.rot);
                        s->gl_tess->set_translation (s->GLv_data.trans);
                        s->gl_tess->set_camera (s->GLv_data.camera);
                }
                if (s->v3dcontrol)
                        s->v3dcontrol->rerender_scene ();

                // automated output file generation, control fields via python remote console to generate movie!
                if (s->GLv_data.animation_index > 0. && s->GLv_data.animation_index > prev_index){
                        gchar *tmp = g_strdup_printf (s->GLv_data.animation_file, s->GLv_data.animation_index);
                        g_message ("Frame Output: %s", tmp);
                        s->SaveImagePNG (GTK_GL_AREA (s->v3dcontrol->get_glarea ()), tmp);
                        g_free (tmp);
                }
                prev_index = s->GLv_data.animation_index;
        }


        
        XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::GLUPDATE done." << std::flush);
}

void Surf3d::set_gl_data (){
        if (gl_tess){
                XSM_DEBUG (GL_DEBUG_L2, "Surf3d::set_gl_data." << std::flush);
                delete_surface_buffer ();
                create_surface_buffer ();
                gl_tess->set_surface_data (surface_z_data_buffer, ColorLookup, XPM_x, XPM_y, XPM_v);
                XSM_DEBUG (GL_DEBUG_L2, "Surf3d::set_gl_data OK." << std::flush);
        }
}

gboolean Surf3d::check_dimension_changed(){
        if (XPM_x != scan->mem2d->GetNx() || XPM_y != scan->mem2d->GetNy() || XPM_v != scan->mem2d->GetNv()){
                g_message ("Reshaping GL scene/surface");
                if (gl_tess){
                        g_message ("Calling gl_tess->end()");
                        gl_tess->end ();

                        g_message ("Recalculating buffers");
                        set_gl_data ();
        
                        g_message ("Calling  gl_tess->begin()");
                        if (! gl_tess->begin()){
                                gchar *message = g_strdup_printf
                                        ("FAILURE GL-TESS BEGIN/INIT failed at check_dimension_changed:\n"
                                         " --> GL VERSION requirements for GL 4.0 not satified?\n"
                                         " --> GLSL  program code error or not found/installed?\n"
                                         "     check: %s for .glsl files.", getDataDirectory().c_str());

                                g_critical ("%s", message);
                                main_get_gapp ()->warning (message);
                                g_free (message);

                                delete gl_tess;
                                gl_tess = NULL;
                        }
                }
                return true;
        } else
                return false;

}

void Surf3d::create_surface_buffer(){ 
	XSM_DEBUG (GL_DEBUG_L2, "Surf3d::create_surface_buffer");

	if (v3dcontrol){
		gchar *titel = g_strdup_printf ("GL: Ch%d: %s TF%d %s", 
						ChanNo+1, data->ui.name, 
						(int)GLv_data.tess_level,
                                                scan->mem2d->GetEname());
		
		v3dcontrol->SetTitle(titel);
		g_free(titel);
	}

        mem2d_x=NULL;
        int ChSrcX=main_get_gapp ()->xsm->FindChan (ID_CH_M_X);
        if (ChSrcX >= 0){
                if(main_get_gapp ()->xsm->scan[ChSrcX])
                        mem2d_x = main_get_gapp ()->xsm->scan[ChSrcX]->mem2d;

        } else
                if(main_get_gapp ()->xsm->scan[0])
                        mem2d_x = main_get_gapp ()->xsm->scan[0]->mem2d;

	XPM_x = scan->mem2d->GetNx();
	XPM_y = scan->mem2d->GetNy();
	XPM_v = scan->mem2d->GetNv();

	size = XPM_x * XPM_y * XPM_v;

	XSM_DEBUG_GP (GL_DEBUG_L3, "Surf3d::create_surface_buffer  **nXYV (%d, %d, %d)\n", XPM_x, XPM_y, XPM_v);

        // renew if exisits
        g_free (surface_z_data_buffer);
        surface_z_data_buffer = g_new (glm::vec4, size * sizeof(glm::vec4));

	XSM_DEBUG (GL_DEBUG_L3, "Surf3d::create_surface_buffer  ** g_new completed");

	setup_data_transformation();

	XSM_DEBUG (GL_DEBUG_L3, "Surf3d::create_surface_buffer  ** computing surface and normals");
        // grab and prepare data buffers
        if (main_get_gapp ()->xsm->scan[0])
                main_get_gapp ()->xsm->scan[0]->mem2d->data->update_ranges (0);

        scan->mem2d->data->update_ranges (0);
        for(int v=0; v<scan->mem2d->GetNv(); ++v)
                scan->mem2d->data->update_ranges (v, true);
        
        for(int v=0; v<scan->mem2d->GetNv(); ++v){
		for(int j=0; j<scan->mem2d->GetNy(); ++j)
			for(int k=0; k<scan->mem2d->GetNx(); ++k)
				PutPointMode (k,j,v);
        }
	XSM_DEBUG (GL_DEBUG_L3, "Surf3d::create_surface_buffer  ** completed");
}

void Surf3d::delete_surface_buffer(){ 
	XSM_DEBUG (GL_DEBUG_L2, "SURF3D::delete_surface_buffer -- check for cleanup");

	if (surface_z_data_buffer){
                XSM_DEBUG (GL_DEBUG_L2, "SURF3D::delete_surface_buffer -- g_free: normal_z_buffer");
		g_free (surface_z_data_buffer);
                surface_z_data_buffer = NULL;
        }

	size = 0;
}

void Surf3d::printstring(void *font, char *string)
{
 	int len,i;
 	len = (int)strlen(string);
 	for(i=0; i<len; i++)
 		;//glutBitmapCharacter(font,string[i]);
}

void Surf3d::MouseControl (int mouse, double x, double y){
        if (gl_tess){
                gl_tess->cursorPositionCallback(mouse, x, y);
                gl_tess->get_rotation (GLv_data.rot);
                gl_tess->get_translation (GLv_data.trans);
                gl_tess->get_camera (GLv_data.camera);
                UpdateGLv_control();
        }
}

void Surf3d::RotateAbs(int n, double phi){
	GLv_data.rot[n] = phi;
        if (gl_tess)
                gl_tess->set_rotation (GLv_data.rot);
	if (v3dcontrol)
                v3dcontrol->rerender_scene ();
}

void Surf3d::Rotate(int n, double dphi){
	if(n>2){
		GLv_data.rot[1] += dphi;
		GLv_data.rot[2] += dphi;
	}
	else
		GLv_data.rot[n] += dphi;

        if (gl_tess)
                gl_tess->set_rotation (GLv_data.rot);
}

double Surf3d::RotateX(double dphi){
	GLv_data.rot[0] += 5*dphi;
        if (gl_tess)
                gl_tess->set_rotation (GLv_data.rot);
	if (v3dcontrol)
                v3dcontrol->rerender_scene ();

	return GLv_data.rot[0];
}

double Surf3d::RotateY(double dphi){
	GLv_data.rot[1] += 5*dphi;
        if (gl_tess)
                gl_tess->set_rotation (GLv_data.rot);
	if (v3dcontrol)
                v3dcontrol->rerender_scene ();

	return GLv_data.rot[1];
}

double Surf3d::RotateZ(double dphi){
	GLv_data.rot[2] += 5*dphi;
        if (gl_tess)
                gl_tess->set_rotation (GLv_data.rot);
	if (v3dcontrol)
                v3dcontrol->rerender_scene ();

	return GLv_data.rot[2];
}

void Surf3d::Translate(int n, double delta){
        GLv_data.trans[n] += 0.02*delta;
        if (gl_tess)
                gl_tess->set_translation (GLv_data.trans);
	if (v3dcontrol)
                v3dcontrol->rerender_scene ();
}

double Surf3d::Zoom(double x){ 
	GLv_data.camera[1] += 0.02*x; 
        if (gl_tess)
                gl_tess->set_camera (GLv_data.camera);
	if (v3dcontrol){
                v3dcontrol->rerender_scene ();
                v3dControl_pref_dlg->block = TRUE;
                gnome_res_update_all (v3dControl_pref_dlg);
                v3dControl_pref_dlg->block = FALSE;
        }
	return GLv_data.camera[1]; 
}

double Surf3d::HeightSkl(double x){ 
	if (v3dcontrol){
                GLv_data.hskl += 0.02*x; 
                v3dControl_pref_dlg->block = TRUE;
                gnome_res_update_all (v3dControl_pref_dlg);
                v3dControl_pref_dlg->block = FALSE;
                draw(NULL);
        }
	return GLv_data.hskl; 
}

void Surf3d::ColorSrc(){
}

void Surf3d::GLModes(int n, int m){
	switch(n){
	case ID_GL_nZP: 
		GLv_data.ZeroPlane=m?1:0; break;
	case ID_GL_Mesh: 
		GLv_data.Mesh=m?1:0; break;
	case ID_GL_Ticks: 
		GLv_data.Ticks=m?1:0; break;
	case ID_GL_Cull: 
		GLv_data.Cull=m?1:0; break;
	case ID_GL_Smooth: 
		GLv_data.Smooth=m?1:0; break;
	}  
	if (v3dcontrol)
                v3dcontrol->rerender_scene ();
}


void Surf3d::ReadPalette(char *name){
        maxcolors = GXSM_GPU_PALETTE_ENTRIES;
	if (name){
		std::ifstream cpal;
		char pline[256];
		int nx,ny;
		cpal.open(name, std::ios::in);
		if(cpal.good()){
			cpal.getline(pline, 255);
			cpal.getline(pline, 255);
			cpal >> nx >> ny;
			cpal.getline(pline, 255);
			cpal.getline(pline, 255);

                        maxcolors = MIN(nx, 8192);

			for (int i=0; i<maxcolors; ++i){
				int r,g,b;
				cpal >> r >> g >> b;
				ColorLookup[i][0] = r/255.;
				ColorLookup[i][1] = g/255.;
				ColorLookup[i][2] = b/255.;
			}
			return;
		}
	}
	// default grey and fallback mode:
        maxcolors = 4096;
	for (int i=0; i<maxcolors; ++i)
		ColorLookup[i][0] = ColorLookup[i][1] = ColorLookup[i][2] = i/(double)maxcolors;
}

// height in [0..1] expected!
void Surf3d::calccolor(GLfloat height, glm::vec4 &c)
{
	GLfloat color[4][3]={
		{1.0,1.0,1.0},
		{0.0,0.8,0.0},
		{1.0,1.0,0.3},
		{0.0,0.0,0.8}
	};
	GLfloat fact;
	
	if(height>=0.9) {
		c = glm::vec4 (color[0][0], color[0][1], color[0][2], 1);
		return;
	}
	
	if((height<0.9) && (height>=0.7)) {
		fact=(height-0.7)*5.0;
		c = glm::vec4 (fact*color[0][0]+(1.0-fact)*color[1][0],
                               fact*color[0][1]+(1.0-fact)*color[1][1],
                               fact*color[0][2]+(1.0-fact)*color[1][2],
                               1);
		return;
	}
	
	if((height<0.7) && (height>=0.6)) {
		fact=(height-0.6)*10.0;
		c = glm::vec4 (fact*color[1][0]+(1.0-fact)*color[2][0],
                               fact*color[1][1]+(1.0-fact)*color[2][1],
                               fact*color[1][2]+(1.0-fact)*color[2][2],
                               1);
                return;
	}
	
	if((height<0.6) && (height>=0.5)) {
		fact=(height-0.5)*10.0;
		c = glm::vec4 (fact*color[2][0]+(1.0-fact)*color[3][0],
                               fact*color[2][1]+(1.0-fact)*color[3][1],
                               fact*color[2][2]+(1.0-fact)*color[3][2],
                               1);
		return;
	}
	
	c = glm::vec4 (color[3][0], color[3][1], color[3][2], 1);
}


void Surf3d::setup_data_transformation(){
	scan->mem2d->SetDataPktMode (data->display.ViewFlg);
	scan->mem2d->SetDataRange (0, maxcolors);
}

int Surf3d::update(int y1, int y2){
	XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::update y1 y2: " << y1 << "," << y2);

        if (y1 == 0)
                GLupdate (this);

        
	if (!scan || size == 0) return -1;

	XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::update trafo");
	setup_data_transformation();

        if (GLv_data.ZeroOld && y1 == 0){
                for(int v=0; v<scan->mem2d->GetNv(); ++v)
                        for(int j=y2; j < scan->mem2d->GetNy (); ++j)
                                for(int k=0; k < scan->mem2d->GetNx (); ++k){
                                        int i = k + j*XPM_x + XPM_x*XPM_y*v;
                                        surface_z_data_buffer[i].x=0.;
                                        surface_z_data_buffer[i].y=0.;
                                        surface_z_data_buffer[i].z=0.;
                                        surface_z_data_buffer[i].w=0.;
                                }
                if (gl_tess)
                        gl_tess->updateTexture (0, scan->mem2d->GetNy ());
        }
	XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::update data");
//	int v = scan->mem2d->GetLayer();
	for(int v=0; v<scan->mem2d->GetNv(); ++v){
                if (mem2d_x)
                        mem2d_x->data->update_ranges (v);
                if (scan->mem2d->data->update_ranges (v)){
                        // ranges changes, need to recalculate all
                        y1=0;
                        if (!GLv_data.ZeroOld) y2=scan->mem2d->GetNy();
                }
		for(int j=y1; j < y2; ++j)
			for(int k=0; k < scan->mem2d->GetNx (); ++k)
				PutPointMode (k,j,v);
	}

        if (v3dcontrol){
                if (gl_tess)
                        gl_tess->updateTexture (y1, y2-y1);
                v3dcontrol->start_auto_update();
                //v3dcontrol->rerender_scene ();
        }
        
        XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::update done.");
	return 0;
}



/* 
 * Tickmarks and other stuff....
 */

void Surf3d::GLdrawGimmicks(){
        ;
}


/* 
   Tesselate surface, ...
*/

void Surf3d::GLdrawsurface(int y_to_update, int refresh_all){
        if (gl_tess){
                if (refresh_all)
                        gl_tess->updateTexture (0, XPM_y);
                else
                        gl_tess->updateTexture (y_to_update);
        }
}



void realize_vsurf3d_cb (GtkGLArea *area, Surf3d *s){
	XSM_DEBUG (GL_DEBUG_L2, "GL:::REALIZE-EVENT");
        // We need to make the context current if we want to
        // call GL API

        // If there were errors during the initialization or
        // when trying to make the context current, this
        // function will return a #GError for you to catch

        if (gtk_gl_area_get_error (area) != NULL)
                return;

        s->gl_tess = new gl_400_3D_visualization (area, s);
        s->set_gl_data ();
        
	if (! s->gl_tess->begin()){
                gchar *message = g_strdup_printf
                        ("FAILURE GL-TESS BEGIN/INIT failed at realize_vsurf3d_cb:\n"
                         " --> GL VERSION requirements for GL 4.0 not satified?\n"
                         " --> GLSL  program code error or not found/installed?\n"
                         "     check: %s for .glsl files.", getDataDirectory().c_str());

                g_critical ("%s",message);
                main_get_gapp ()->warning (message);
                g_free (message);

                delete s->gl_tess;
                s->gl_tess = NULL;
        }
        XSM_DEBUG (GL_DEBUG_L2, "GL:::REALIZE-EVENT  (realize_vsurf3d_cb) completed.");
}


// =========== GTK-GL-AREA CALLBACKS =================
// The “resize” signal handler
void resize_vsurf3d_cb (GtkGLArea *area,
                        gint       width,
                        gint       height,
                        Surf3d *s)
{
	XSM_DEBUG (GL_DEBUG_L2, "GL:::RESIZE-EVENT");
        if (!s) return;
        // if (!s->is_ready()) return;
	XSM_DEBUG (GL_DEBUG_L2, "GL:::RESIZE-EVENT -- updating");

        s->scrwidth=width;
        s->scrheight=height;
        if (s->gl_tess)
                s->gl_tess->resize (width, height);
}

// render_event :    redraw scene

static gboolean
render_vsurf3d_cb (GtkGLArea *area, GdkGLContext *context, Surf3d *s)
{
        XSM_DEBUG (GL_DEBUG_L2, "GL:::RENDER-EVENT enter");

        if (!s) return FALSE;
        // if (!s->is_ready()) return FALSE;
        if (!s->gl_tess) return FALSE;
        
        XSM_DEBUG (GL_DEBUG_L2, "GL:::RENDER-EVENT -- execute GPU tesseleation");

        //s->gl_tess->set_rotation (s->GLv_data.rot);
        //s->gl_tess->get_translation (s->GLv_data.trans);
        return s->gl_tess->render ();
}


int Surf3d::draw(int zoomoverride){

	XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::DRAW");
 
	if (!scan->mem2d) { 
		XSM_DEBUG (GL_DEBUG_L2, "Surf3d: no mem2d !"); 
		return 1; 
	}
	
	if ( !v3dcontrol ){
                if (!scan->get_app ()){
                        g_warning ("ERROR Grey2D::DRAW -- invalid request: no app reference provided.");
                        return -1;
                }

		v3dcontrol = new V3dControl (scan->get_app (),
                                             "3D Surface View (using GL/GPU)", ChanNo, scan,
					     G_CALLBACK (resize_vsurf3d_cb),
					     G_CALLBACK (render_vsurf3d_cb),
                                             G_CALLBACK (realize_vsurf3d_cb),
					     self);
        }
        v3dcontrol->rerender_scene ();
        
	return 0;
}

void Surf3d::preferences(){
	XSM_DEBUG (GL_DEBUG_L2, "SURF3D:::PREFERENCES");

	if (v3dControl_pref_dlg)
		gnome_res_run_change_user_config (v3dControl_pref_dlg, "GL Scene Setup");
}











bool Surf3d::checkError(const char* Title)
{
	int Error;
	if((Error = glGetError()) != GL_NO_ERROR)
	{
		std::string ErrorString;
		switch(Error)
		{
		case GL_INVALID_ENUM:
			ErrorString = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			ErrorString = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			ErrorString = "GL_INVALID_OPERATION";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			ErrorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			ErrorString = "GL_OUT_OF_MEMORY";
			break;
		default:
			ErrorString = "UNKNOWN";
			break;
		}
		
		gchar *message = g_strdup_printf ("OpenGL Error (%s) at %s\n", ErrorString.c_str(), Title);
		g_critical ("%s", message);
		//main_get_gapp () -> warning (message);
		g_free (message);

		if (0){
			fprintf(stdout, "OpenGL Error(%s): %s\n", ErrorString.c_str(), Title);
			assert(0);
		}
	}
	return Error == GL_NO_ERROR;
}

inline bool Surf3d::checkFramebuffer(GLuint FramebufferName)
{
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch(Status)
	{
	case GL_FRAMEBUFFER_UNDEFINED:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_UNDEFINED");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_UNSUPPORTED");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
		break;
	}

	return Status != GL_FRAMEBUFFER_COMPLETE;
}


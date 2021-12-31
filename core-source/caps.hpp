#pragma once

#include "ogl_framework.hpp"
#include <string>

struct caps
{
	enum profile
	{
		CORE = 0x00000001,
		COMPATIBILITY = 0x00000002,
		ES = 0x00000004
	};

private:
	bool check(GLint MajorVersionRequire, GLint MinorVersionRequire);

	struct version
	{
		version(profile const & Profile) :
			PROFILE(Profile),
			MINOR_VERSION(0),
			MAJOR_VERSION(0),
			CONTEXT_FLAGS(0),
			NUM_EXTENSIONS(0)
		{}
		profile PROFILE;
		GLint MINOR_VERSION;
		GLint MAJOR_VERSION;
		GLint CONTEXT_FLAGS;
		GLint NUM_EXTENSIONS;
		std::string RENDERER;
		std::string VENDOR;
		std::string CAPS_VERSION;
		std::string SHADING_LANGUAGE_VERSION;
		GLint NUM_SHADING_LANGUAGE_VERSIONS;
		bool GLSL100;
		bool GLSL110;
		bool GLSL120;
		bool GLSL130;
		bool GLSL140;
		bool GLSL150Core;
		bool GLSL150Comp;
		bool GLSL300ES;
		bool GLSL330Core;
		bool GLSL330Comp;
		bool GLSL400Core;
		bool GLSL400Comp;
		bool GLSL410Core;
		bool GLSL410Comp;
		bool GLSL420Core;
		bool GLSL420Comp;
		bool GLSL430Core;
		bool GLSL430Comp;
		bool GLSL440Core;
		bool GLSL440Comp;
	} VersionData;

	void initVersion();

	struct extensions
	{
		bool ARB_multitexture;
		bool ARB_transpose_matrix;
		bool ARB_multisample;
		bool ARB_texture_env_add;
		bool ARB_texture_cube_map;
		bool ARB_texture_compression;
		bool ARB_texture_border_clamp;
		bool ARB_point_parameters;
		bool ARB_vertex_blend;
		bool ARB_matrix_palette;
		bool ARB_texture_env_combine;
		bool ARB_texture_env_crossbar;
		bool ARB_texture_env_dot3;
		bool ARB_texture_mirrored_repeat;
		bool ARB_depth_texture;
		bool ARB_shadow;
		bool ARB_shadow_ambient;
		bool ARB_window_pos;
		bool ARB_vertex_program;
		bool ARB_fragment_program;
		bool ARB_vertex_buffer_object;
		bool ARB_occlusion_query;
		bool ARB_shader_objects;
		bool ARB_vertex_shader;
		bool ARB_fragment_shader;
		bool ARB_shading_language_100;
		bool ARB_texture_non_power_of_two;
		bool ARB_point_sprite;
		bool ARB_fragment_program_shadow;
		bool ARB_draw_buffers;
		bool ARB_texture_rectangle;
		bool ARB_color_buffer_float;
		bool ARB_half_float_pixel;
		bool ARB_texture_float;
		bool ARB_pixel_buffer_object;
		bool ARB_depth_buffer_float;
		bool ARB_draw_instanced;
		bool ARB_framebuffer_object;
		bool ARB_framebuffer_sRGB;
		bool ARB_geometry_shader4;
		bool ARB_half_float_vertex;
		bool ARB_instanced_arrays;
		bool ARB_map_buffer_range;
		bool ARB_texture_buffer_object;
		bool ARB_texture_compression_rgtc;
		bool ARB_texture_rg;
		bool ARB_vertex_array_object;
		bool ARB_uniform_buffer_object;
		bool ARB_compatibility;
		bool ARB_copy_buffer;
		bool ARB_shader_texture_lod;
		bool ARB_depth_clamp;
		bool ARB_draw_elements_base_vertex;
		bool ARB_fragment_coord_conventions;
		bool ARB_provoking_vertex;
		bool ARB_seamless_cube_map;
		bool ARB_sync;
		bool ARB_texture_multisample;
		bool ARB_vertex_array_bgra;
		bool ARB_draw_buffers_blend;
		bool ARB_sample_shading;
		bool ARB_texture_cube_map_array;
		bool ARB_texture_gather;
		bool ARB_texture_query_lod;
		bool ARB_shading_language_include;
		bool ARB_texture_compression_bptc;
		bool ARB_blend_func_extended;
		bool ARB_explicit_attrib_location;
		bool ARB_occlusion_query2;
		bool ARB_sampler_objects;
		bool ARB_shader_bit_encoding;
		bool ARB_texture_rgb10_a2ui;
		bool ARB_texture_swizzle;
		bool ARB_timer_query;
		bool ARB_vertex_type_2_10_10_10_rev;
		bool ARB_draw_indirect;
		bool ARB_gpu_shader5;
		bool ARB_gpu_shader_fp64;
		bool ARB_shader_subroutine;
		bool ARB_tessellation_shader;
		bool ARB_texture_buffer_object_rgb32;
		bool ARB_transform_feedback2;
		bool ARB_transform_feedback3;
		bool ARB_ES2_compatibility;
		bool ARB_get_program_binary;
		bool ARB_separate_shader_objects;
		bool ARB_shader_precision;
		bool ARB_vertex_attrib_64bit;
		bool ARB_viewport_array;
		bool ARB_cl_event;
		bool ARB_debug_output;
		bool ARB_robustness;
		bool ARB_shader_stencil_export;
		bool ARB_base_instance;
		bool ARB_shading_language_420pack;
		bool ARB_transform_feedback_instanced;
		bool ARB_compressed_texture_pixel_storage;
		bool ARB_conservative_depth;
		bool ARB_internalformat_query;
		bool ARB_map_buffer_alignment;
		bool ARB_shader_atomic_counters;
		bool ARB_shader_image_load_store;
		bool ARB_shading_language_packing;
		bool ARB_texture_storage;
		bool KHR_texture_compression_astc_hdr;
		bool KHR_texture_compression_astc_ldr;
		bool KHR_debug;
		bool ARB_arrays_of_arrays;
		bool ARB_clear_buffer_object;
		bool ARB_compute_shader;
		bool ARB_copy_image;
		bool ARB_texture_view;
		bool ARB_vertex_attrib_binding;
		bool ARB_robustness_isolation;
		bool ARB_ES3_compatibility;
		bool ARB_explicit_uniform_location;
		bool ARB_fragment_layer_viewport;
		bool ARB_framebuffer_no_attachments;
		bool ARB_internalformat_query2;
		bool ARB_invalidate_subdata;
		bool ARB_multi_draw_indirect;
		bool ARB_program_interface_query;
		bool ARB_robust_buffer_access_behavior;
		bool ARB_shader_image_size;
		bool ARB_shader_storage_buffer_object;
		bool ARB_stencil_texturing;
		bool ARB_texture_buffer_range;
		bool ARB_texture_query_levels;
		bool ARB_texture_storage_multisample;
		bool ARB_buffer_storage;
		bool ARB_clear_texture;
		bool ARB_enhanced_layouts;
		bool ARB_multi_bind;
		bool ARB_query_buffer_object;
		bool ARB_texture_mirror_clamp_to_edge;
		bool ARB_texture_stencil8;
		bool ARB_vertex_type_10f_11f_11f_rev;
		bool ARB_bindless_texture;
		bool ARB_compute_variable_group_size;
		bool ARB_indirect_parameters;
		bool ARB_seamless_cubemap_per_texture;
		bool ARB_shader_draw_parameters;
		bool ARB_shader_group_vote;
		bool ARB_sparse_texture;
		bool ARB_ES3_1_compatibility;
		bool ARB_clip_control;
		bool ARB_conditional_render_inverted;
		bool ARB_cull_distance;
		bool ARB_derivative_control;
		bool ARB_direct_state_access;
		bool ARB_get_texture_sub_image;
		bool ARB_shader_texture_image_samples;
		bool ARB_texture_barrier;
		bool KHR_context_flush_control;
		bool KHR_robust_buffer_access_behavior;
		bool KHR_robustness;
		bool ARB_pipeline_statistics_query;
		bool ARB_sparse_buffer;
		bool ARB_transform_feedback_overflow_query;

		bool EXT_texture_compression_latc;
		bool EXT_transform_feedback;
		bool EXT_direct_state_access;
		bool EXT_texture_filter_anisotropic;
		bool EXT_texture_compression_s3tc;
		bool EXT_texture_array;
		bool EXT_texture_snorm;
		bool EXT_texture_sRGB_decode;
		bool EXT_framebuffer_multisample_blit_scaled;
		bool EXT_shader_integer_mix;
		bool EXT_shader_image_load_formatted;
		bool EXT_polygon_offset_clamp;

		bool NV_explicit_multisample;
		bool NV_shader_buffer_load;
		bool NV_vertex_buffer_unified_memory;
		bool NV_shader_buffer_store;
		bool NV_bindless_multi_draw_indirect;
		bool NV_blend_equation_advanced;
		bool NV_deep_texture3D;
		bool NV_shader_thread_group;
		bool NV_shader_thread_shuffle;
		bool NV_shader_atomic_int64;
		bool NV_bindless_multi_draw_indirect_count;
		bool NV_uniform_buffer_unified_memory;

		bool ATI_texture_compression_3dc;
		bool AMD_depth_clamp_separate;
		bool AMD_stencil_operation_extended;
		bool AMD_vertex_shader_viewport_index;
		bool AMD_vertex_shader_layer;
		bool AMD_shader_trinary_minmax;
		bool AMD_interleaved_elements;
		bool AMD_shader_atomic_counter_ops;
		bool AMD_occlusion_query_event;
		bool AMD_shader_stencil_value_export;
		bool AMD_transform_feedback4;
		bool AMD_gpu_shader_int64;
		bool AMD_gcn_shader;

		bool INTEL_map_texture;
		bool INTEL_fragment_shader_ordering;
		bool INTEL_performance_query;
	} ExtensionData;

	void initExtensions();

	struct debug
	{
		GLint CONTEXT_FLAGS;
		GLint MAX_DEBUG_GROUP_STACK_DEPTH;
		GLint MAX_LABEL_LENGTH;
		GLint MAX_SERVER_WAIT_TIMEOUT;
	} DebugData;

	void initDebug();

	struct limits
	{
		GLint MAX_COMPUTE_SHADER_STORAGE_BLOCKS;
		GLint MAX_COMPUTE_UNIFORM_BLOCKS;
		GLint MAX_COMPUTE_TEXTURE_IMAGE_UNITS;
		GLint MAX_COMPUTE_IMAGE_UNIFORMS;
		GLint MAX_COMPUTE_UNIFORM_COMPONENTS;
		GLint MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS;
		GLint MAX_COMPUTE_ATOMIC_COUNTERS;
		GLint MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS;
		GLint MAX_COMPUTE_SHARED_MEMORY_SIZE;
		GLint MAX_COMPUTE_WORK_GROUP_INVOCATIONS;
		GLint MAX_COMPUTE_WORK_GROUP_COUNT;
		GLint MAX_COMPUTE_WORK_GROUP_SIZE;

		GLint MAX_VERTEX_ATOMIC_COUNTERS;
		GLint MAX_VERTEX_SHADER_STORAGE_BLOCKS;
		GLint MAX_VERTEX_ATTRIBS;
		GLint MAX_VERTEX_OUTPUT_COMPONENTS;
		GLint MAX_VERTEX_TEXTURE_IMAGE_UNITS;
		GLint MAX_VERTEX_UNIFORM_COMPONENTS;
		GLint MAX_VERTEX_UNIFORM_VECTORS;
		GLint MAX_VERTEX_UNIFORM_BLOCKS;
		GLint MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS;

		GLint MAX_TESS_CONTROL_ATOMIC_COUNTERS;
		GLint MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS;
		GLint MAX_TESS_CONTROL_INPUT_COMPONENTS;
		GLint MAX_TESS_CONTROL_OUTPUT_COMPONENTS;
		GLint MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS;
		GLint MAX_TESS_CONTROL_UNIFORM_BLOCKS;
		GLint MAX_TESS_CONTROL_UNIFORM_COMPONENTS;
		GLint MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS;

		GLint MAX_TESS_EVALUATION_ATOMIC_COUNTERS;
		GLint MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS;
		GLint MAX_TESS_EVALUATION_INPUT_COMPONENTS;
		GLint MAX_TESS_EVALUATION_OUTPUT_COMPONENTS;
		GLint MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS;
		GLint MAX_TESS_EVALUATION_UNIFORM_BLOCKS;
		GLint MAX_TESS_EVALUATION_UNIFORM_COMPONENTS;
		GLint MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS;

		GLint MAX_GEOMETRY_ATOMIC_COUNTERS;
		GLint MAX_GEOMETRY_SHADER_STORAGE_BLOCKS;
		GLint MAX_GEOMETRY_INPUT_COMPONENTS;
		GLint MAX_GEOMETRY_OUTPUT_COMPONENTS;
		GLint MAX_GEOMETRY_TEXTURE_IMAGE_UNITS;
		GLint MAX_GEOMETRY_UNIFORM_BLOCKS;
		GLint MAX_GEOMETRY_UNIFORM_COMPONENTS;
		GLint MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS;
		GLint MAX_VERTEX_STREAMS;

		GLint MAX_FRAGMENT_ATOMIC_COUNTERS;
		GLint MAX_FRAGMENT_SHADER_STORAGE_BLOCKS;
		GLint MAX_FRAGMENT_INPUT_COMPONENTS;
		GLint MAX_FRAGMENT_UNIFORM_COMPONENTS;
		GLint MAX_FRAGMENT_UNIFORM_VECTORS;
		GLint MAX_FRAGMENT_UNIFORM_BLOCKS;
		GLint MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS;
		GLint MAX_DRAW_BUFFERS;
		GLint MAX_DUAL_SOURCE_DRAW_BUFFERS;

		GLint MAX_COLOR_ATTACHMENTS;
		GLint MAX_FRAMEBUFFER_WIDTH;
		GLint MAX_FRAMEBUFFER_HEIGHT;
		GLint MAX_FRAMEBUFFER_LAYERS;
		GLint MAX_FRAMEBUFFER_SAMPLES;
		GLint MAX_SAMPLE_MASK_WORDS;

		GLint MAX_TRANSFORM_FEEDBACK_BUFFERS;
		GLint MIN_MAP_BUFFER_ALIGNMENT;

		GLint MAX_TEXTURE_IMAGE_UNITS;
		GLint MAX_COMBINED_TEXTURE_IMAGE_UNITS;
		GLint MAX_RECTANGLE_TEXTURE_SIZE;
		GLint MAX_DEEP_3D_TEXTURE_WIDTH_HEIGHT_NV;
		GLint MAX_DEEP_3D_TEXTURE_DEPTH_NV;
		GLint MAX_COLOR_TEXTURE_SAMPLES;
		GLint MAX_DEPTH_TEXTURE_SAMPLES;
		GLint MAX_INTEGER_SAMPLES;
		GLint MAX_TEXTURE_BUFFER_SIZE;
		GLint NUM_COMPRESSED_TEXTURE_FORMATS;
		GLint MAX_TEXTURE_MAX_ANISOTROPY_EXT;

		GLint MAX_PATCH_VERTICES;
		GLint MAX_TESS_GEN_LEVEL;
		GLint MAX_SUBROUTINES;
		GLint MAX_SUBROUTINE_UNIFORM_LOCATIONS;
		GLint MAX_COMBINED_ATOMIC_COUNTERS;
		GLint MAX_COMBINED_SHADER_STORAGE_BLOCKS;
		GLint MAX_PROGRAM_TEXEL_OFFSET;
		GLint MIN_PROGRAM_TEXEL_OFFSET;
		GLint MAX_COMBINED_UNIFORM_BLOCKS;
		GLint MAX_UNIFORM_BUFFER_BINDINGS;
		GLint MAX_UNIFORM_BLOCK_SIZE;
		GLint MAX_UNIFORM_LOCATIONS;
		GLint MAX_VARYING_COMPONENTS;
		GLint MAX_VARYING_VECTORS;
		GLint MAX_VARYING_FLOATS;
		GLint MAX_SHADER_STORAGE_BUFFER_BINDINGS;
		GLint MAX_SHADER_STORAGE_BLOCK_SIZE;
		GLint MAX_COMBINED_SHADER_OUTPUT_RESOURCES;
		GLint SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT;
		GLint UNIFORM_BUFFER_OFFSET_ALIGNMENT;
		GLint NUM_PROGRAM_BINARY_FORMATS;
		GLint PROGRAM_BINARY_FORMATS;
		GLint NUM_SHADER_BINARY_FORMATS;
		GLint SHADER_BINARY_FORMATS;
	} LimitsData;

	void initLimits();

	struct values
	{
		GLint SUBPIXEL_BITS;
		GLint MAX_CLIP_DISTANCES;
		GLint MAX_CULL_DISTANCES;
		GLint MAX_COMBINED_CLIP_AND_CULL_DISTANCES;
		GLint64 MAX_ELEMENT_INDEX;
		GLint MAX_ELEMENTS_INDICES;
		GLint MAX_ELEMENTS_VERTICES;
		GLenum IMPLEMENTATION_COLOR_READ_FORMAT;
		GLenum IMPLEMENTATION_COLOR_READ_TYPE;
		GLboolean PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED;

		GLint MAX_3D_TEXTURE_SIZE;
		GLint MAX_TEXTURE_SIZE;
		GLint MAX_ARRAY_TEXTURE_LAYERS;
		GLint MAX_CUBE_MAP_TEXTURE_SIZE;
		GLint MAX_TEXTURE_LOD_BIAS;
		GLint MAX_RENDERBUFFER_SIZE;

		GLfloat MAX_VIEWPORT_DIMS;
		GLint MAX_VIEWPORTS;
		GLint VIEWPORT_SUBPIXEL_BITS;
		glm::vec2 VIEWPORT_BOUNDS_RANGE;

		GLenum LAYER_PROVOKING_VERTEX;
		GLenum VIEWPORT_INDEX_PROVOKING_VERTEX;

		GLfloat POINT_SIZE_MAX;
		GLfloat POINT_SIZE_MIN;
		glm::vec2 POINT_SIZE_RANGE;
		GLfloat POINT_SIZE_GRANULARITY;

		glm::vec2 ALIASED_LINE_WIDTH_RANGE;
		glm::vec2 SMOOTH_LINE_WIDTH_RANGE;
		GLfloat SMOOTH_LINE_WIDTH_GRANULARITY;

		GLint MAX_VERTEX_ATTRIB_RELATIVE_OFFSET;
		GLint MAX_VERTEX_ATTRIB_BINDINGS;

		GLint TEXTURE_BUFFER_OFFSET_ALIGNMENT;
	} ValuesData;

	void initValues();

	struct formats
	{
		bool COMPRESSED_RGB_S3TC_DXT1_EXT;
		bool COMPRESSED_RGBA_S3TC_DXT1_EXT;
		bool COMPRESSED_RGBA_S3TC_DXT3_EXT;
		bool COMPRESSED_RGBA_S3TC_DXT5_EXT;
		bool COMPRESSED_SRGB_S3TC_DXT1_EXT;
		bool COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		bool COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		bool COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;

		bool COMPRESSED_RED_RGTC1;
		bool COMPRESSED_SIGNED_RED_RGTC1;
		bool COMPRESSED_RG_RGTC2;
		bool COMPRESSED_SIGNED_RG_RGTC2;
		bool COMPRESSED_RGBA_BPTC_UNORM;
		bool COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
		bool COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
		bool COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
		bool COMPRESSED_R11_EAC;
		bool COMPRESSED_SIGNED_R11_EAC;
		bool COMPRESSED_RG11_EAC;
		bool COMPRESSED_SIGNED_RG11_EAC;
		bool COMPRESSED_RGB8_ETC2;
		bool COMPRESSED_SRGB8_ETC2;
		bool COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
		bool COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;
		bool COMPRESSED_RGBA8_ETC2_EAC;
		bool COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;

		bool COMPRESSED_RGBA_ASTC_4x4_KHR;
		bool COMPRESSED_RGBA_ASTC_5x4_KHR;
		bool COMPRESSED_RGBA_ASTC_5x5_KHR;
		bool COMPRESSED_RGBA_ASTC_6x5_KHR;
		bool COMPRESSED_RGBA_ASTC_6x6_KHR;
		bool COMPRESSED_RGBA_ASTC_8x5_KHR;
		bool COMPRESSED_RGBA_ASTC_8x6_KHR;
		bool COMPRESSED_RGBA_ASTC_8x8_KHR;
		bool COMPRESSED_RGBA_ASTC_10x5_KHR;
		bool COMPRESSED_RGBA_ASTC_10x6_KHR;
		bool COMPRESSED_RGBA_ASTC_10x8_KHR;
		bool COMPRESSED_RGBA_ASTC_10x10_KHR;
		bool COMPRESSED_RGBA_ASTC_12x10_KHR;
		bool COMPRESSED_RGBA_ASTC_12x12_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR;
		bool COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR;

		bool COMPRESSED_LUMINANCE_LATC1_EXT;
		bool COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT;
		bool COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT;
		bool COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT;
		bool COMPRESSED_LUMINANCE_ALPHA_3DC_ATI;

		bool COMPRESSED_RGB_FXT1_3DFX;
		bool COMPRESSED_RGBA_FXT1_3DFX;

		bool PALETTE4_RGB8_OES;
		bool PALETTE4_RGBA8_OES;
		bool PALETTE4_R5_G6_B5_OES;
		bool PALETTE4_RGBA4_OES;
		bool PALETTE4_RGB5_A1_OES;
		bool PALETTE8_RGB8_OES;
		bool PALETTE8_RGBA8_OES;
		bool PALETTE8_R5_G6_B5_OES;
		bool PALETTE8_RGBA4_OES;
		bool PALETTE8_RGB5_A1_OES;
		bool ETC1_RGB8_OES;
	} FormatsData;

#	ifndef GL_ETC1_RGB8_OES
#		define GL_ETC1_RGB8_OES	0x8D64
#	endif

	void initFormats();

public:
	caps(profile const & Profile);

	version const & Version;
	extensions const & Extensions;
	debug const & Debug;
	limits const & Limits;
	values const & Values;
	formats const & Formats;
};


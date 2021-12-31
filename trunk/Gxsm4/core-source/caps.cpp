///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Samples Pack (ogl-samples.g-truc.net)
///
/// Copyright (c) 2004 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////

#include "caps.hpp"

bool caps::check(GLint MajorVersionRequire, GLint MinorVersionRequire)
{
	return (VersionData.MAJOR_VERSION * 100 + VersionData.MINOR_VERSION * 10)
		>= (MajorVersionRequire * 100 + MinorVersionRequire * 10);
}

void caps::initVersion()
{
	glGetIntegerv(GL_MINOR_VERSION, &VersionData.MINOR_VERSION);
	glGetIntegerv(GL_MAJOR_VERSION, &VersionData.MAJOR_VERSION);

	this->VersionData.RENDERER = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
	this->VersionData.VENDOR = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
	this->VersionData.CAPS_VERSION = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	this->VersionData.SHADING_LANGUAGE_VERSION = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

	if(this->check(3, 0))
		glGetIntegerv(GL_NUM_EXTENSIONS, &VersionData.NUM_EXTENSIONS);

	if(this->check(4, 3))
	{
		glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &VersionData.NUM_SHADING_LANGUAGE_VERSIONS);
		for(GLint i = 0; i < VersionData.NUM_SHADING_LANGUAGE_VERSIONS; ++i)
		{
			std::string Version((char const*)glGetStringi(GL_SHADING_LANGUAGE_VERSION, i));

			if(Version == std::string("100"))
				VersionData.GLSL100 = true;
			else if(Version == std::string("110"))
				VersionData.GLSL110 = true;
			else if(Version == std::string("120"))
				VersionData.GLSL120 = true;
			else if(Version == std::string("130"))
				VersionData.GLSL130 = true;
			else if(Version == std::string("140"))
				VersionData.GLSL140 = true;
			else if(Version == std::string("150 core"))
				VersionData.GLSL150Core = true;
			else if(Version == std::string("150 compatibility"))
				VersionData.GLSL150Comp = true;
			else if(Version == std::string("300 es"))
				VersionData.GLSL300ES  = true;
			else if(Version == std::string("330 core"))
				VersionData.GLSL330Core = true;
			else if(Version == std::string("330 compatibility"))
				VersionData.GLSL330Comp = true;
			else if(Version == std::string("400 core"))
				VersionData.GLSL400Core = true;
			else if(Version == std::string("400 compatibility"))
				VersionData.GLSL400Comp = true;
			else if(Version == std::string("410 core"))
				VersionData.GLSL410Core = true;
			else if(Version == std::string("410 compatibility"))
				VersionData.GLSL410Comp = true;
			else if(Version == std::string("420 core"))
				VersionData.GLSL420Core = true;
			else if(Version == std::string("420 compatibility"))
				VersionData.GLSL420Comp = true;
			else if(Version == std::string("430 core"))
				VersionData.GLSL430Core = true;
			else if(Version == std::string("440 compatibility"))
				VersionData.GLSL440Comp = true;
			else if(Version == std::string("440 core"))
				VersionData.GLSL440Core = true;
		}
	}
}

void caps::initExtensions()
{
	memset(&ExtensionData, 0, sizeof(ExtensionData));

	glGetIntegerv(GL_NUM_EXTENSIONS, &VersionData.NUM_EXTENSIONS);

	if((this->VersionData.PROFILE == CORE) || (this->VersionData.PROFILE == COMPATIBILITY))
	{
		for (GLint i = 0; i < VersionData.NUM_EXTENSIONS; ++i)
		{
			const char* Extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
			if(!strcmp("GL_ARB_multitexture", Extension)) {
				ExtensionData.ARB_multitexture = true;
				continue;
			}
			if(!strcmp("GL_ARB_transpose_matrix", Extension)) {
				ExtensionData.ARB_transpose_matrix = true;
				continue;
			}
			if(!strcmp("GL_ARB_multisample", Extension)) {
				ExtensionData.ARB_multisample = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_env_add", Extension)) {
				ExtensionData.ARB_texture_env_add = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_cube_map", Extension)) {
				ExtensionData.ARB_texture_cube_map = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_compression", Extension)) {
				ExtensionData.ARB_texture_compression = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_border_clamp", Extension)) {
				ExtensionData.ARB_texture_border_clamp = true;
				continue;
			}
			if(!strcmp("GL_ARB_point_parameters", Extension)) {
				ExtensionData.ARB_point_parameters = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_blend", Extension)) {
				ExtensionData.ARB_vertex_blend = true;
				continue;
			}
			if(!strcmp("GL_ARB_matrix_palette", Extension)) {
				ExtensionData.ARB_matrix_palette = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_env_combine", Extension)) {
				ExtensionData.ARB_texture_env_combine = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_env_crossbar", Extension)) {
				ExtensionData.ARB_texture_env_crossbar = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_env_dot3", Extension)) {
				ExtensionData.ARB_texture_env_dot3 = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_mirrored_repeat", Extension)) { 
				ExtensionData.ARB_texture_mirrored_repeat = true;
				continue;
			}
			if(!strcmp("GL_ARB_depth_texture", Extension)) {
				ExtensionData.ARB_depth_texture = true;
				continue;
			}
			if(!strcmp("GL_ARB_shadow", Extension)) {
				ExtensionData.ARB_shadow = true;
				continue;
			}
			if(!strcmp("GL_ARB_shadow_ambient", Extension)) {
				ExtensionData.ARB_shadow_ambient = true;
				continue;
			}
			if(!strcmp("GL_ARB_window_pos", Extension)) {
				ExtensionData.ARB_window_pos = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_program", Extension)) {
				ExtensionData.ARB_vertex_program = true;
				continue;
			}
			if(!strcmp("GL_ARB_fragment_program", Extension)) {
				ExtensionData.ARB_fragment_program = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_buffer_object", Extension)) {
				ExtensionData.ARB_vertex_buffer_object = true;
				continue;
			}
			if(!strcmp("GL_ARB_occlusion_query", Extension)) {
				ExtensionData.ARB_occlusion_query = true;
				continue;
			}
			if(!strcmp("GL_ARB_shader_objects", Extension)) {
				ExtensionData.ARB_shader_objects = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_shader", Extension)) {
				ExtensionData.ARB_vertex_shader = true;
				continue;
			}
			if(!strcmp("GL_ARB_fragment_shader", Extension)) {
				ExtensionData.ARB_fragment_shader = true;
				continue;
			}
			if(!strcmp("GL_ARB_shading_language_100", Extension)) {
				ExtensionData.ARB_shading_language_100 = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_non_power_of_two", Extension)) {
				ExtensionData.ARB_texture_non_power_of_two = true;
				continue;
			}
			if(!strcmp("GL_ARB_point_sprite", Extension)) {
				ExtensionData.ARB_point_sprite = true;
				continue;
			}
			if(!strcmp("GL_ARB_fragment_program_shadow", Extension)) {
				ExtensionData.ARB_fragment_program_shadow = true;
				continue;
			}
			if(!strcmp("GL_ARB_draw_buffers", Extension)) {
				ExtensionData.ARB_draw_buffers = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_rectangle", Extension)) {
				ExtensionData.ARB_texture_rectangle = true;
				continue;
			}
			if(!strcmp("GL_ARB_color_buffer_float", Extension)) {
				ExtensionData.ARB_color_buffer_float = true;
				continue;
			}
			if(!strcmp("GL_ARB_half_float_pixel", Extension)) {
				ExtensionData.ARB_half_float_pixel = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_float", Extension)) {
				ExtensionData.ARB_texture_float = true;
				continue;
			}
			if(!strcmp("GL_ARB_pixel_buffer_object", Extension)) {
				ExtensionData.ARB_pixel_buffer_object = true;
				continue;
			}
			if(!strcmp("GL_ARB_depth_buffer_float", Extension)) {
				ExtensionData.ARB_depth_buffer_float = true;
				continue;
			}
			if(!strcmp("GL_ARB_draw_instanced", Extension)) {
				ExtensionData.ARB_draw_instanced = true;
				continue;
			}
			if(!strcmp("GL_ARB_framebuffer_object", Extension)) {
				ExtensionData.ARB_framebuffer_object = true;
				continue;
			}
			if(!strcmp("GL_ARB_framebuffer_sRGB", Extension)) {
				ExtensionData.ARB_framebuffer_sRGB = true;
				continue;
			}
			if(!strcmp("GL_ARB_geometry_shader4", Extension)) {
				ExtensionData.ARB_geometry_shader4 = true;
				continue;
			}
			if(!strcmp("GL_ARB_half_float_vertex", Extension)) {
				ExtensionData.ARB_half_float_vertex = true;
				continue;
			}
			if(!strcmp("GL_ARB_instanced_arrays", Extension)) {
				ExtensionData.ARB_instanced_arrays = true;
				continue;
			}
			if(!strcmp("GL_ARB_map_buffer_range", Extension)) {
				ExtensionData.ARB_map_buffer_range = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_buffer_object", Extension)) {
				ExtensionData.ARB_texture_buffer_object = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_compression_rgtc", Extension)) {
				ExtensionData.ARB_texture_compression_rgtc = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_rg", Extension)) {
				ExtensionData.ARB_texture_rg = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_array_object", Extension)) {
				ExtensionData.ARB_vertex_array_object = true;
				continue;
			}
			if(!strcmp("GL_ARB_uniform_buffer_object", Extension)) {
				ExtensionData.ARB_uniform_buffer_object = true;
				continue;
			}
			if(!strcmp("GL_ARB_compatibility", Extension)) {
				ExtensionData.ARB_compatibility = true;
				continue;
			}
			if(!strcmp("GL_ARB_copy_buffer", Extension)) {
				ExtensionData.ARB_copy_buffer = true;
				continue;
			}
			if(!strcmp("GL_ARB_shader_texture_lod", Extension)) {
				ExtensionData.ARB_shader_texture_lod = true;
				continue;
			}
			if(!strcmp("GL_ARB_depth_clamp", Extension)) {
				ExtensionData.ARB_depth_clamp = true;
				continue;
			}
			if(!strcmp("GL_ARB_draw_elements_base_vertex", Extension)) {
				ExtensionData.ARB_draw_elements_base_vertex = true;
				continue;
			}
			if(!strcmp("GL_ARB_fragment_coord_conventions", Extension)) {
				ExtensionData.ARB_fragment_coord_conventions = true;
				continue;
			}
			if(!strcmp("GL_ARB_provoking_vertex", Extension)) {
				ExtensionData.ARB_provoking_vertex = true;
				continue;
			}
			if(!strcmp("GL_ARB_seamless_cube_map", Extension)) {
				ExtensionData.ARB_seamless_cube_map = true;
				continue;
			}
			if(!strcmp("GL_ARB_sync", Extension)) {
				ExtensionData.ARB_sync = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_multisample", Extension)) {
				ExtensionData.ARB_texture_multisample = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_array_bgra", Extension)) {
				ExtensionData.ARB_vertex_array_bgra = true;
				continue;
			}
			if(!strcmp("GL_ARB_draw_buffers_blend", Extension)) {
				ExtensionData.ARB_draw_buffers_blend = true;
				continue;
			}
			if(!strcmp("GL_ARB_sample_shading", Extension)) {
				ExtensionData.ARB_sample_shading = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_cube_map_array", Extension)) {
				ExtensionData.ARB_texture_cube_map_array = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_gather", Extension)) {
				ExtensionData.ARB_texture_gather = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_query_lod", Extension)) {
				ExtensionData.ARB_texture_query_lod = true;
				continue;
			}
			if(!strcmp("GL_ARB_shading_language_include", Extension)) {
				ExtensionData.ARB_shading_language_include = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_compression_bptc", Extension)) {
				ExtensionData.ARB_texture_compression_bptc = true;
				continue;
			}
			if(!strcmp("GL_ARB_blend_func_extended", Extension)) {
				ExtensionData.ARB_blend_func_extended = true;
				continue;
			}
			if(!strcmp("GL_ARB_explicit_attrib_location", Extension)) {
				ExtensionData.ARB_explicit_attrib_location = true;
				continue;
			}
			if(!strcmp("GL_ARB_occlusion_query2", Extension)) {
				ExtensionData.ARB_occlusion_query2 = true;
				continue;
			}
			if(!strcmp("GL_ARB_sampler_objects", Extension)) {
				ExtensionData.ARB_sampler_objects = true;
				continue;
			}
			if(!strcmp("GL_ARB_shader_bit_encoding", Extension)) {
				ExtensionData.ARB_shader_bit_encoding = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_rgb10_a2ui", Extension)) {
				ExtensionData.ARB_texture_rgb10_a2ui = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_swizzle", Extension)) {
				ExtensionData.ARB_texture_swizzle = true;
				continue;
			}
			if(!strcmp("GL_ARB_timer_query", Extension)) {
				ExtensionData.ARB_timer_query = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_type_2_10_10_10_rev", Extension)) {
				ExtensionData.ARB_vertex_type_2_10_10_10_rev = true;
				continue;
			}
			if(!strcmp("GL_ARB_draw_indirect", Extension)) {
				ExtensionData.ARB_draw_indirect = true;
				continue;
			}
			if(!strcmp("GL_ARB_gpu_shader5", Extension)) {
				ExtensionData.ARB_gpu_shader5 = true;
				continue;
			}
			if(!strcmp("GL_ARB_gpu_shader_fp64", Extension)) {
				ExtensionData.ARB_gpu_shader_fp64 = true;
				continue;
			}
			if(!strcmp("GL_ARB_shader_subroutine", Extension)) {
				ExtensionData.ARB_shader_subroutine = true;
				continue;
			}
			if(!strcmp("GL_ARB_tessellation_shader", Extension)) {
				ExtensionData.ARB_tessellation_shader = true;
				continue;
			}
			if(!strcmp("GL_ARB_texture_buffer_object_rgb32", Extension)) {
				ExtensionData.ARB_texture_buffer_object_rgb32 = true;
				continue;
			}
			if(!strcmp("GL_ARB_transform_feedback2", Extension)) {
				ExtensionData.ARB_transform_feedback2 = true;
				continue;
			}
			if(!strcmp("GL_ARB_transform_feedback3", Extension)) {
				ExtensionData.ARB_transform_feedback3 = true;
				continue;
			}
			if(!strcmp("GL_ARB_ES2_compatibility", Extension)) {
				ExtensionData.ARB_ES2_compatibility = true;
				continue;
			}
			if(!strcmp("GL_ARB_get_program_binary", Extension)) {
				ExtensionData.ARB_get_program_binary = true;
				continue;
			}
			if(!strcmp("GL_ARB_separate_shader_objects", Extension)) {
				ExtensionData.ARB_separate_shader_objects = true;
				continue;
			}
			if(!strcmp("GL_ARB_shader_precision", Extension)) {
				ExtensionData.ARB_shader_precision = true;
				continue;
			}
			if(!strcmp("GL_ARB_vertex_attrib_64bit", Extension)) {
				ExtensionData.ARB_vertex_attrib_64bit = true;
				continue;
			}
			if(!strcmp("GL_ARB_viewport_array", Extension)) {
				ExtensionData.ARB_viewport_array = true;
				continue;
			}
			if(!strcmp("GL_ARB_cl_event", Extension)) {
				ExtensionData.ARB_cl_event = true;
				continue;
			}
			if(!strcmp("GL_ARB_debug_output", Extension)) {
				ExtensionData.ARB_debug_output = true;
				continue;
			}
			if(!strcmp("GL_ARB_robustness", Extension)) {
				ExtensionData.ARB_robustness = true;
				continue;
			}
			if(!strcmp("GL_ARB_shader_stencil_export", Extension)) {
				ExtensionData.ARB_shader_stencil_export = true;
				continue;
			}
			if(!strcmp("GL_ARB_base_instance", Extension)) {
				ExtensionData.ARB_base_instance = true;
				continue;
			}
			else if(!strcmp("GL_ARB_shading_language_420pack", Extension)) {
				ExtensionData.ARB_shading_language_420pack = true;
				continue;
			}
			else if(!strcmp("GL_ARB_transform_feedback_instanced", Extension)) {
				ExtensionData.ARB_transform_feedback_instanced = true;
				continue;
			}
			else if(!strcmp("GL_ARB_compressed_texture_pixel_storage", Extension)) {
				ExtensionData.ARB_compressed_texture_pixel_storage = true;
				continue;
			}
			else if(!strcmp("GL_ARB_conservative_depth", Extension)) {
				ExtensionData.ARB_conservative_depth = true;
				continue;
			}
			else if(!strcmp("GL_ARB_internalformat_query", Extension)) {
				ExtensionData.ARB_internalformat_query = true;
				continue;
			}
			else if(!strcmp("GL_ARB_map_buffer_alignment", Extension)) {
				ExtensionData.ARB_map_buffer_alignment = true;
				continue;
			}
			else if(!strcmp("GL_ARB_shader_atomic_counters", Extension)) {
				ExtensionData.ARB_shader_atomic_counters = true;
				continue;
			}
			else if(!strcmp("GL_ARB_shader_image_load_store", Extension)) {
				ExtensionData.ARB_shader_image_load_store = true;
				continue;
			}
			else if(!strcmp("GL_ARB_shading_language_packing", Extension)) {
				ExtensionData.ARB_shading_language_packing = true;
				continue;
			}
			else if(!strcmp("GL_ARB_texture_storage", Extension)) {
				ExtensionData.ARB_texture_storage = true;
				continue;
			}
			else if(!strcmp("GL_KHR_texture_compression_astc_hdr", Extension)) {
				ExtensionData.KHR_texture_compression_astc_hdr = true;
				continue;
			}
			else if(!strcmp("GL_KHR_texture_compression_astc_ldr", Extension)) {
				ExtensionData.KHR_texture_compression_astc_ldr = true;
				continue;
			}
			else if(!strcmp("GL_KHR_debug", Extension)) {
				ExtensionData.KHR_debug = true;
				continue;
			}
			else if(!strcmp("GL_ARB_arrays_of_arrays", Extension)) {
				ExtensionData.ARB_arrays_of_arrays = true;
				continue;
			}
			else if(!strcmp("GL_ARB_clear_buffer_object", Extension))
				ExtensionData.ARB_clear_buffer_object = true;
			else if(!strcmp("GL_ARB_compute_shader", Extension))
				ExtensionData.ARB_compute_shader = true;
			else if(!strcmp("GL_ARB_copy_image", Extension))
				ExtensionData.ARB_copy_image = true;
			else if(!strcmp("GL_ARB_texture_view", Extension))
				ExtensionData.ARB_texture_view = true;
			else if(!strcmp("GL_ARB_vertex_attrib_binding", Extension))
				ExtensionData.ARB_vertex_attrib_binding = true;
			else if(!strcmp("GL_ARB_robustness_isolation", Extension))
				ExtensionData.ARB_robustness_isolation = true;
			else if(!strcmp("GL_ARB_ES3_compatibility", Extension))
				ExtensionData.ARB_ES3_compatibility = true;
			else if(!strcmp("GL_ARB_explicit_uniform_location", Extension))
				ExtensionData.ARB_explicit_uniform_location = true;
			else if(!strcmp("GL_ARB_fragment_layer_viewport", Extension))
				ExtensionData.ARB_fragment_layer_viewport = true;
			else if(!strcmp("GL_ARB_framebuffer_no_attachments", Extension))
				ExtensionData.ARB_framebuffer_no_attachments = true;
			else if(!strcmp("GL_ARB_internalformat_query2", Extension))
				ExtensionData.ARB_internalformat_query2 = true;
			else if(!strcmp("GL_ARB_invalidate_subdata", Extension))
				ExtensionData.ARB_invalidate_subdata = true;
			else if(!strcmp("GL_ARB_multi_draw_indirect", Extension))
				ExtensionData.ARB_multi_draw_indirect = true;
			else if(!strcmp("GL_ARB_program_interface_query", Extension))
				ExtensionData.ARB_program_interface_query = true;
			else if(!strcmp("GL_ARB_robust_buffer_access_behavior", Extension))
				ExtensionData.ARB_robust_buffer_access_behavior = true;
			else if(!strcmp("GL_ARB_shader_image_size", Extension))
				ExtensionData.ARB_shader_image_size = true;
			else if(!strcmp("GL_ARB_shader_storage_buffer_object", Extension))
				ExtensionData.ARB_shader_storage_buffer_object = true;
			else if(!strcmp("GL_ARB_stencil_texturing", Extension))
				ExtensionData.ARB_stencil_texturing = true;
			else if(!strcmp("GL_ARB_texture_buffer_range", Extension))
				ExtensionData.ARB_texture_buffer_range = true;
			else if(!strcmp("GL_ARB_texture_query_levels", Extension))
				ExtensionData.ARB_texture_query_levels = true;
			else if(!strcmp("GL_ARB_texture_storage_multisample", Extension))
				ExtensionData.ARB_texture_storage_multisample = true;
			else if(!strcmp("GL_ARB_buffer_storage", Extension))
				ExtensionData.ARB_buffer_storage = true;
			else if(!strcmp("GL_ARB_clear_texture", Extension))
				ExtensionData.ARB_clear_texture = true;
			else if(!strcmp("GL_ARB_enhanced_layouts", Extension))
				ExtensionData.ARB_enhanced_layouts = true;
			else if(!strcmp("GL_ARB_multi_bind", Extension))
				ExtensionData.ARB_multi_bind = true;
			else if(!strcmp("GL_ARB_query_buffer_object", Extension))
				ExtensionData.ARB_query_buffer_object = true;
			else if(!strcmp("GL_ARB_texture_mirror_clamp_to_edge", Extension))
				ExtensionData.ARB_texture_mirror_clamp_to_edge = true;
			else if(!strcmp("GL_ARB_texture_stencil8", Extension))
				ExtensionData.ARB_texture_stencil8 = true;
			else if(!strcmp("GL_ARB_vertex_type_10f_11f_11f_rev", Extension))
				ExtensionData.ARB_vertex_type_10f_11f_11f_rev = true;
			else if(!strcmp("GL_ARB_bindless_texture", Extension))
				ExtensionData.ARB_bindless_texture = true;
			else if(!strcmp("GL_ARB_compute_variable_group_size", Extension))
				ExtensionData.ARB_compute_variable_group_size = true;
			else if(!strcmp("GL_ARB_indirect_parameters", Extension))
				ExtensionData.ARB_indirect_parameters = true;
			else if(!strcmp("GL_ARB_seamless_cubemap_per_texture", Extension))
				ExtensionData.ARB_seamless_cubemap_per_texture = true;
			else if(!strcmp("GL_ARB_shader_draw_parameters", Extension))
				ExtensionData.ARB_shader_draw_parameters = true;
			else if(!strcmp("GL_ARB_shader_group_vote", Extension))
				ExtensionData.ARB_shader_group_vote = true;
			else if(!strcmp("GL_ARB_sparse_texture", Extension))
				ExtensionData.ARB_sparse_texture = true;
			else if(!strcmp("GL_ARB_ES3_1_compatibility", Extension))
				ExtensionData.ARB_ES3_1_compatibility = true;
			else if(!strcmp("GL_ARB_clip_control", Extension))
				ExtensionData.ARB_clip_control = true;
			else if(!strcmp("GL_ARB_conditional_render_inverted", Extension))
				ExtensionData.ARB_conditional_render_inverted = true;
			else if(!strcmp("GL_ARB_derivative_control", Extension))
				ExtensionData.ARB_derivative_control = true;
			else if(!strcmp("GL_ARB_direct_state_access", Extension))
				ExtensionData.ARB_direct_state_access = true;
			else if(!strcmp("GL_ARB_get_texture_sub_image", Extension))
				ExtensionData.ARB_get_texture_sub_image = true;
			else if(!strcmp("GL_ARB_shader_texture_image_samples", Extension))
				ExtensionData.ARB_shader_texture_image_samples = true;
			else if(!strcmp("GL_ARB_texture_barrier", Extension))
				ExtensionData.ARB_texture_barrier = true;
			else if(!strcmp("GL_KHR_context_flush_control", Extension))
				ExtensionData.KHR_context_flush_control = true;
			else if(!strcmp("GL_KHR_robust_buffer_access_behavior", Extension))
				ExtensionData.KHR_robust_buffer_access_behavior = true;
			else if(!strcmp("GL_KHR_robustness", Extension))
				ExtensionData.KHR_robustness = true;
			else if(!strcmp("GL_ARB_pipeline_statistics_query", Extension))
				ExtensionData.ARB_pipeline_statistics_query = true;
			else if(!strcmp("GL_ARB_sparse_buffer", Extension))
				ExtensionData.ARB_sparse_buffer = true;
			else if(!strcmp("GL_ARB_transform_feedback_overflow_query", Extension))
				ExtensionData.ARB_transform_feedback_overflow_query = true;

			// EXT
			if(!strcmp("GL_EXT_texture_compression_s3tc", Extension)) {
				ExtensionData.EXT_texture_compression_s3tc = true;
				continue;
			}
			if(!strcmp("GL_EXT_texture_compression_latc", Extension)) {
				ExtensionData.EXT_texture_compression_latc = true;
				continue;
			}
			if(!strcmp("GL_EXT_transform_feedback", Extension)) {
				ExtensionData.EXT_transform_feedback = true;
				continue;
			}
			if(!strcmp("GL_EXT_direct_state_access", Extension)) {
				ExtensionData.EXT_direct_state_access = true;
				continue;
			}
			if(!strcmp("GL_EXT_texture_filter_anisotropic", Extension)) {
				ExtensionData.EXT_texture_filter_anisotropic = true;
				continue;
			}
			if(!strcmp("GL_EXT_texture_array", Extension)) {
				ExtensionData.EXT_texture_array = true;
				continue;
			}
			if(!strcmp("GL_EXT_texture_snorm", Extension)) {
				ExtensionData.EXT_texture_snorm = true;
				continue;
			}
			if(!strcmp("GL_EXT_texture_sRGB_decode", Extension)) {
				ExtensionData.EXT_texture_sRGB_decode = true;
				continue;
			}
			if(!strcmp("GL_EXT_framebuffer_multisample_blit_scaled", Extension)) {
				ExtensionData.EXT_framebuffer_multisample_blit_scaled = true;
				continue;
			}
			if(!strcmp("GL_EXT_shader_integer_mix", Extension)) {
				ExtensionData.EXT_shader_integer_mix = true;
				continue;
			}
			if(!strcmp("GL_EXT_polygon_offset_clamp", Extension)) {
				ExtensionData.EXT_polygon_offset_clamp = true;
				continue;
			}

			// NV
			if(!strcmp("GL_NV_explicit_multisample", Extension)) {
				ExtensionData.NV_explicit_multisample = true;
				continue;
			}
			if(!strcmp("GL_NV_shader_buffer_load", Extension)) {
				ExtensionData.NV_shader_buffer_load = true;
				continue;
			}
			if(!strcmp("GL_NV_vertex_buffer_unified_memory", Extension)) {
				ExtensionData.NV_vertex_buffer_unified_memory = true;
				continue;
			}
			if(!strcmp("GL_NV_shader_buffer_store", Extension)) {
				ExtensionData.NV_shader_buffer_store = true;
				continue;
			}
			if(!strcmp("GL_NV_bindless_multi_draw_indirect", Extension)) {
				ExtensionData.NV_bindless_multi_draw_indirect = true;
				continue;
			}
			if(!strcmp("GL_NV_blend_equation_advanced", Extension)) {
				ExtensionData.NV_blend_equation_advanced = true;
				continue;
			}
			if(!strcmp("GL_NV_deep_texture3D", Extension)) {
				ExtensionData.NV_deep_texture3D = true;
				continue;
			}
			if(!strcmp("GL_NV_shader_thread_group", Extension)) {
				ExtensionData.NV_shader_thread_group = true;
				continue;
			}
			if(!strcmp("GL_NV_shader_thread_shuffle", Extension)) {
				ExtensionData.NV_shader_thread_shuffle = true;
				continue;
			}
			if(!strcmp("GL_NV_shader_atomic_int64", Extension)) {
				ExtensionData.NV_shader_atomic_int64 = true;
				continue;
			}
			if(!strcmp("GL_NV_bindless_multi_draw_indirect_count", Extension)) {
				ExtensionData.NV_bindless_multi_draw_indirect_count = true;
				continue;
			}
			if(!strcmp("GL_NV_uniform_buffer_unified_memory", Extension)) {
				ExtensionData.NV_uniform_buffer_unified_memory = true;
				continue;
			}

			// AMD

			if(!strcmp("GL_ATI_texture_compression_3dc", Extension)) {
				ExtensionData.ATI_texture_compression_3dc = true;
				continue;
			}
			if(!strcmp("GL_AMD_depth_clamp_separate", Extension)) {
				ExtensionData.AMD_depth_clamp_separate = true;
				continue;
			}
			if(!strcmp("GL_AMD_stencil_operation_extended", Extension)) {
				ExtensionData.AMD_stencil_operation_extended = true;
				continue;
			}
			if(!strcmp("GL_AMD_vertex_shader_viewport_index", Extension)) {
				ExtensionData.AMD_vertex_shader_viewport_index = true;
				continue;
			}
			if(!strcmp("GL_AMD_vertex_shader_layer", Extension)) {
				ExtensionData.AMD_vertex_shader_layer = true;
				continue;
			}
			if(!strcmp("GL_AMD_shader_trinary_minmax", Extension)) {
				ExtensionData.AMD_shader_trinary_minmax = true;
				continue;
			}
			if(!strcmp("GL_AMD_interleaved_elements", Extension)) {
				ExtensionData.AMD_interleaved_elements = true;
				continue;
			}
			if(!strcmp("GL_AMD_shader_atomic_counter_ops", Extension)) {
				ExtensionData.AMD_shader_atomic_counter_ops = true;
				continue;
			}
			if(!strcmp("GL_AMD_shader_stencil_value_export", Extension)) {
				ExtensionData.AMD_shader_stencil_value_export = true;
				continue;
			}
			if(!strcmp("GL_AMD_transform_feedback4", Extension)) {
				ExtensionData.AMD_transform_feedback4 = true;
				continue;
			}
			if(!strcmp("GL_AMD_gpu_shader_int64", Extension)) {
				ExtensionData.AMD_gpu_shader_int64 = true;
				continue;
			}
			if(!strcmp("GL_AMD_gcn_shader", Extension)) {
				ExtensionData.AMD_gcn_shader = true;
				continue;
			}

			// Intel
			if(!strcmp("GL_INTEL_map_texture", Extension)) {
				ExtensionData.INTEL_map_texture = true;
				continue;
			}
			if(!strcmp("GL_INTEL_fragment_shader_ordering", Extension)) {
				ExtensionData.INTEL_fragment_shader_ordering = true;
				continue;
			}
			if(!strcmp("GL_INTEL_performance_query", Extension)) {
				ExtensionData.INTEL_performance_query = true;
				continue;
			}
		}
	}
}

void caps::initDebug()
{
	memset(&DebugData, 0, sizeof(DebugData));

	glGetIntegerv(GL_CONTEXT_FLAGS, &DebugData.CONTEXT_FLAGS);
	glGetIntegerv(GL_MAX_DEBUG_GROUP_STACK_DEPTH, &DebugData.MAX_DEBUG_GROUP_STACK_DEPTH);
	glGetIntegerv(GL_MAX_LABEL_LENGTH, &DebugData.MAX_LABEL_LENGTH);
	glGetIntegerv(GL_MAX_SERVER_WAIT_TIMEOUT, &DebugData.MAX_SERVER_WAIT_TIMEOUT);
}

void caps::initLimits()
{
	memset(&LimitsData, 0, sizeof(LimitsData));

	if(check(4, 3) || ExtensionData.ARB_compute_shader)
	{
		glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &LimitsData.MAX_COMPUTE_TEXTURE_IMAGE_UNITS);
		glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, &LimitsData.MAX_COMPUTE_UNIFORM_COMPONENTS);
		glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &LimitsData.MAX_COMPUTE_SHARED_MEMORY_SIZE);
		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &LimitsData.MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &LimitsData.MAX_COMPUTE_WORK_GROUP_COUNT);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &LimitsData.MAX_COMPUTE_WORK_GROUP_SIZE);
	}

	if(check(4, 3) || (ExtensionData.ARB_compute_shader && ExtensionData.ARB_uniform_buffer_object))
	{
		glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &LimitsData.MAX_COMPUTE_UNIFORM_BLOCKS);
		glGetIntegerv(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, &LimitsData.MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS);
	}

	if(check(4, 3) || (ExtensionData.ARB_compute_shader && ExtensionData.ARB_shader_image_load_store))
		glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &LimitsData.MAX_COMPUTE_IMAGE_UNIFORMS);

	if(check(4, 3) || (ExtensionData.ARB_compute_shader && ExtensionData.ARB_shader_atomic_counters))
	{
		glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, &LimitsData.MAX_COMPUTE_ATOMIC_COUNTERS);
		glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, &LimitsData.MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS);
	}

	if(check(4, 3) || (ExtensionData.ARB_compute_shader && ExtensionData.ARB_shader_storage_buffer_object))
		glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &LimitsData.MAX_COMPUTE_SHADER_STORAGE_BLOCKS);


	if(check(2, 1) || ExtensionData.ARB_vertex_shader)
	{
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &LimitsData.MAX_VERTEX_ATTRIBS);
		glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &LimitsData.MAX_VERTEX_OUTPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &LimitsData.MAX_VERTEX_TEXTURE_IMAGE_UNITS);
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &LimitsData.MAX_VERTEX_UNIFORM_COMPONENTS);
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &LimitsData.MAX_VERTEX_UNIFORM_VECTORS);
	}
	if(check(3, 2) || (ExtensionData.ARB_vertex_shader && ExtensionData.ARB_uniform_buffer_object))
	{
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &LimitsData.MAX_VERTEX_UNIFORM_BLOCKS);
		glGetIntegerv(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, &LimitsData.MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS);
	}
	if(check(4, 2) || (ExtensionData.ARB_vertex_shader && ExtensionData.ARB_shader_atomic_counters))
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &LimitsData.MAX_VERTEX_ATOMIC_COUNTERS);
	if(check(4, 3) || (ExtensionData.ARB_vertex_shader && ExtensionData.ARB_shader_storage_buffer_object))
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &LimitsData.MAX_VERTEX_SHADER_STORAGE_BLOCKS);

	if(check(4, 0) || ExtensionData.ARB_tessellation_shader)
	{
		glGetIntegerv(GL_MAX_TESS_CONTROL_INPUT_COMPONENTS, &LimitsData.MAX_TESS_CONTROL_INPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS, &LimitsData.MAX_TESS_CONTROL_OUTPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS, &LimitsData.MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS);
		glGetIntegerv(GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS, &LimitsData.MAX_TESS_CONTROL_UNIFORM_COMPONENTS);
	}
	if(check(4, 0) || (ExtensionData.ARB_tessellation_shader && ExtensionData.ARB_uniform_buffer_object))
	{
		glGetIntegerv(GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS, &LimitsData.MAX_TESS_CONTROL_UNIFORM_BLOCKS);
		glGetIntegerv(GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS, &LimitsData.MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS);
	}
	if(check(4, 2) || (ExtensionData.ARB_tessellation_shader && ExtensionData.ARB_shader_atomic_counters))
		glGetIntegerv(GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS, &LimitsData.MAX_TESS_CONTROL_ATOMIC_COUNTERS);
	if(check(4, 3) || (ExtensionData.ARB_tessellation_shader && ExtensionData.ARB_shader_storage_buffer_object))
		glGetIntegerv(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, &LimitsData.MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS);

	if(check(4, 0) || ExtensionData.ARB_tessellation_shader)
	{
		glGetIntegerv(GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS, &LimitsData.MAX_TESS_EVALUATION_INPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, &LimitsData.MAX_TESS_EVALUATION_OUTPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, &LimitsData.MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS);
		glGetIntegerv(GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS, &LimitsData.MAX_TESS_EVALUATION_UNIFORM_COMPONENTS);
	}
	if(check(4, 0) || (ExtensionData.ARB_tessellation_shader && ExtensionData.ARB_uniform_buffer_object))
	{
		glGetIntegerv(GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS, &LimitsData.MAX_TESS_EVALUATION_UNIFORM_BLOCKS);
		glGetIntegerv(GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS, &LimitsData.MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS);
	}
	if(check(4, 2) || (ExtensionData.ARB_tessellation_shader && ExtensionData.ARB_shader_atomic_counters))
		glGetIntegerv(GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS, &LimitsData.MAX_TESS_EVALUATION_ATOMIC_COUNTERS);
	if(check(4, 3) || (ExtensionData.ARB_tessellation_shader && ExtensionData.ARB_shader_storage_buffer_object))
		glGetIntegerv(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, &LimitsData.MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS);

	if(check(3, 2) || ExtensionData.ARB_geometry_shader4)
	{
		glGetIntegerv(GL_MAX_GEOMETRY_INPUT_COMPONENTS, &LimitsData.MAX_GEOMETRY_INPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &LimitsData.MAX_GEOMETRY_OUTPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &LimitsData.MAX_GEOMETRY_TEXTURE_IMAGE_UNITS);
		glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, &LimitsData.MAX_GEOMETRY_UNIFORM_COMPONENTS);
	}
	if(check(3, 2) || (ExtensionData.ARB_geometry_shader4 && ExtensionData.ARB_uniform_buffer_object))
	{
		glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &LimitsData.MAX_GEOMETRY_UNIFORM_BLOCKS);
		glGetIntegerv(GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, &LimitsData.MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS);
	}
	if(check(4, 0) || (ExtensionData.ARB_geometry_shader4 && ExtensionData.ARB_transform_feedback3))
		glGetIntegerv(GL_MAX_VERTEX_STREAMS, &LimitsData.MAX_VERTEX_STREAMS);
	if(check(4, 2) || (ExtensionData.ARB_geometry_shader4 && ExtensionData.ARB_shader_atomic_counters))
		glGetIntegerv(GL_MAX_GEOMETRY_ATOMIC_COUNTERS, &LimitsData.MAX_GEOMETRY_ATOMIC_COUNTERS);
	if(check(4, 3) || (ExtensionData.ARB_geometry_shader4 && ExtensionData.ARB_shader_storage_buffer_object))
		glGetIntegerv(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &LimitsData.MAX_GEOMETRY_SHADER_STORAGE_BLOCKS);

	if(check(2, 1))
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &LimitsData.MAX_DRAW_BUFFERS);

	if(check(2, 1) || ExtensionData.ARB_fragment_shader)
	{
		glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &LimitsData.MAX_FRAGMENT_INPUT_COMPONENTS);
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &LimitsData.MAX_FRAGMENT_UNIFORM_COMPONENTS);
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &LimitsData.MAX_FRAGMENT_UNIFORM_VECTORS);
	}
	if(check(3, 2) || (ExtensionData.ARB_fragment_shader && ExtensionData.ARB_uniform_buffer_object))
	{
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &LimitsData.MAX_FRAGMENT_UNIFORM_BLOCKS);
		glGetIntegerv(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, &LimitsData.MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS);
	}
	if(check(3, 3) || (ExtensionData.ARB_blend_func_extended))
		glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, &LimitsData.MAX_DUAL_SOURCE_DRAW_BUFFERS);
	if(check(4, 2) || (ExtensionData.ARB_fragment_shader && ExtensionData.ARB_shader_atomic_counters))
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &LimitsData.MAX_FRAGMENT_ATOMIC_COUNTERS);
	if(check(4, 3) || (ExtensionData.ARB_fragment_shader && ExtensionData.ARB_shader_storage_buffer_object))
		glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &LimitsData.MAX_FRAGMENT_SHADER_STORAGE_BLOCKS);

	if(check(3, 0) || (ExtensionData.ARB_framebuffer_object))
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &LimitsData.MAX_COLOR_ATTACHMENTS);

	if(check(4, 3) || (ExtensionData.ARB_framebuffer_no_attachments))
	{
		glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &LimitsData.MAX_FRAMEBUFFER_HEIGHT);
		glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &LimitsData.MAX_FRAMEBUFFER_WIDTH);
		glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &LimitsData.MAX_FRAMEBUFFER_LAYERS);
		glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &LimitsData.MAX_FRAMEBUFFER_SAMPLES);
	}

	if(check(4, 0) || (ExtensionData.ARB_transform_feedback3))
		glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &LimitsData.MAX_TRANSFORM_FEEDBACK_BUFFERS);
	if(check(4, 2) || (ExtensionData.ARB_map_buffer_alignment))
		glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &LimitsData.MIN_MAP_BUFFER_ALIGNMENT);

	if(ExtensionData.NV_deep_texture3D)
	{
		glGetIntegerv(GL_MAX_DEEP_3D_TEXTURE_WIDTH_HEIGHT_NV, &LimitsData.MAX_DEEP_3D_TEXTURE_WIDTH_HEIGHT_NV);
		glGetIntegerv(GL_MAX_DEEP_3D_TEXTURE_DEPTH_NV, &LimitsData.MAX_DEEP_3D_TEXTURE_DEPTH_NV);
	}

	if(check(2, 1))
	{
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &LimitsData.MAX_TEXTURE_IMAGE_UNITS);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &LimitsData.MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &LimitsData.MAX_TEXTURE_MAX_ANISOTROPY_EXT);
	}

	if(check(3, 0) || (ExtensionData.ARB_texture_buffer_object))
		glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &LimitsData.MAX_TEXTURE_BUFFER_SIZE);

	if(check(3, 2) || (ExtensionData.ARB_texture_multisample))
	{
		glGetIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &LimitsData.MAX_SAMPLE_MASK_WORDS);
		glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &LimitsData.MAX_COLOR_TEXTURE_SAMPLES);
		glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &LimitsData.MAX_DEPTH_TEXTURE_SAMPLES);
		glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &LimitsData.MAX_INTEGER_SAMPLES);
	}

	if(check(3, 3) || (ExtensionData.ARB_texture_rectangle))
		glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &LimitsData.MAX_RECTANGLE_TEXTURE_SIZE);

	if(check(2, 2) && VersionData.PROFILE == caps::COMPATIBILITY)
	{
		glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &LimitsData.MAX_VARYING_COMPONENTS);
		glGetIntegerv(GL_MAX_VARYING_VECTORS, &LimitsData.MAX_VARYING_VECTORS);
		glGetIntegerv(GL_MAX_VARYING_FLOATS, &LimitsData.MAX_VARYING_FLOATS);
	}

	if(check(3, 2))
	{
		glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &LimitsData.MAX_COMBINED_UNIFORM_BLOCKS);
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &LimitsData.MAX_UNIFORM_BUFFER_BINDINGS);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &LimitsData.MAX_UNIFORM_BLOCK_SIZE);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &LimitsData.UNIFORM_BUFFER_OFFSET_ALIGNMENT);
	}

	if(check(4, 0))
	{
		glGetIntegerv(GL_MAX_PATCH_VERTICES, &LimitsData.MAX_PATCH_VERTICES);
		glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &LimitsData.MAX_TESS_GEN_LEVEL);
		glGetIntegerv(GL_MAX_SUBROUTINES, &LimitsData.MAX_SUBROUTINES);
		glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &LimitsData.MAX_SUBROUTINE_UNIFORM_LOCATIONS);
		glGetIntegerv(GL_MAX_COMBINED_ATOMIC_COUNTERS, &LimitsData.MAX_COMBINED_ATOMIC_COUNTERS);
		glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &LimitsData.MAX_COMBINED_SHADER_STORAGE_BLOCKS);
		glGetIntegerv(GL_MAX_PROGRAM_TEXEL_OFFSET, &LimitsData.MAX_PROGRAM_TEXEL_OFFSET);
		glGetIntegerv(GL_MIN_PROGRAM_TEXEL_OFFSET, &LimitsData.MIN_PROGRAM_TEXEL_OFFSET);
	}

	if(check(4, 1))
	{
		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &LimitsData.NUM_PROGRAM_BINARY_FORMATS);
		glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &LimitsData.NUM_SHADER_BINARY_FORMATS);
		glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, &LimitsData.PROGRAM_BINARY_FORMATS);
	}

	if(check(4, 2))
	{
		glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &LimitsData.MAX_COMBINED_SHADER_OUTPUT_RESOURCES);
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &LimitsData.MAX_SHADER_STORAGE_BUFFER_BINDINGS);
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &LimitsData.MAX_SHADER_STORAGE_BLOCK_SIZE);
		glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &LimitsData.MAX_COMBINED_SHADER_OUTPUT_RESOURCES);
		glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &LimitsData.SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
	}

	if(check(4, 3))
	{
		glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &LimitsData.MAX_COMBINED_SHADER_OUTPUT_RESOURCES);
	}

	if(check(4, 3) || ExtensionData.ARB_explicit_uniform_location)
	{
		glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &LimitsData.MAX_UNIFORM_LOCATIONS);
	}
}

void caps::initValues()
{
	memset(&ValuesData, 0, sizeof(ValuesData));

	if(check(2, 1))
	{
		glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &ValuesData.MAX_ELEMENTS_INDICES);
		glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &ValuesData.MAX_ELEMENTS_VERTICES);
	}

	if(check(4, 3) || (ExtensionData.ARB_vertex_attrib_binding))
	{
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &ValuesData.MAX_VERTEX_ATTRIB_RELATIVE_OFFSET);
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &ValuesData.MAX_VERTEX_ATTRIB_BINDINGS);
	}

	if(check(4, 3) || (ExtensionData.ARB_ES3_compatibility))
		glGetInteger64v(GL_MAX_ELEMENT_INDEX, &ValuesData.MAX_ELEMENT_INDEX);

	if(VersionData.PROFILE == caps::COMPATIBILITY)
	{
		glGetFloatv(GL_POINT_SIZE_MIN, &ValuesData.POINT_SIZE_MIN);
		glGetFloatv(GL_POINT_SIZE_MAX, &ValuesData.POINT_SIZE_MAX);
	}

	glGetFloatv(GL_POINT_SIZE_RANGE, &ValuesData.POINT_SIZE_RANGE[0]);
	glGetFloatv(GL_POINT_SIZE_GRANULARITY, &ValuesData.POINT_SIZE_GRANULARITY);
	glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, &ValuesData.ALIASED_LINE_WIDTH_RANGE[0]);
	glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, &ValuesData.SMOOTH_LINE_WIDTH_RANGE[0]);
	glGetFloatv(GL_SMOOTH_LINE_WIDTH_GRANULARITY, &ValuesData.SMOOTH_LINE_WIDTH_GRANULARITY);

	if(check(2, 1))
	{
		glGetIntegerv(GL_SUBPIXEL_BITS, &ValuesData.SUBPIXEL_BITS);
		glGetFloatv(GL_MAX_VIEWPORT_DIMS, &ValuesData.MAX_VIEWPORT_DIMS);
	}

	if(check(3, 0))
	{
		glGetIntegerv(GL_MAX_CLIP_DISTANCES, &ValuesData.MAX_CLIP_DISTANCES);
	}

	if(check(4, 5) || (ExtensionData.ARB_cull_distance))
	{
		glGetIntegerv(GL_MAX_CULL_DISTANCES, &ValuesData.MAX_CULL_DISTANCES);
		glGetIntegerv(GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES, &ValuesData.MAX_COMBINED_CLIP_AND_CULL_DISTANCES);
	}

	if(check(4, 1) || (ExtensionData.ARB_viewport_array))
	{
		glGetIntegerv(GL_MAX_VIEWPORTS, &ValuesData.MAX_VIEWPORTS);
		glGetIntegerv(GL_VIEWPORT_SUBPIXEL_BITS, &ValuesData.VIEWPORT_SUBPIXEL_BITS);
		glGetFloatv(GL_VIEWPORT_BOUNDS_RANGE, &ValuesData.VIEWPORT_BOUNDS_RANGE[0]);
		glGetIntegerv(GL_LAYER_PROVOKING_VERTEX, reinterpret_cast<GLint*>(&ValuesData.LAYER_PROVOKING_VERTEX));
		glGetIntegerv(GL_VIEWPORT_INDEX_PROVOKING_VERTEX, reinterpret_cast<GLint*>(&ValuesData.VIEWPORT_INDEX_PROVOKING_VERTEX));
	}

	if(check(4, 1) || (ExtensionData.ARB_ES2_compatibility))
	{
		GLint IMPLEMENTATION_COLOR_READ_FORMAT = 0;
		GLint IMPLEMENTATION_COLOR_READ_TYPE = 0;
		glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &IMPLEMENTATION_COLOR_READ_FORMAT);
		glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &IMPLEMENTATION_COLOR_READ_TYPE);
		ValuesData.IMPLEMENTATION_COLOR_READ_FORMAT = IMPLEMENTATION_COLOR_READ_FORMAT;
		ValuesData.IMPLEMENTATION_COLOR_READ_TYPE = IMPLEMENTATION_COLOR_READ_TYPE;
	}

	if(check(2, 1))
	{
		glGetIntegerv(GL_MAX_TEXTURE_LOD_BIAS, &ValuesData.MAX_TEXTURE_LOD_BIAS);
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &ValuesData.MAX_TEXTURE_SIZE);
		glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &ValuesData.MAX_3D_TEXTURE_SIZE);
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &ValuesData.MAX_CUBE_MAP_TEXTURE_SIZE);
	}

	if(check(3, 0) || (ExtensionData.EXT_texture_array))
		glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &ValuesData.MAX_ARRAY_TEXTURE_LAYERS);

	if(check(4, 3) || (ExtensionData.ARB_texture_buffer_object))
		glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &ValuesData.TEXTURE_BUFFER_OFFSET_ALIGNMENT);
}

void caps::initFormats()
{
	memset(&FormatsData, 0, sizeof(FormatsData));

	GLint NUM_COMPRESSED_TEXTURE_FORMATS(0);
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &NUM_COMPRESSED_TEXTURE_FORMATS);

	std::vector<GLint> COMPRESSED_TEXTURE_FORMATS(static_cast<std::size_t>(NUM_COMPRESSED_TEXTURE_FORMATS));
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, &COMPRESSED_TEXTURE_FORMATS[0]);

	for(GLint i = 0; i < NUM_COMPRESSED_TEXTURE_FORMATS; ++i)
	{
		switch(COMPRESSED_TEXTURE_FORMATS[i])
		{
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			FormatsData.COMPRESSED_RGB_S3TC_DXT1_EXT = true;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			FormatsData.COMPRESSED_RGBA_S3TC_DXT1_EXT = true;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			FormatsData.COMPRESSED_RGBA_S3TC_DXT3_EXT = true;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			FormatsData.COMPRESSED_RGBA_S3TC_DXT5_EXT = true;
			break;
		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
			FormatsData.COMPRESSED_SRGB_S3TC_DXT1_EXT = true;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
			FormatsData.COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT = true;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
			FormatsData.COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT = true;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
			FormatsData.COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT = true;
			break;

		case GL_COMPRESSED_RED_RGTC1:
			FormatsData.COMPRESSED_RED_RGTC1 = true;
			break;
		case GL_COMPRESSED_SIGNED_RED_RGTC1:
			FormatsData.COMPRESSED_SIGNED_RED_RGTC1 = true;
			break;
		case GL_COMPRESSED_RG_RGTC2:
			FormatsData.COMPRESSED_RG_RGTC2 = true;
			break;
		case GL_COMPRESSED_SIGNED_RG_RGTC2:
			FormatsData.COMPRESSED_SIGNED_RG_RGTC2 = true;
			break;
		case GL_COMPRESSED_RGBA_BPTC_UNORM:
			FormatsData.COMPRESSED_RGBA_BPTC_UNORM = true;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
			FormatsData.COMPRESSED_SRGB_ALPHA_BPTC_UNORM = true;
			break;
		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
			FormatsData.COMPRESSED_RGB_BPTC_SIGNED_FLOAT = true;
			break;
		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
			FormatsData.COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = true;
			break;
		case GL_COMPRESSED_R11_EAC:
			FormatsData.COMPRESSED_R11_EAC = true;
			break;
		case GL_COMPRESSED_SIGNED_R11_EAC:
			FormatsData.COMPRESSED_SIGNED_R11_EAC = true;
			break;
		case GL_COMPRESSED_RG11_EAC:
			FormatsData.COMPRESSED_RG11_EAC = true;
			break;
		case GL_COMPRESSED_SIGNED_RG11_EAC:
			FormatsData.COMPRESSED_SIGNED_RG11_EAC = true;
			break;
		case GL_COMPRESSED_RGB8_ETC2:
			FormatsData.COMPRESSED_RGB8_ETC2 = true;
			break;
		case GL_COMPRESSED_SRGB8_ETC2:
			FormatsData.COMPRESSED_SRGB8_ETC2 = true;
			break;
		case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			FormatsData.COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = true;
			break;
		case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			FormatsData.COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = true;
			break;
		case GL_COMPRESSED_RGBA8_ETC2_EAC:
			FormatsData.COMPRESSED_RGBA8_ETC2_EAC = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = true;
			break;

		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_4x4_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_5x4_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_5x5_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_6x5_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_6x6_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_8x5_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_8x6_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_8x8_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_10x5_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_10x6_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_10x8_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_10x10_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_12x10_KHR = true;
			break;
		case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
			FormatsData.COMPRESSED_RGBA_ASTC_12x12_KHR = true;
			break;

		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR = true;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
			FormatsData.COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR = true;
			break;

		case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
			FormatsData.COMPRESSED_LUMINANCE_LATC1_EXT = true;
			break;
		case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
			FormatsData.COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT = true;
			break;
		case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
			FormatsData.COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT = true;
			break;
		case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
			FormatsData.COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT = true;
			break;
		case GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI:
			FormatsData.COMPRESSED_LUMINANCE_ALPHA_3DC_ATI = true;
			break;
		case GL_COMPRESSED_RGB_FXT1_3DFX:
			FormatsData.COMPRESSED_RGB_FXT1_3DFX = true;
			break;
		case GL_COMPRESSED_RGBA_FXT1_3DFX:
			FormatsData.COMPRESSED_RGBA_FXT1_3DFX = true;
			break;
#if 0
		case GL_PALETTE4_RGB8_OES:
			FormatsData.PALETTE4_RGB8_OES = true;
			break;
		case GL_PALETTE4_RGBA8_OES:
			FormatsData.PALETTE4_RGBA8_OES = true;
			break;
		case GL_PALETTE4_R5_G6_B5_OES:
			FormatsData.PALETTE4_R5_G6_B5_OES = true;
			break;
		case GL_PALETTE4_RGBA4_OES:
			FormatsData.PALETTE4_RGBA4_OES = true;
			break;
		case GL_PALETTE4_RGB5_A1_OES:
			FormatsData.PALETTE4_RGB5_A1_OES = true;
			break;
		case GL_PALETTE8_RGB8_OES:
			FormatsData.PALETTE8_RGB8_OES = true;
			break;
		case GL_PALETTE8_RGBA8_OES:
			FormatsData.PALETTE8_RGBA8_OES = true;
			break;
		case GL_PALETTE8_R5_G6_B5_OES:
			FormatsData.PALETTE8_R5_G6_B5_OES = true;
			break;
		case GL_PALETTE8_RGBA4_OES:
			FormatsData.PALETTE8_RGBA4_OES = true;
			break;
		case GL_PALETTE8_RGB5_A1_OES:
			FormatsData.PALETTE8_RGB5_A1_OES = true;
			break;
#endif
		case GL_ETC1_RGB8_OES:
			FormatsData.ETC1_RGB8_OES = true;
			break;

		default:
			// Unknown formats
			break;
		}
	}
}

caps::caps(profile const & Profile) :
	VersionData(Profile),
	Version(VersionData),
	Extensions(ExtensionData),
	Debug(DebugData),
	Limits(LimitsData),
	Values(ValuesData),
	Formats(FormatsData)
{
	this->initVersion();
	this->initExtensions();

	if(this->check(4, 3) || Extensions.KHR_debug)
		glGetIntegerv(GL_CONTEXT_FLAGS, &VersionData.CONTEXT_FLAGS);

	this->initDebug();
	this->initLimits();
	this->initValues();
	this->initFormats();
}

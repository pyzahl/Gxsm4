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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

Surf3d_GLview_data GLvd_offset; // dummy

#define GET_GLV_OFFSET(ELEM) (void*)((long)(ELEM)-(long)(&GLvd_offset))

const gchar *TrueFalse_OptionsList[]  = { "true", "false", NULL };
const gchar *OnOff_OptionsList[]      = { "On", "Off", NULL };
const gchar *ViewPreset_OptionsList[] = { "Manual", "Top", "Front", "Left", "Right", "Areal View Front", "Scan: Auto Tip View", NULL };
const gchar *LookAt_OptionsList[]     = { "Manual", "Tip", "Center", NULL };

const gchar *VertexSrc_OptionsList[]  = { "Flat", "Direct Height", "Mode View Height", "Channel-X", "y-data",
					  "X-Slice", "Y-Slice", "Z-Slice",
					  "Volume", "Scatter",
					  NULL };
const gchar *ShadeModel_OptionsList[]  = { "Lambertian, use Palette",
					   "Terrain",
					   "Material Color",
					   "Fog+Material Color",
					   "X Color Lambertian",
					   "Volume",
					   "Debug Shader",
					   NULL };
const gchar *ColorSrc_OptionsList[]   = { "Flat", "Direct Height", "View Mode Height", "X-Channel", "Y", NULL };
const gchar *TickFrame_OptionsList[]  = { "0: Simple", "1: XYZ with Labels", "2: XYZ Box", "3: XYZ w L Box", NULL };

const gchar *CScale_OptionsList[] = { "-3","3","0.001","0.001","4", NULL };
const gchar *ColorContrast_OptionsList[] = { "0","2","0.001","0.001","4", NULL };
const gchar *ColorOffset_OptionsList[]   = { "-1","1","0.001","0.001","4", NULL };
const gchar *LOD_OptionsList[]  = {"0","64","1","1","0",NULL };

const gchar *Rot_OptionsList[]   = {"-180","180","1","1","0",NULL };
const gchar *FoV_OptionsList[]   = {"0","180","1","1","0",NULL };
const gchar *Frustum_OptionsList[] = {"0.0001","30","0.01","0.01","3",NULL };
const gchar *Dist_OptionsList[]  = {"-20","20","0.01","0.01","2",NULL };
const gchar *Hskl_mode_OptionsList[] = { "Absolute Ang", "Relative Range", NULL };
const gchar *Hskl_OptionsList[]  = {"-20","20","0.001","0.001","4",NULL };
const gchar *Tskl_OptionsList[]  = {"-10","10","0.01","0.01","1",NULL };
const gchar *Slice_OptionsList[]  = {"-1","1","0.01","0.01","2",NULL };
const gchar *Shininess_OptionsList[]  = {"0","100","0.1","1","1",NULL };
const gchar *FogD_OptionsList[]  = {"0","100","0.001","0.01","2",NULL };
const gchar *shader_mode_OptionsList[]  = {"0","20","1","1","0",NULL };
const gchar *tess_level_OptionsList[]  = {"1","64","1.0","1.0","0",NULL };
const gchar *base_grid_OptionsList[]  = {"8","512","1.0","1.0","0",NULL };
const gchar *probe_atoms_OptionsList[]  = {"5","1000000","100.0","100.0","1",NULL };
const gchar *Animation_Index_OptionsList[]   = {"0","10000","1","1","0",NULL };

GnomeResEntryInfoType v3dControl_pref_def_const[] = {
	GNOME_RES_ENTRY_FIRST_NAME("GXSM_V3DCONTROL_20030901000"),

// ============ View

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/RotationX", "Rotation X", "0", GET_GLV_OFFSET (&GLvd_offset.rot[0]), 
	  Rot_OptionsList, N_("View"),
	  N_("Rotation Angle in X [LMB + Mouse left/right]"), NULL
		),
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/RotationY", "Rotation Y", "-90", GET_GLV_OFFSET (&GLvd_offset.rot[1]), 
	  Rot_OptionsList, N_("View"),
	  N_("Rotation Angle in Y [LMB + Mouse up/down]"), NULL
		),
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/RotationZ", "Rotation Z", "0", GET_GLV_OFFSET (&GLvd_offset.rot[2]), 
	  Rot_OptionsList, N_("View"),
	  N_("Rotation Angle in Z"), NULL
		),
	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.View/RotationPreset", "Manual", GET_GLV_OFFSET (&GLvd_offset.view_preset[0]),
	  ViewPreset_OptionsList, N_("View"), 
	  N_("Predefined Views.\nElevation and Distance referring to Top View Model Orientation.")
		),
	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.View/LookAt", "Center", GET_GLV_OFFSET (&GLvd_offset.look_at[0]),
	  LookAt_OptionsList, N_("View"), 
	  N_("Predefined Look-At Positions.")
		),
		
	GNOME_RES_ENTRY_SEPARATOR (N_("View"), NULL),

	GNOME_RES_ENTRY_FLOAT_VEC3
	( "V3dControl.View/Translation", "Translation", "0 0 0", GET_GLV_OFFSET (&GLvd_offset.trans), N_("View"),
	  N_("set translations vector X, Y, Z\n"
	     "[MMB + Mouse up/down/left/right]"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/FoV", "FoV", "55.0", GET_GLV_OFFSET (&GLvd_offset.fov), 
	  FoV_OptionsList, N_("View"),
	  N_("Field of View"), NULL
	  ),

#if 0
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/FrustumNear", "FrustumNear", "0.001", GET_GLV_OFFSET (&GLvd_offset.fnear), 
	  Frustum_OptionsList, N_("View"),
	  N_("Frustum Distance Near: Z-Buffer Near Cut Off"), NULL
	  ),
	
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/FrustumFar", "FrustumFar", "10.", GET_GLV_OFFSET (&GLvd_offset.ffar), 
	  Frustum_OptionsList, N_("View"),
	  N_("Frustum Distance Far: Z-Buffer Far Cut Off"), NULL
	  ),

#endif

	
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/Shift", "Shift", "0.0", GET_GLV_OFFSET (&GLvd_offset.camera[0]), 
	  Dist_OptionsList, N_("View"),
	  N_("Shift: Camera GL_X Position (Shift to left/right from surface center -- in top view rotation)"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/Distance", "Distance", "1.5", GET_GLV_OFFSET (&GLvd_offset.camera[1]), 
	  Dist_OptionsList, N_("View"),
	  N_("Distance: Camera GL_Z Position [also use Mouse Wheel] (Distance from surface center in front -- in top view rotation)"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/Elevation", "Elevation", "1.0", GET_GLV_OFFSET (&GLvd_offset.camera[2]), 
	  Dist_OptionsList, N_("View"),
	  N_("Elevation: Camera GL-Y Position [RMB + Mouse up/dn] (surface normal axis, elevation above surface -- in top view rotation)"), NULL
		),
  
	GNOME_RES_ENTRY_SEPARATOR (N_("View"), NULL),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.View/HeighScaleMode", "Absolute Ang", GET_GLV_OFFSET (&GLvd_offset.height_scale_mode[0]),
	  Hskl_mode_OptionsList, N_("View"), 
	  N_("Height Scaling Mode)")
		),
	
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/Hskl", "Height Skl", "1.0", GET_GLV_OFFSET (&GLvd_offset.hskl), 
	  Hskl_OptionsList, N_("View"),
	  N_("Height Scaling (Z scale)"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/Tskl", "Tetra Skl", "1", GET_GLV_OFFSET (&GLvd_offset.tskl), 
	  Tskl_OptionsList, N_("View"),
	  N_("Tetra Scaling (Triangle scale)"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.View/SliceOffset", "Slice Offset", "1", GET_GLV_OFFSET (&GLvd_offset.slice_offset), 
	  Slice_OptionsList, N_("View"),
	  N_("Volume Slice Offset Z"), NULL
		),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.View/VertexSource", "Direct Height", GET_GLV_OFFSET (&GLvd_offset.vertex_source[0]),
	  VertexSrc_OptionsList, N_("View"), 
	  N_("Vertex/Geometry Height Source for Surface Model or Flat Slicing/Volumetric rendering.")
		),

	GNOME_RES_ENTRY_FLOAT_VEC4
	( "V3dControl.View/SliceLimiter", "SliceLimiter", "0 1000 25 0", GET_GLV_OFFSET (&GLvd_offset.slice_start_n[0]), 
	  N_("View"), N_("Slicing Control: 1000 slice plane cover full range [Start, Stop, Step, --]"), NULL
		),

#if 0
	GNOME_RES_ENTRY_FLOAT_VEC4
	( "V3dControl.View/SlicePlanes", "SlicePlanes", "-1 -1 -1 0", GET_GLV_OFFSET (&GLvd_offset.slice_plane_index[0]), 
	  N_("View"), N_("Slice Plane to add, set to -1 for off, else index [X, Y, Z, mode]"), NULL
		),
#endif
// ============ Light

	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/GlobalAmbient", "GlobalAmbient", "1 1 1 1", GET_GLV_OFFSET (&GLvd_offset.light_global_ambient), N_("Light"),
	  N_("Global Ambient Light [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("Light"), NULL),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.Light/Sun", "On", GET_GLV_OFFSET (&GLvd_offset.light[0][0]),
	  OnOff_OptionsList, N_("Light"), 
	  N_("Light0: switch On/Off")
		),
	GNOME_RES_ENTRY_FLOAT_VEC4
	( "V3dControl.Light/Light0Dir", "LightSunDirection", "-0.2 -1.0 0.3 1", GET_GLV_OFFSET (&GLvd_offset.light_position[0]), N_("Light"),
	  N_("Sun Light Direction (Incident Vector Model Space) [dX, dY, dZ, 1]"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/Light0Spec", "LightSunColor", "1 1 1 1", GET_GLV_OFFSET (&GLvd_offset.light_specular[0]), N_("Light"),
	  N_("Light0: Specular Light Color [red, green, blue, alpha]"), NULL
		),
#if 0
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/Light0Diff", "Light0Diff", "0.4 0.4 0.4 1", GET_GLV_OFFSET (&GLvd_offset.light_diffuse[0]), N_("Light"),
	  N_("Light0: Diffuse Light Color [red, green, blue, alpha]"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/Light0Amb", "Light0Amb", "0.1 0.1 0.1 1", GET_GLV_OFFSET (&GLvd_offset.light_ambient[0]), N_("Light"),
	  N_("Light0: Ambient Light Color [red, green, blue, alpha]"), NULL
		),
#endif

	GNOME_RES_ENTRY_SEPARATOR (N_("Light"), NULL),

#if 0
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "V3dControl.Light/Labels", "On", GET_GLV_OFFSET (&GLvd_offset.light[1][0]),
	  OnOff_OptionsList, N_("Light"), 
	  N_("Light1: switch On/Off")
		),

	GNOME_RES_ENTRY_FLOAT_VEC4
	( "V3dControl.Light/Light1Pos", "Light1Pos", "0.5 0 -0.5 0", GET_GLV_OFFSET (&GLvd_offset.light_position[1]), N_("Light"),
	  N_("Light1: Position, relative to surface width (=1) [X, Y, Z, 1]"), NULL
		),

	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/LabelColor", "LabelColor", "1 1 1 1", GET_GLV_OFFSET (&GLvd_offset.light_specular[1]), N_("Light"),
	  N_("Light1: Label Color [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/Light1Diff", "Light1Diff", "0.4 0.4 0.4 1", GET_GLV_OFFSET (&GLvd_offset.light_diffuse[1]), N_("Light"),
	  N_("Light1: Diffuse Light Color [red, green, blue, alpha]"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/Light1Amb", "Light1Amb", "0.1 0.1 0.1 1", GET_GLV_OFFSET (&GLvd_offset.light_ambient[1]), N_("Light"),
	  N_("Light1: Ambient Light Color [red, green, blue, alpha]"), NULL
		),
#endif
	GNOME_RES_ENTRY_SEPARATOR (N_("Light"), NULL),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.Light/Tip", "Off", GET_GLV_OFFSET (&GLvd_offset.tip_display),
	  OnOff_OptionsList, N_("Light"), 
	  N_("Tip display On/Off")
		),
	GNOME_RES_ENTRY_FLOAT_VEC4
	( "V3dControl.Light/TipPos", "TipPos", "0 0 4 3.2", GET_GLV_OFFSET (&GLvd_offset.tip_geometry[0]), N_("Light"),
	  N_("Tip Position and Gap and scale [X-off, Y-off, Z-gap, scale]"), NULL
		),
	GNOME_RES_ENTRY_FLOAT_VEC4
	( "V3dControl.Light/TipASR", "TipASR", "1 1 1 0", GET_GLV_OFFSET (&GLvd_offset.tip_geometry[1]), N_("Light"),
	  N_("Tip Atoms Aspect Ratio vs X [X, Y, Z, 0]"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/TipColorFA", "TipColorFA", "1 0 0 1", GET_GLV_OFFSET (&GLvd_offset.tip_colors[0]), N_("Light"),
	  N_("Tip Color Front Atom"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/TipColorFA1", "TipColorFA2", "1 1 1 1", GET_GLV_OFFSET (&GLvd_offset.tip_colors[1]), N_("Light"),
	  N_("Tip Color Front Atom 2 opt"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/TipColorFA2", "TipColorFA3", "1 1 1 1", GET_GLV_OFFSET (&GLvd_offset.tip_colors[2]), N_("Light"),
	  N_("Tip Color Front Atom 3 opt"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Light/TipColorAP", "TipColorAP", "0.5 0.5 0.5 1", GET_GLV_OFFSET (&GLvd_offset.tip_colors[3]), N_("Light"),
	  N_("Tip Color Apex"), NULL
		),

// ============ Material Surface

	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.MatSurf/Color", "Color", "0.45 0.15 0.07 1", GET_GLV_OFFSET (&GLvd_offset.surf_mat_color), N_("Surface Material"),
	  N_("Surface Material Color:\n Specify the material color for Flat Material Color Shader Mode."), NULL
	  ),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.MatSurf/BackSideColor", "BackSideColor", "0.6 0.1 0.1 1", GET_GLV_OFFSET (&GLvd_offset.surf_mat_backside_color), N_("Surface Material"),
	  N_("Backside Surface Color"), NULL
	  ),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.MatSurf/Diffuse", "Diffuse", "0.6 0.2 0.1 1", GET_GLV_OFFSET (&GLvd_offset.surf_mat_diffuse), N_("Surface Material"),
	  N_("Surface Diffuse Color:\n Specify the diffuse RGBA reflectance of the material.\nUsed only in \"Uniform/Material Color Mode\"."), NULL
	  ),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.MatSurf/Specular", "Specular", "1 1 0.3 1", GET_GLV_OFFSET (&GLvd_offset.surf_mat_specular), N_("Surface Material"),
	  N_("Surface Specular Color:\n Specify the specular RGBA reflectance of the material."), NULL
	  ),
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.MatSurf/Shininess", "Shininess", "50", GET_GLV_OFFSET (&GLvd_offset.surf_mat_shininess[0]), 
	  Shininess_OptionsList, N_("Surface Material"),
	  N_("Surface Shininess"), NULL
	  ),

	GNOME_RES_ENTRY_SEPARATOR (N_("Surface Material"), NULL),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.MatSurf/ColorSrc", "Direct Height", 
	  GET_GLV_OFFSET (&GLvd_offset.ColorSrc[0]),
	  ColorSrc_OptionsList, N_("Surface Material"), 
	  N_("Select Surface Color Source.")
		),
	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.MatSurf/ShadeModel", "Lambertian, use Palette",
	  GET_GLV_OFFSET (&GLvd_offset.ShadeModel[0]),
	  ShadeModel_OptionsList, N_("Surface Material"), 
	  N_("Select Surface Shading Mode.")
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("Surface Material"), NULL),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.MatSurf/Lightness", "Lightness", "0.65", 
	  GET_GLV_OFFSET (&GLvd_offset.Lightness), 
	  CScale_OptionsList, N_("Surface Material"),
	  N_("Color Lightness or Exposure level -- for a typical light mix around 0.6"), NULL
		),
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.MatSurf/ColorOffset", "Color Offset", "0", 
	  GET_GLV_OFFSET (&GLvd_offset.ColorOffset), 
	  CScale_OptionsList, N_("Surface Material"),
	  N_("Color Contrast: shift color source. Default=0"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.MatSurf/ColorSaturation", "Color Saturation", "1", 
	  GET_GLV_OFFSET (&GLvd_offset.ColorSat), 
	  CScale_OptionsList, N_("Surface Material"),
	  N_("Color Saturation, Default=1"), NULL
		),
	GNOME_RES_ENTRY_SEPARATOR (N_("Surface Material"), NULL),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.MatSurf/Transparency", "Transparency", "1", 
	  GET_GLV_OFFSET (&GLvd_offset.transparency), 
	  ColorContrast_OptionsList, N_("Surface Material"),
	  N_("Volume Model Transparency, Default=1"), NULL
		),
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.MatSurf/TransparencyOffset", "Trans. Offset", "0", 
	  GET_GLV_OFFSET (&GLvd_offset.transparency_offset), 
	  CScale_OptionsList, N_("Surface Material"),
	  N_("Volume Model Transparency Offset, Default=0"), NULL
		),
#if 0
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.MatSurf/LOD", "Level of Detail", "0", 
	  GET_GLV_OFFSET (&GLvd_offset.tex3d_lod), 
	  LOD_OptionsList, N_("Surface Material"),
	  N_("Volume Texture Level of Detail (LOD), Default=0"), NULL
		),
#endif

// ============ Annotations

	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Annotations/ZeroPlaneColor", "ZeroPlaneColor", "0.6 0.1 0.1 1", GET_GLV_OFFSET (&GLvd_offset.anno_zero_plane_color), N_("Annotations"),
	  N_("Zero/reference plane color"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Annotations/TitleColor", "TitleColor", "1.0 0.1 0.5 0.7", GET_GLV_OFFSET (&GLvd_offset.anno_title_color), N_("Annotations"),
	  N_("Title Color"), NULL
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.Annotations/LabelColor", "LabelColor", "1.0 0.1 0.5 0.7", GET_GLV_OFFSET (&GLvd_offset.anno_label_color), N_("Annotations"),
	  N_("Label Color"), NULL
		),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "V3dControl.Annotations/Title", "GXSM-3.0 GL4.0",  GET_GLV_OFFSET (&GLvd_offset.anno_title), N_("Annotations"),
	  N_("Title text")
          ),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "V3dControl.Annotations/X-Axis", "X-axis",  GET_GLV_OFFSET (&GLvd_offset.anno_xaxis), N_("Annotations"),
	  N_("Title X-axis")
          ),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "V3dControl.Annotations/Y-Axis", "Y-axis",  GET_GLV_OFFSET (&GLvd_offset.anno_yaxis), N_("Annotations"),
	  N_("Title Y-axis")
          ),
	GNOME_RES_ENTRY_ASK_PATH
	( GNOME_RES_STRING, "V3dControl.Annotations/Z-Axis", "Z-axis",  GET_GLV_OFFSET (&GLvd_offset.anno_zaxis), N_("Annotations"),
	  N_("Title Z-axis")
          ),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "V3dControl.Annotations/ShowTitle", "On", GET_GLV_OFFSET (&GLvd_offset.anno_show_title[0]),
	  OnOff_OptionsList, N_("Annotations"), 
	  N_("Annotations: Title On/Off")
	),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "V3dControl.Annotations/ShowLabels", "On", GET_GLV_OFFSET (&GLvd_offset.anno_show_axis_labels[0]),
	  OnOff_OptionsList, N_("Annotations"), 
	  N_("Annotations: Labels On/Off")
	),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "V3dControl.Annotations/ShowDimensions", "On", GET_GLV_OFFSET (&GLvd_offset.anno_show_axis_dimensions[0]),
	  OnOff_OptionsList, N_("Annotations"), 
	  N_("Annotations: Dimensiones On/Off")
	),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "V3dControl.Annotations/ShowBearings", "On", GET_GLV_OFFSET (&GLvd_offset.anno_show_bearings[0]),
	  OnOff_OptionsList, N_("Annotations"), 
	  N_("Annotations: Bearings On/Off")
	),
	GNOME_RES_ENTRY_AUTO_PATH_OPTION
	( GNOME_RES_STRING, "V3dControl.Annotations/ShowZeroPlanes", "Off", GET_GLV_OFFSET (&GLvd_offset.anno_show_zero_planes[0]),
	  OnOff_OptionsList, N_("Annotations"), 
	  N_("Annotations: Zero/base plane/grid selections On/Off")
	),

// ============ Rendering Options

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Ortho", "false", GET_GLV_OFFSET (&GLvd_offset.Ortho),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable orthographic vs. prespective projection.")
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.RenderOp/TessLevel", "Tesselation Level", "12.0", GET_GLV_OFFSET (&GLvd_offset.tess_level), 
	  tess_level_OptionsList, N_("Render Opt."),
	  N_("Tesseletion Level Max:\n"
	     " 32: normal, 1: no tesselation"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.RenderOp/BasePlaneGrid", "Base Plane Grid", "128.0", GET_GLV_OFFSET (&GLvd_offset.base_plane_size), 
	  base_grid_OptionsList, N_("Render Opt."),
	  N_("Base Plane Grid Size:\n"
	     " 128: default -- max via max tesselation is this number x 64"), NULL
		),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.RenderOp/ProbeAtoms", "Probe Atoms", "1000.0", GET_GLV_OFFSET (&GLvd_offset.probe_atoms), 
	  probe_atoms_OptionsList, N_("Render Opt."),
	  N_("# Probe Atoms"), NULL
		),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Mesh", "false", GET_GLV_OFFSET (&GLvd_offset.Mesh),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable mesh mode (use mesh or solid model)")
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("Render Opt."), NULL),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.RenderOp/ShadingMode", "Fragment Debug Shading Mode Selector", "0.0", GET_GLV_OFFSET (&GLvd_offset.shader_mode), 
	  shader_mode_OptionsList, N_("Render Opt."),
	  N_("Fragment Shading Mode Color Selector for Debug Shader:\n"
	     " 1: return vec4 (lightness*DiffuseColor, 1.)\n"
	     " 2: return vec4 (lightness*SpecularColor, 1.)\n"
	     " 3: return vec4 (lightness*LambertianColor, 1.)\n"
	     " 4: return lightness*color\n"
	     " 5: finalColor = applyFog (finalColor, dist, viewDir); return vec4(lightness*finalColor, 1.)\n"
	     " 6: return vec4 (mix (finalColor.xyz, nois, 0.2), 1.)\n"
	     " 7: return vec4 (nois, 1.)\n"
	     " 8: return lightness*color\n"
	     " 9: return color\n"
	     " 10: return vec4(normal*0.5+0.5, 1.0)\n"
	     " 11: return vec4(normal.y*0.5+0.5, 0.,0., 1.0)\n"
	     " 12: return vec4(normal.x*0.5+0.5, 0.,0., 1.0)\n"
	     " 13: return specular*color\n"
	     " 14: return vec4(vec3(specular,normal.y,diffuse),1.0)\n"
	     " 15: return vec4(vec3(dist/100.), 1.0)\n"
	     " 16: return specular*lightness*vec4(1)\n"
	     " 17: return vec4(vec3(lightness*(gl_FragCoord.z+transparency_offset)), 1.0f)\n"
	     " 18: return vec4(vec3(lightness*(gl_FragCoord.w+transparency_offset)), 1)\n"
	     " 19: return vec4(lightness*gl_FragCoord.w+vec4(transparency_offset))\n"
	     " default: return vec4 (lightness*finalColor, 1.)\n"
	     ), NULL
	  ),

	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.RenderOp/ClearColor", "ClearColor", "0.6 0.7 0.7 1.0", GET_GLV_OFFSET (&GLvd_offset.clear_color), N_("Render Opt."),
	  N_("GL Clear Color, e.g. background  [red, green, blue, alpha]"), NULL
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("Render Opt."), NULL),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Fog", "false", GET_GLV_OFFSET (&GLvd_offset.Fog),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable fog")
		),
	GNOME_RES_ENTRY_COLORSEL
	( "V3dControl.RenderOp/FogColor", "FogColor", "0.6 0.7 0.7 1.0", GET_GLV_OFFSET (&GLvd_offset.fog_color), N_("Render Opt."),
	  N_("Fog Color [red, green, blue, alpha].\n"
	     "Fog blends a fog color with each rasterized pixel fragment's\n"
	     "posttexturing color using a blending factor f. Factor f is computed in\n"
	     "one of three ways (here: f=exp(-density*z)), depending on the fog\n"
	     "mode. Let z be the distance in eye coordinates from the origin to the\n"
	     "fragment being fogged.\n"
	     "Note: you usually want to use the background (clear) color."), NULL
		),
	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.RenderOp/FogDensity", "Fog Density", "0.8", GET_GLV_OFFSET (&GLvd_offset.fog_density), 
	  FogD_OptionsList, N_("Render Opt."),
	  N_("Density of fog, see fog color help!\n"
	     "Effect depends on distance to object and size of object."), NULL
		),

	GNOME_RES_ENTRY_SEPARATOR (N_("Render Opt."), NULL),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Cull", "true", GET_GLV_OFFSET (&GLvd_offset.Cull),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable cull face mode (surface back side as invisible in top view is not drawn!)\n"
	     "Note: if enabled you will not see your surface,\n"
	     "if you are looking from below! Normally enable for speed and reduction of artifacts")
		),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/TransparentSlices", "false", GET_GLV_OFFSET (&GLvd_offset.TransparentSlices),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable transparency for normal non voluem view/slices")
		),
	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Emission", "false", GET_GLV_OFFSET (&GLvd_offset.Emission),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable emission mode")
		),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/ZeroOld", "true", GET_GLV_OFFSET (&GLvd_offset.ZeroOld),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable clear (zero) olddata in life scan update -- only good for top-down scan!)")
		),

	GNOME_RES_ENTRY_STRING
	( "V3dControl.RenderOp/AnimationFile", "/tmp/gxsm-movie-frame%05.0f.png", GET_GLV_OFFSET (&GLvd_offset.animation_file),
	  N_("Render Opt."), 
	  N_("Animation output file, must be like '/tmp/MovieFrame%05.0f.png' to indicate the index or time formatting!\n")
	     ),

	GNOME_RES_ENTRY_FLOATSLIDER
	( "V3dControl.RenderOp/AnimationIndex", "Animation Index", "0", GET_GLV_OFFSET (&GLvd_offset.animation_index), 
	  Animation_Index_OptionsList, N_("Render Opt."),
	  N_("Animation index, set to 0 to disable, if >0 at every increment a file is saved."), NULL
		),

#if 0
	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Tex", "false", GET_GLV_OFFSET (&GLvd_offset.Texture),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable texture (sorry not implemented today)")
		),
	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Smooth", "true", GET_GLV_OFFSET (&GLvd_offset.Smooth),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable Smooth Shading (GL color model is smooth or flat)\n"
	     "Note: smooth shading is much slower but looks also much better.")
		),
	


	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/ZeroPlane", "true", GET_GLV_OFFSET (&GLvd_offset.ZeroPlane),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable Zero Plane/Box")
		),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_BOOL, "V3dControl.RenderOp/Tickmarks", "false", GET_GLV_OFFSET (&GLvd_offset.Ticks),
	  TrueFalse_OptionsList, N_("Render Opt."), 
	  N_("enable/disable 3D Tickmarks drawing\n (Note: not yet available!!)")
		),

	GNOME_RES_ENTRY_OPTION
	( GNOME_RES_STRING, "V3dControl.RenderOp/TickFrameOptions", "2: XYZ Box", GET_GLV_OFFSET (&GLvd_offset.TickFrameOptions[0]),
	  TickFrame_OptionsList, N_("Render Opt."), 
	  N_("Tick Frame Options)")
		),
#endif

	GNOME_RES_ENTRY_LAST
};


libdir = $(plugindir)/math
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4
# -O3 -ftree-loop-linear -ftree-loop-ivcanon -ftree-vectorize -funroll-loops

lib_LTLIBRARIES = libvolume_transform.la \
	libreverse_layers.la libmulti_dim_transpose.la  libmovieconcat.la libautoalign.la libmandriftfix.la \
	libmerge_v.la libmerge_h.la libflip_diagonal.la libmirror_y.la libmirror_x.la libquenchscan.la libscalescan.la \
	libshiftarea.la \
	libaffine.la \
	librotate.la \
	libshear_x.la \
	libshear_y.la \
	libOctoCorr.la \
	librotate90.la \
	libunwrap.la

libunwrap_la_SOURCES = unwrap.C
libunwrap_la_LDFLAGS = -module -export-dynamic -avoid-version

libaffine_la_SOURCES = affine.C
libaffine_la_LDFLAGS = -module -export-dynamic -avoid-version

librotate_la_SOURCES = rotate.C
librotate_la_LDFLAGS = -module -export-dynamic -avoid-version

libshear_x_la_SOURCES = shear_x.C
libshear_x_la_LDFLAGS = -module -export-dynamic -avoid-version

libshear_y_la_SOURCES = shear_y.C
libshear_y_la_LDFLAGS = -module -export-dynamic -avoid-version

libOctoCorr_la_SOURCES = OctoCorr.C
libOctoCorr_la_LDFLAGS = -module -export-dynamic -avoid-version

libshiftarea_la_SOURCES = shiftarea.C
libshiftarea_la_LDFLAGS = -module -export-dynamic -avoid-version

libscalescan_la_SOURCES = scalescan.C
libscalescan_la_LDFLAGS = -module -export-dynamic -avoid-version

libquenchscan_la_SOURCES = quenchscan.C
libquenchscan_la_LDFLAGS = -module -export-dynamic -avoid-version

libmirror_x_la_SOURCES = mirror_x.C
libmirror_x_la_LDFLAGS = -module -export-dynamic -avoid-version

libmirror_y_la_SOURCES = mirror_y.C
libmirror_y_la_LDFLAGS = -module -export-dynamic -avoid-version

libflip_diagonal_la_SOURCES = flip_diagonal.C
libflip_diagonal_la_LDFLAGS = -module -export-dynamic -avoid-version

libmerge_h_la_SOURCES = merge_h.C
libmerge_h_la_LDFLAGS = -module -export-dynamic -avoid-version

libmerge_v_la_SOURCES = merge_v.C
libmerge_v_la_LDFLAGS = -module -export-dynamic -avoid-version

librotate90_la_SOURCES = rotate90.C
librotate90_la_LDFLAGS = -module -export-dynamic -avoid-version

libautoalign_la_SOURCES = autoalign.C autoalign_stack_reg.C autoalign_turbo_reg.C autoalign_turbo_reg.h autoalign_stack_reg.h autoalign_vector_types.h
libautoalign_la_LDFLAGS = -module -export-dynamic -avoid-version

libmovieconcat_la_SOURCES = movieconcat.C
libmovieconcat_la_LDFLAGS = -module -export-dynamic -avoid-version

libmandriftfix_la_SOURCES = mandriftfix.C
libmandriftfix_la_LDFLAGS = -module -export-dynamic -avoid-version

libmulti_dim_transpose_la_SOURCES = multi_dim_transpose.C
libmulti_dim_transpose_la_LDFLAGS = -module -export-dynamic -avoid-version

libreverse_layers_la_SOURCES = reverse_layers.C
libreverse_layers_la_LDFLAGS = -module -export-dynamic -avoid-version

libvolume_transform_la_SOURCES = volume_transform.C
libvolume_transform_la_LDFLAGS = -module -export-dynamic -avoid-version

libdir = $(plugindir)/math
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = libift2d.la libft2d.la libcurvature.la liblocal_height.la\
	libTderive.la libstat_diff.la libsmooth.la libnormal_z.la \
	libedge.la libsmallconvol.la libdespike2d.la liblineinterpol.la

libdespike2d_la_SOURCES = despike2d.C
libdespike2d_la_LDFLAGS = -module -export-dynamic -avoid-version

liblineinterpol_la_SOURCES = lineinterpol.C
liblineinterpol_la_LDFLAGS = -module -export-dynamic -avoid-version

libsmallconvol_la_SOURCES = smallconvol.C
libsmallconvol_la_LDFLAGS = -module -export-dynamic -avoid-version

libsmooth_la_SOURCES = smooth.C
libsmooth_la_LDFLAGS = -module -export-dynamic -avoid-version

libnormal_z_la_SOURCES = normal_z.C
libnormal_z_la_LDFLAGS = -module -export-dynamic -avoid-version

libedge_la_SOURCES = edge.C
libedge_la_LDFLAGS = -module -export-dynamic -avoid-version

libstat_diff_la_SOURCES = stat_diff.C
libstat_diff_la_LDFLAGS = -module -export-dynamic -avoid-version

libTderive_la_SOURCES = Tderive.C
libTderive_la_LDFLAGS = -module -export-dynamic -avoid-version

liblocal_height_la_SOURCES = local_height.C
liblocal_height_la_LDFLAGS = -module -export-dynamic -avoid-version

libcurvature_la_SOURCES = curvature.C
libcurvature_la_LDFLAGS = -module -export-dynamic -avoid-version

libft2d_la_SOURCES = ft2d.C
libft2d_la_LDFLAGS = -module -export-dynamic -avoid-version

libift2d_la_SOURCES = ift2d.C
libift2d_la_LDFLAGS = -module -export-dynamic -avoid-version

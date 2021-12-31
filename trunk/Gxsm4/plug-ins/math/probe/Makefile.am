libdir = $(plugindir)/probe
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = \
	libprobe_image_extract.la \
	libafm_lj_mechanical_sim.la

libprobe_image_extract_la_SOURCES = probe_image_extract.C
libprobe_image_extract_la_LDFLAGS = -module -export-dynamic -avoid-version

libafm_lj_mechanical_sim_la_SOURCES = afm_lj_mechanical_sim.C
libafm_lj_mechanical_sim_la_LDFLAGS = -module -export-dynamic -avoid-version -lnlopt


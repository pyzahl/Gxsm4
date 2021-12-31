libdir = $(plugindir)/math
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = liblinear_stat_diff.la libkoehler.la libdiff.la \
	librepair_cs.la libtimedomfftfilter.la libdespike1d.la \
	libft1d.la

libdespike1d_la_SOURCES = despike1d.C
libdespike1d_la_LDFLAGS = -module -export-dynamic -avoid-version

librepair_cs_la_SOURCES = repair_cs.C
librepair_cs_la_LDFLAGS = -module -export-dynamic -avoid-version

libdiff_la_SOURCES = diff.C
libdiff_la_LDFLAGS = -module -export-dynamic -avoid-version

libkoehler_la_SOURCES = koehler.C
libkoehler_la_LDFLAGS = -module -export-dynamic -avoid-version

liblinear_stat_diff_la_SOURCES = linear_stat_diff.C
liblinear_stat_diff_la_LDFLAGS = -module -export-dynamic -avoid-version

libtimedomfftfilter_la_SOURCES = timedomfftfilter.C
libtimedomfftfilter_la_LDFLAGS = -module -export-dynamic -avoid-version

libft1d_la_SOURCES = ft1d.C
libft1d_la_LDFLAGS = -module -export-dynamic -avoid-version

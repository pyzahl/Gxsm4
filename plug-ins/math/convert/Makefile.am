libdir = $(plugindir)/math
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = \
	libto_byte.la libto_short.la libshort_to_short.la libto_long.la\
	libto_float.la 	libuto_float.la \
	libto_double.la\
	libto_complex.la\
	libmake_test.la

libto_byte_la_SOURCES = to_byte.C
libto_byte_la_LDFLAGS = -module -export-dynamic -avoid-version

libto_short_la_SOURCES = to_short.C
libto_short_la_LDFLAGS = -module -export-dynamic -avoid-version

libshort_to_short_la_SOURCES = short_to_short.C
libshort_to_short_la_LDFLAGS = -module -export-dynamic -avoid-version

libto_long_la_SOURCES = to_long.C
libto_long_la_LDFLAGS = -module -export-dynamic -avoid-version

libto_float_la_SOURCES = to_float.C
libto_float_la_LDFLAGS = -module -export-dynamic -avoid-version

libuto_float_la_SOURCES = uto_float.C
libuto_float_la_LDFLAGS = -module -export-dynamic -avoid-version

libto_double_la_SOURCES = to_double.C
libto_double_la_LDFLAGS = -module -export-dynamic -avoid-version

libto_complex_la_SOURCES = to_complex.C
libto_complex_la_LDFLAGS = -module -export-dynamic -avoid-version

libmake_test_la_SOURCES = make_test.C
libmake_test_la_LDFLAGS = -module -export-dynamic -avoid-version

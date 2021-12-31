libdir = $(plugindir)/math
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = libZ_limiter.la libmax.la libinvert_z.la \
		  libdiv_scan.la libmul_scan.la \
		  libsub_scan.la libadd_scan.la \
		  liblog_z.la libZ_rescale.la libZ_usr_rescale.la \
		  libabs_scan.la

liblog_z_la_SOURCES = log_z.C
liblog_z_la_LDFLAGS = -module -export-dynamic -avoid-version

libZ_rescale_la_SOURCES = Z_rescale.C
libZ_rescale_la_LDFLAGS = -module -export-dynamic -avoid-version

libZ_usr_rescale_la_SOURCES = Z_usr_rescale.C
libZ_usr_rescale_la_LDFLAGS = -module -export-dynamic -avoid-version

libadd_scan_la_SOURCES = add_scan.C
libadd_scan_la_LDFLAGS = -module -export-dynamic -avoid-version

libsub_scan_la_SOURCES = sub_scan.C
libsub_scan_la_LDFLAGS = -module -export-dynamic -avoid-version

libmul_scan_la_SOURCES = mul_scan.C
libmul_scan_la_LDFLAGS = -module -export-dynamic -avoid-version

libdiv_scan_la_SOURCES = div_scan.C
libdiv_scan_la_LDFLAGS = -module -export-dynamic -avoid-version

libinvert_z_la_SOURCES = invert_z.C
libinvert_z_la_LDFLAGS = -module -export-dynamic -avoid-version

libmax_la_SOURCES = max.C
libmax_la_LDFLAGS = -module -export-dynamic -avoid-version

libZ_limiter_la_SOURCES = Z_limiter.C
libZ_limiter_la_LDFLAGS = -module -export-dynamic -avoid-version

libabs_scan_la_SOURCES = abs_scan.C
libabs_scan_la_LDFLAGS = -module -export-dynamic -avoid-version


libdir = $(plugindir)/math
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = libtimescalefft.la libremovelineshifts.la libplane_regression.la libwaterlevel.la \
	libpass_cc.la libstop_ccr.la libplane_max_prop.la libparabolregress.la libbg_z_drift.la \
	libbggamma.la libsubconst.la libmulconst.la \
	libplane3pkt.la liblineregression.la

liblineregression_la_SOURCES = lineregression.C
liblineregression_la_LDFLAGS = -module -export-dynamic -avoid-version

libplane3pkt_la_SOURCES = plane3pkt.C
libplane3pkt_la_LDFLAGS = -module -export-dynamic -avoid-version

libbggamma_la_SOURCES = bggamma.C
libbggamma_la_LDFLAGS = -module -export-dynamic -avoid-version

libsubconst_la_SOURCES = subconst.C
libsubconst_la_LDFLAGS = -module -export-dynamic -avoid-version

libmulconst_la_SOURCES = mulconst.C
libmulconst_la_LDFLAGS = -module -export-dynamic -avoid-version

libbg_z_drift_la_SOURCES = bg_z_drift.C
libbg_z_drift_la_LDFLAGS = -module -export-dynamic -avoid-version

libparabolregress_la_SOURCES = parabolregress.C
libparabolregress_la_LDFLAGS = -module -export-dynamic -avoid-version

libplane_max_prop_la_SOURCES = plane_max_prop.C
libplane_max_prop_la_LDFLAGS = -module -export-dynamic -avoid-version

libstop_ccr_la_SOURCES = stop_ccr.C
libstop_ccr_la_LDFLAGS = -module -export-dynamic -avoid-version

libpass_cc_la_SOURCES = pass_cc.C
libpass_cc_la_LDFLAGS = -module -export-dynamic -avoid-version

libwaterlevel_la_SOURCES = waterlevel.C
libwaterlevel_la_LDFLAGS = -module -export-dynamic -avoid-version

libplane_regression_la_SOURCES = plane_regression.C
libplane_regression_la_LDFLAGS = -module -export-dynamic -avoid-version

libremovelineshifts_la_SOURCES = removelineshifts.C
libremovelineshifts_la_LDFLAGS = -module -export-dynamic -avoid-version

libtimescalefft_la_SOURCES = timescalefft.C
libtimescalefft_la_LDFLAGS = -module -export-dynamic -avoid-version

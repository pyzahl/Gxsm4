libdir = $(plugindir)/control
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = \
	libspm_scancontrol.la \
	libinet_json_external_scandata.la \
	libmultidim_movie_control.la

#	libNanoPlott.la

#	libnano_manipulator.la \
#	librhk_scancontrol.la \
#	libCCDControl.la \
#	libDSPControl.la \
#	libDSPProbe.la
#	libDriftComp.la \
#	libDSPMover.la \
#

libspm_scancontrol_la_LDFLAGS = -module -export-dynamic -avoid-version
libspm_scancontrol_la_SOURCES = spm_scancontrol.C

libinet_json_external_scandata_la_LDFLAGS = -module -export-dynamic -avoid-version -lgsl -lgslcblas -lsoup-2.4
libinet_json_external_scandata_la_SOURCES = inet_json_external_scandata.C jsmn.c jsmn.h resonance_fit.h

libmultidim_movie_control_la_LDFLAGS = -module -export-dynamic -avoid-version
libmultidim_movie_control_la_SOURCES = multidim_movie_control.C

#libNanoPlott_la_LDFLAGS = -module -export-dynamic -avoid-version
#libNanoPlott_la_SOURCES = NanoPlott.C

# broken plugins, not gtkdatabox / not gcc3.3 compat
#	libSpaLeedControl.la
#	libDSPPeakFind.la
# otherwise broken/unstable/testing:
#	libnano_manipulator.la
#       libsranger_hwi_probe_control.la

#libSpaLeedControl_la_LDFLAGS = -module -export-dynamic -avoid-version
#libSpaLeedControl_la_SOURCES = SpaLeedControl.C

#libDSPPeakFind_la_LDFLAGS = -module -export-dynamic -avoid-version -L/usr/lib -lgtkdatabox
#libDSPPeakFind_la_SOURCES = DSPPeakFind.C \
#	app_peakfind.C app_peakfind.h \
#	peakfind_scan.C peakfind_scan.h \
#	app_databox.C app_databox.h \
#	app_focus.C gtkled.c gtkledbar.c

#libsranger_hwi_probe_control_la_LDFLAGS = -module -export-dynamic -avoid-version
#libsranger_hwi_probe_control_la_SOURCES = sranger_hwi_probe_control.C test_structure.h

#libCCDControl_la_LDFLAGS = -module -export-dynamic -avoid-version
#libCCDControl_la_SOURCES = CCDControl.C

#libDSPControl_la_LDFLAGS = -module -export-dynamic -avoid-version
#libDSPControl_la_SOURCES = DSPControl.C

#librhk_scancontrol_la_LDFLAGS = -module -export-dynamic -avoid-version
#librhk_scancontrol_la_SOURCES = rhk_scancontrol.C

#libnano_manipulator_la_LDFLAGS = -module -export-dynamic -avoid-version
#libnano_manipulator_la_SOURCES = nano_manipulator.C

#libDSPMover_la_LDFLAGS = -module -export-dynamic -avoid-version
#libDSPMover_la_SOURCES = DSPMover.C

#libDriftComp_la_LDFLAGS = -module -export-dynamic -avoid-version
#libDriftComp_la_SOURCES = DriftComp.C

#libDSPProbe_la_LDFLAGS = -module -export-dynamic -avoid-version
#libDSPProbe_la_SOURCES = DSPProbe.C app_probe.C app_probe.h probe_scan.C probe_scan.h



# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = \
	org.gnome.gxsm4.pcs.plugin-libspm-scancontrol.gschema.xml \
	org.gnome.gxsm4.pcsadjustments.plugin-libinet-json-external-scandata.gschema.xml \
	org.gnome.gxsm4.pcs.plugin-libinet-json-external-scandata.gschema.xml \
	org.gnome.gxsm4.inet_json_settings.gschema.xml

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@


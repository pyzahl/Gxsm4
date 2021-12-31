libdir = $(plugindir)/common
AM_CPPFLAGS = $(GXSM4_CFLAGS) ${GTKSOURCEVIEW_CFLAGS} -I$(includedir) -I$(top_builddir)/gxsm4 $(shell python3-config --cflags)

lib_LTLIBRARIES = \
	libpyremote.la \
	libPanView.la \
	libProbeIndicator.la \
	libqueryDSPinfo.la \
	libextra_scan_info.la

#	libeditnc.la
#	liblistplugins.la \
#	libmkicons.la \
#	libprinter.la

libpyremote_la_SOURCES = pyremote.C
libpyremote_la_LDFLAGS = -module -export-dynamic -avoid-version ${GTKSOURCEVIEW_LIBS} $(shell python3-config --ldflags)

libPanView_la_SOURCES = PanView.C
libPanView_la_LDFLAGS = -module -export-dynamic -avoid-version

libProbeIndicator_la_SOURCES = ProbeIndicator.C
libProbeIndicator_la_LDFLAGS = -module -export-dynamic -avoid-version

libqueryDSPinfo_la_SOURCES = queryDSPinfo.C
libqueryDSPinfo_la_LDFLAGS = -module -export-dynamic -avoid-version

libextra_scan_info_la_SOURCES = extra_scan_info.C
libextra_scan_info_la_LDFLAGS = -module -export-dynamic -avoid-version

#libeditnc_la_SOURCES = editnc.C
#libeditnc_la_LDFLAGS = -module -export-dynamic -avoid-version

#libmkicons_la_SOURCES = mkicons.C
#libmkicons_la_LDFLAGS = -module -export-dynamic -avoid-version

#libprinter_la_SOURCES = printer.C
#libprinter_la_LDFLAGS = -module -export-dynamic -avoid-version

#liblistplugins_la_SOURCES =  listplugins.C
#liblistplugins_la_LDFLAGS = -module -export-dynamic -avoid-version



# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = \
	org.gnome.gxsm4.pcs.plugin-libpyremote.gschema.xml \
	org.gnome.gxsm4.pcsadjustments.plugin-libpyremote.gschema.xml


# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@

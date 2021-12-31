libdir = $(plugindir)/hard
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4 -I$(top_builddir)/include 

lib_LTLIBRARIES = \
	libsranger_mk2_hwi.la \
	libspm_simulator_hwi.la


libsranger_mk2_hwi_la_SOURCES = sranger_mk2_hwi.C sranger_mk23common_hwi.h \
	sranger_mk2_hwi_dev.C sranger_mk2_hwi_spm.C sranger_mk2_hwi.h \
	sranger_mk3_hwi_dev.C sranger_mk3_hwi_spm.C sranger_mk3_hwi.h \
	sranger_mk2_hwi_control.C sranger_mk2_hwi_vectorgen.C sranger_mk2_hwi_eventhandling.C \
	sranger_mk2_hwi_mover.C sranger_mk3_hwi_pac.C sranger_mk2_hwi_control.h \
	MK2-A810_spmcontrol/FB_spm_dataexchange.h MK3-A810_spmcontrol/FB_spm_dataexchange.h

libsranger_mk2_hwi_la_LDFLAGS = -module -export-dynamic -avoid-version


libspm_simulator_hwi_la_SOURCES = spm_simulator_hwi.C spm_simulator_hwi.h
libspm_simulator_hwi_la_LDFLAGS = -module -export-dynamic -avoid-version


# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = \
	org.gnome.gxsm4.hwi.sranger_mk23.gschema.xml \
	org.gnome.gxsm4.hwi.sranger_mk23_mover.gschema.xml \
	org.gnome.gxsm4.pcs.plugin-libsranger-mk2-hwi.gschema.xml \
	org.gnome.gxsm4.pcsadjustments.plugin-libsranger-mk2-hwi.gschema.xml\
\
	org.gnome.gxsm4.hwi.spm-sim-control.gschema.xml \
	org.gnome.gxsm4.pcs.plugin-libspm-simulator-hwi.gschema.xml \
	org.gnome.gxsm4.pcsadjustments.plugin-libspm-simulator-hwi.gschema.xml

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@

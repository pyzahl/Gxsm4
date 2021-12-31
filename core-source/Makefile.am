# This is the Makefile.am for the gxsm core
# glib-compile-resources gxsm4_resources.xml --target=gxsm4_resources.C --generate-source

bin_PROGRAMS = gxsm4

gxsm4_SOURCES =	\
gxsm_main.C \
gxsm4_resources.C gxsm4_resources.h \
gxsm_window.C gxsm_window.h \
\
plugin_ctrl.C plugin.h plugin_ctrl.h \
\
cairo_item.C cairo_item.h \
gxsm_conf.h \
gapp_service.C gapp_service.h \
gxsm_app.C gxsm_app.h \
xxsmlogo.xpm \
\
gxsm_menu-extension.c gxsm_menu-extension.h \
gnome-res.c gnome-res.h \
gtkmarshalers.c gtkmarshalers.h \
\
gxsm_resoucetable.C gxsm_resoucetable.h \
gxsmdruidlogo.xpm \
gxsmwatermark.xpm \
gxsm_menucb.C \
app_spm.C \
app_channelselector.C \
app_vinfo.C app_vinfo.h \
app_vobj.C app_vobj.h \
app_view.C app_vpopupdef.h app_view.h \
app_monitor.C \
app_profile.C app_profile.h app_profilepopupdef.h \
app_v3dcontrol.C app_v3dcontrol.h app_v3dpopupdef.h \
lineprofile.C lineprofile.h \
vpdata_read.C vpdata_read.h app_vpdata_view.h \
unit.h \
pcs.C pcs.h \
\
meldungen.h \
glbvars.h \
xsmtypes.h \
xsmdebug.h \
xsm_limits.h \
xsm.C xsm.h \
instrument.C instrument.h \
surface.C surface.h \
scan.C scan.h \
topographic.C \
spascan.C \
\
xshmimg.C xshmimg.h \
view.C view.h \
vprofile.C \
\
vsurf3d.C \
ogl_framework.hpp \
vertex.hpp \
uniform_buffer_objects.hpp \
caps.cpp caps.hpp \
error.cpp error.hpp \
compiler.cpp compiler.hpp \
common.hpp \
csv.cpp csv.hpp \
pugiconfig.hpp pugixml.cpp pugixml.hpp \
sementics.hpp \
\
xsmhard.C xsmhard.h \
\
bench.h \
mem2d.C mem2d.h zdata.h \
scan_event.C scan_event.h \
regress.C regress.h \
xsmmath.C xsmmath.h \
util.C \
clip.c clip.h \
\
monitor.C monitor.h \
dataio.C dataio.h \
meldungen.h \
\
writepng.C readpng.C writepng.h readpng.h \
epsfutils.C epsfutils.h

pixmapdir = $(datadir)/pixmaps

DEFS = @DEFS@ \
	$(CFLAGS) \
	-Wall -Wstrict-aliasing=0 -ggdb

#-w -Wstrict-aliasing=0 -ggdb -Wno-fpermissive

AM_CPPFLAGS = -I.. \
	-I$(includedir) \
	${libnetcdf_CFLAGS} \
	-I../include $(GXSM4_CFLAGS) \
	-DGNOME_ICONDIR=\""$(pixmapdir)"\"

gxsm4_LDFLAGS = -export-dynamic $(GXSM4_LIBS)

gxsm4_LDADD =  \
	$(FFTW_LIBS) \
	${libnetcdf_LIBS} \
	-lpng -lGLEW -lGL -lz -lfreetype -licuuc


sysdir = $(datadir)/apps/System


# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = \
	org.gnome.gxsm4.gschema.xml \
	org.gnome.gxsm4.window-geometry.gschema.xml \
	org.gnome.gxsm4.preferences.gschema.xml \
	org.gnome.gxsm4.gl.gschema.xml \
	org.gnome.gxsm4.pcs.mainwindow.gschema.xml \
	org.gnome.gxsm4.pcsadjustments.mainwindow.gschema.xml

# build gxsm2_resource.C
# glib-compile-resources gxsm4_resources.xml --target=gxsm4_resources.C --generate-source
resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gxsm4.gresources.xml)
gxsm4_resources.C: gxsm4.gresources.xml  $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name gxsm4 $<
gxsm4_resources.h: gxsm4.gresources.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name gxsm4 $<

#gxsm4.gresource: gxsm4.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gxsm4.gresource.xml)


# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@


deps = declare_dependency(
    compile_args : ['-DTESTING', '-fpermissive', '-w'],
    dependencies : [dgtk, dsoup, dgtksourceview, dcairo, dfreet, dgset, dnetcdfcpp, dfftw, dxml, dpng, dgmod, dgmodexp, dx11, dgl, dglew],
    include_directories : configuration_inc,
    link_args : ['-lpng', '-lGLEW', '-lz', '-lfreetype', '-licuuc', '-export-dynamic'],
    )


gnome = import('gnome')

# NEED THIS:
#    -DGNOME_ICONDIR=\""$(pixmapdir)"\"

# gxsm4_LDFLAGS = -export-dynamic $(GXSM4_LIBS)

# gxsm4_LDADD =  \
#	$(FFTW_LIBS) \
#	${libnetcdf_LIBS} \
#	-lpng -lGLEW -lGL -lz -lfreetype -licuuc -lX11

# gsettings_SCHEMAS is a list of all the schemas you want to install
#gsettings_SCHEMAS = \
#	org.gnome.gxsm4.gschema.xml \
#	org.gnome.gxsm4.window-geometry.gschema.xml \
#	org.gnome.gxsm4.preferences.gschema.xml \
#	org.gnome.gxsm4.gl.gschema.xml \
#	org.gnome.gxsm4.pcs.mainwindow.gschema.xml \
#	org.gnome.gxsm4.pcsadjustments.mainwindow.gschema.xml

#schemas = (
#    'org.gnome.gxsm4.gschema.xml',
#    'org.gnome.gxsm4.window-geometry.gschema.xml',
#    'org.gnome.gxsm4.preferences.gschema.xml',
#    'org.gnome.gxsm4.gl.gschema.xml',
#    'org.gnome.gxsm4.pcs.mainwindow.gschema.xml',
#    'org.gnome.gxsm4.pcsadjustments.mainwindow.gschema.xml',
#)

# The returned source would be passed to another target
#gdbus_src = gnome.gdbus_codegen('example-interface',
#  sources: 'com.example.Sample.xml',
#  interface_prefix : 'com.example.',
#  namespace : 'Sample',
#  annotations : [
#    ['com.example.Hello()', 'org.freedesktop.DBus.Deprecated', 'true']
#  ],
#  docbook : 'example-interface-doc'
#)
#
#executable(
#    meson.project_name(),
#    asschemas,
#    dependencies: my_deps,
#    install: true
#)

#
# build gxsm2_resource.cpp
# glib-compile-resources gxsm4_resources.xml --target=gxsm4_resources.cpp --generate-source
# resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gxsm4.gresources.xml)
# gxsm4_resources.cpp: gxsm4.gresources.xml  $(resource_files)
# 	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name gxsm4 $<
# gxsm4_resources.h: gxsm4.gresources.xml $(resource_files)
# 	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name gxsm4 $<

gxsm4resources = gnome.compile_resources(
    'gxsm4-resources', 'gxsm4.gresources.xml',
    c_name: 'gxsm4'
)

executable(
    meson.project_name(),
    'app_channelselector.cpp',
    'app_monitor.cpp',
    'app_profile.cpp',
    'app_spm.cpp',
    'app_v3dcontrol.cpp',
    'app_view.cpp',
    'app_vinfo.cpp',
    'app_vobj.cpp',
    
    'cairo_item.cpp',
    'clip.c',
    'compiler.cpp',
    'csv.cpp',
    'caps.cpp',
    'error.cpp',
    
    'dataio.cpp',
    'epsfutils.cpp',
    'gnome-res.c',
    'gtkmarshal.c',
    'gtkmarshalers.c',
    'gpl.cpp',
    
    'gxsm_main.cpp',
    'gxsm_app.cpp',
    'gxsm4_resources.cpp',
    'gxsm_window.cpp',
    'gapp_service.cpp',
    'gxsm_menucb.cpp',
    'gxsm_menu-extension.c',
    'gxsm_resoucetable.cpp',
    
    'instrument.cpp',
    'lineprofile.cpp',
    'mem2d.cpp',
    'monitor.cpp',
    'pcs.cpp',
    'plugin_ctrl.cpp',
    'pugixml.cpp',
    'readpng.cpp',
    'regress.cpp',
    'remote.cpp',
    'scan.cpp',
    'scan_event.cpp',
    'spascan.cpp',
    'surface.cpp',
    'topographic.cpp',
    'util.cpp',
    'view.cpp',
    'vpdata_read.cpp',
    'vprofile.cpp',
    'vsurf3d.cpp',
    'writepng.cpp',
    'xshmimg.cpp',
    'xsm.cpp',
    'xsmhard.cpp',
    'xsmmath.cpp',
    gxsm4resources,
    dependencies : deps,
    install: true
)

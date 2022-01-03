
deps = declare_dependency(
    compile_args : ['-DTESTING', '-fpermissive', '-w'],
    dependencies : [icuuc, gtk, soup, gtksourceview, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp, x11, gl, glew],
    include_directories : configuration_inc,
    link_args : [ '-licuuc', '-lnetcdf', '-lnetcdf_c++'],
    )

gnome = import('gnome')

# NEED THIS:
#    -DGNOME_ICONDIR=\""$(pixmapdir)"\"
# gxsm4_LDFLAGS = -export-dynamic $(GXSM4_LIBS)


# The returned source would be passed to another target
gxsm4_gdbus_src = gnome.gdbus_codegen('gxsm4-core-gschema',
  sources: [
      'org.gnome.gxsm4.gschema.xml',
      'org.gnome.gxsm4.window-geometry.gschema.xml',
      'org.gnome.gxsm4.preferences.gschema.xml',
      'org.gnome.gxsm4.gl.gschema.xml',
      'org.gnome.gxsm4.pcs.mainwindow.gschema.xml',
      'org.gnome.gxsm4.pcsadjustments.mainwindow.gschema.xml'
      ],
  interface_prefix : 'org.gnome.',
  namespace : 'gxsm4'
)

gxsm4_resources = gnome.compile_resources(
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
    gxsm4_resources,
    gxsm4_gdbus_src,
    dependencies : deps,
    export_dynamic: true,
    install: true
)


deps = declare_dependency(
    compile_args : all_cflags,
    dependencies : [icuuc, gtk, soup, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp, x11, gl, glew],
    include_directories : configuration_inc,
)

gnome = import('gnome')

# NEED THIS:
#    -DGNOME_ICONDIR=\""$(pixmapdir)"\"
# gxsm4_LDFLAGS = -export-dynamic $(GXSM4_LIBS)


# The returned source would be passed to another target
gxsm4_gdbus_src += gnome.gdbus_codegen('gxsm4-core-gschema',
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


lib_global_gxsm = library(
    lib_global_gxsm_name,
    'gxsm_global.cpp',
    'gxsm_resoucetable.cpp',
    'util.cpp',
    gxsm4_resources,
    dependencies : deps,
    install: true
)

lib_pcs_gxsm = library(
    lib_pcs_gxsm_name,
    'pcs.cpp',
    dependencies : deps,
    link_with: [ lib_global_gxsm ],
    install: true
)

lib_utils_gxsm = library(
    lib_utils_gxsm_name,
    'cairo_item.cpp',
    'clip.c',
    'csv.cpp',
    'caps.cpp',
    'gnome-res.c',
    'gtkmarshal.c',
    'gtkmarshalers.c',
    'gpl.cpp',
    'pugixml.cpp',
    'readpng.cpp',
    'writepng.cpp',
    'regress.cpp',

    dependencies : deps,
    link_with: [ lib_global_gxsm, lib_pcs_gxsm ],
    install: true
)


lib_app_gxsm = library(
    lib_app_gxsm_name,
    'compiler.cpp',

    'gapp_service.cpp',
    'app_channelselector.cpp',
    'app_monitorcontrol.cpp',
    'app_profile.cpp',
    'app_spm.cpp',
    'app_v3dcontrol.cpp',
    'app_view.cpp',
    'app_vinfo.cpp',
    'app_vobj.cpp',
    
    'dataio.cpp',
    'epsfutils.cpp',
   
    'gxsm_window.cpp',
    'gxsm_app.cpp',
    'gxsm4_resources.cpp',
    'gxsm_menucb.cpp',
    'gxsm_menu-extension.c',
    
    'instrument.cpp',
    'lineprofile.cpp',
    'mem2d.cpp',
    'monitor.cpp',
    'plugin_ctrl.cpp',
    'remote.cpp',
    'scan.cpp',
    'scan_event.cpp',
    'spascan.cpp',
    'surface.cpp',
    'topographic.cpp',
    'view.cpp',
    'vpdata_read.cpp',
    'vsurf3d.cpp',
    'xshmimg.cpp',
    'xsm.cpp',
    'xsmhard.cpp',
    'xsmmath.cpp',
    dependencies : deps,
    link_with: [ lib_global_gxsm, lib_pcs_gxsm, lib_utils_gxsm ],
    install: true
)


executable(
    meson.project_name(),
    'gxsm_main.cpp',
    gxsm4_gdbus_src,
    dependencies : deps,
    link_with: [ lib_global_gxsm, lib_utils_gxsm, lib_app_gxsm, lib_pcs_gxsm ],
    export_dynamic: true,
    install: true
)

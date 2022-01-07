plugin_cat = 'control'
install_path = gxsm4_plugin_dir / plugin_cat

plugin_list = [
#    { 'name': 'pyremote',
#      'deps': [python, gtksourceview],
#      'gschemas': [
#          'org.gnome.gxsm4.pcs.plugin-libpyremote.gschema.xml',
#          'org.gnome.gxsm4.pcsadjustments.plugin-libpyremote.gschema.xml' ]},
    { 'name': 'extra_scan_info', },
    { 'name': 'queryDSPinfo', },
    { 'name': 'ProbeIndicator', },
    { 'name': 'PanView' },
#    { 'name': 'PanView', 'deps': [ gtk, cairo, ], },
]

#if libxxx.found()
#  plugins_list += {
#    'name': 'pluing-wlibxxx',
#    'deps': [ gtk3, libxxx, ],
#  }
#endif




foreach plugin : plugin_list
  plugin_name = plugin.get('name')
  plugin_sources = [ plugin.get('sources', plugin_name + '.cpp') ]
  plugin_deps = all_plugin_deps + plugin.get('deps', [])
  plugin_cflags = plugin.get('cflags', []) + all_cflags
  plugin_schemas = plugin.get('gschemas', [])

  if plugin_schemas.length() > 0
      plugin_gdbus_src = gnome.gdbus_codegen('gxsm4-'+plugin_cat+'-'+plugin_name+'-gschema',
        sources: plugin_schemas,
        interface_prefix : 'org.gnome.',
        namespace : 'gxsm4'
    )
  else
      plugin_gdbus_src = [] 
  endif
  
  if platform_windows
    plugin_rc = configure_file(
      input : gimp_plugins_rc.full_path(),
      output: plugin_name + '.rc',
      copy: true,
    )

    plugin_sources += windows.compile_resources(
      plugin_rc,
      args: [
        '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
        '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
        '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
      ],
      include_directories: [
        rootInclude, appInclude,
      ],
      depends: [ gitversion_h ],
    )
  endif

  deps = declare_dependency(
    compile_args : plugin_cflags,
    dependencies : plugin_deps,
    include_directories : [ configuration_inc, appInclude ]
  )

  library(plugin_name,
    plugin_sources,
    plugin_gdbus_src,
    link_with: [ lib_global_gxsm, lib_utils_gxsm, lib_app_gxsm, lib_pcs_gxsm ],
    dependencies: deps,
#    link_args : all_plugin_link_args,
    install: true,
    install_dir: install_path
  )
endforeach







## shell python3-config --ldflags  ==> , '-L/usr/lib/python3.9/config-3.9-x86_64-linux-gnu', '-L/usr/lib/x86_64-linux-gnu ', '-lcrypt', '-lpthread', '-ldl ', '-lutil'],

### NEED To DO THIS ###
# libtool shared runtime libraries (plugins)
#libtool: install: ranlib /usr/local/lib/gxsm4-plugins/math/libZ_usr_rescale.a
#libtool: install: /usr/bin/install -c .libs/libabs_scan.a /usr/local/lib/gxsm4-plugins/math/libabs_scan.a
#libtool: install: chmod 644 /usr/local/lib/gxsm4-plugins/math/libabs_scan.a
#libtool: install: ranlib /usr/local/lib/gxsm4-plugins/math/libabs_scan.a
#libtool: finish: PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/sbin" ldconfig -n /usr/local/lib/gxsm4-plugins/math

### not sure what to do here, nothing works:
## => RuntimeError: File 'plug-ins/common/libpyremote.so' could not be found
##    FAILED: meson-install 

#build_target('pyremote',
#        'pyremote.cpp',
#        dependencies : deps,
#        target_type : 'shared_module',
#        install_dir: install_path,
#        install: true,
#)
#
#shared_library('PanView',
#        'PanView.cpp',
#        dependencies : deps,
#        install_dir: install_path,
#        install: true,
#)
#
#shared_module('ProbeIndicator',
#        'ProbeIndicator.cpp',
#        dependencies : deps,
#        install_dir: install_path,
#        install: true,
#)
#
#shared_library('queryDSPinfo',
#        'queryDSPinfo.cpp',
#        dependencies : deps,
#        install_dir: install_path,
#        install: true,
#)
#
#shared_library('extra_scan_info',
#        'extra_scan_info.cpp',
#        dependencies : deps,
#        install_dir: install_path,
#        install: true,
#)

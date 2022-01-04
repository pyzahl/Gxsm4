install_path = get_option('prefix') / 'lib/gxsm4/plugins/control'
# install_path = plugin_libs_dir+'/control'

deps = declare_dependency(
    compile_args: ['-DG_LOG_DOMAIN="LibGimpModule"', '-DGIMP_MODULE_COMPILATION', '-fpermissive', '-w'],
    dependencies : [icuuc, gtk, soup, gtksourceview, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp, dl],
    include_directories : configuration_inc,
    link_args : ['-Mmodules', '-export-dynamic', '-fno-version'],
    )

gnome = import('gnome')

# The returned source would be passed to another target
gxsm4_gdbus_src += gnome.gdbus_codegen('gxsm4-control-gschema',
  sources: [
      'org.gnome.gxsm4.pcs.plugin-libspm-scancontrol.gschema.xml',
      'org.gnome.gxsm4.pcsadjustments.plugin-libspm-scancontrol.gschema.xml',
      'org.gnome.gxsm4.pcs.plugin-libinet-json-external-scandata.gschema.xml',
      'org.gnome.gxsm4.pcsadjustments.plugin-libinet-json-external-scandata.gschema.xml',
      'org.gnome.gxsm4.inet_json_settings.gschema.xml'
      ],
  interface_prefix : 'org.gnome.',
  namespace : 'gxsm4'
)

# libtool shared runtime libraries (plugins)

### not sure what to do here, nothing works:
## => RuntimeError: File 'plug-ins/common/libpyremote.so' could not be found
##    FAILED: meson-install 

shared_library('spm_scancontrol',
        'spm_scancontrol.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

shared_library('inet_json_external_scandata',
        'inet_json_external_scandata.cpp', 'jsmn.c',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

shared_library('multidim_movie_control',
        'multidim_movie_control.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

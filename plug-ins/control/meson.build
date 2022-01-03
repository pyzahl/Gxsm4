## run frist:  mln -s "*.C" "#1.cpp"

install_path = plugin_libs_dir+'/control'

deps = declare_dependency(
    compile_args: ['-DG_LOG_DOMAIN="LibGimpModule"', '-DGIMP_MODULE_COMPILATION', '-fpermissive', '-w'],
    dependencies : [icuuc, gtk, soup, gtksourceview, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp],
    include_directories : configuration_inc,
    link_args : ['-Mmodules', '-export-dynamic', '-fno-version'],
    )

gnome = import('gnome')

# The returned source would be passed to another target
control_gdbus_src = gnome.gdbus_codegen('gxsm4-control-gschema',
  sources: [
      'org.gnome.gxsm4.pcs.plugin-libspm-scancontrol.gschema.xml',
      'org.gnome.gxsm4.pcsadjustments.plugin-libspm-scancontrol.gschema.xml',
      'org.gnome.gxsm4.pcs.plugin-libinet-json-external-scandata.gschema.xml',
      'org.gnome.gxsm4.pcsadjustments.plugin-libinet-json-external-scandata.gschema.xml',
      'org.gnome.gxsm4.inet_json_settings.gschema.xml'
      ],
  interface_prefix : 'org.gnome.',
  namespace : 'gxsm4',
  docbook : 'gxsm4-control-parameters'
)

library('spm_scancontrol',
        'spm_scancontrol.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

library('inet_json_external_scandata',
        'inet_json_external_scandata.cpp', 'jsmn.c',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

library('multidim_movie_control',
        'multidim_movie_control.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

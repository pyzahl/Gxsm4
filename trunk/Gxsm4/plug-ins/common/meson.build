## run frist:  mln -s "*.C" "#1.cpp"

install_path = plugin_libs_dir+'/common'

deps = declare_dependency(
    compile_args: ['-DG_LOG_DOMAIN="LibGimpModule"', '-DGIMP_MODULE_COMPILATION', '-fpermissive', '-w'],
    dependencies : [icuuc, gtk, python, gtksourceview, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp],
    include_directories : configuration_inc,
    link_args : ['-Mmodules', '-export-dynamic', '-fno-version'],
    )

gnome = import('gnome')

# The returned source would be passed to another target
gdbus_src = gnome.gdbus_codegen('gxsm4-common-gschema',
  sources: [
      'org.gnome.gxsm4.pcs.plugin-libpyremote.gschema.xml',
      'org.gnome.gxsm4.pcsadjustments.plugin-libpyremote.gschema.xml',
      ],
  interface_prefix : 'org.gnome.',
  namespace : 'gxsm4',
  docbook : 'gxsm4-common-parameters'
)

##shell python3-config --ldflags

library('pyremote',
        'pyremote.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

library('PanView',
        'PanView.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

library('ProbeIndicator',
        'ProbeIndicator.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

library('queryDSPinfo',
        'queryDSPinfo.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

library('extra_scan_info',
        'extra_scan_info.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

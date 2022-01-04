install_path = get_option('prefix') / 'lib/gxsm4/plugins/common'
## install_path = plugin_libs_dir+'/common'

deps = declare_dependency(
    compile_args: ['-fpermissive', '-w'],
    dependencies : [icuuc, gtk, python, gtksourceview, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp, dl],
    include_directories : configuration_inc,
    link_args : ['-Mmodules', '-export-dynamic', '-fno-version']
)

gnome = import('gnome')

# The returned source would be passed to another target
gxsm4_gdbus_src += gnome.gdbus_codegen('gxsm4-common-gschema',
  sources: [
      'org.gnome.gxsm4.pcs.plugin-libpyremote.gschema.xml',
      'org.gnome.gxsm4.pcsadjustments.plugin-libpyremote.gschema.xml',
      ],
  interface_prefix : 'org.gnome.',
  namespace : 'gxsm4'
)

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

build_target('pyremote',
        'pyremote.cpp',
        dependencies : deps,
        target_type : 'shared_module',
        install_dir: install_path,
        install: true,
)

shared_library('PanView',
        'PanView.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

shared_module('ProbeIndicator',
        'ProbeIndicator.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

shared_library('queryDSPinfo',
        'queryDSPinfo.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

shared_library('extra_scan_info',
        'extra_scan_info.cpp',
        dependencies : deps,
        install_dir: install_path,
        install: true,
)

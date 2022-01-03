## run frist:  mln -s "*.C" "#1.cpp"

install_path = '/usr/local/lib/gxsm4-plugins/control'  ## FIX-ME

deps = declare_dependency(
    compile_args : ['-fpermissive', '-w'],
    dependencies : [icuuc, gtk, soup, gtksourceview, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp],
    include_directories : configuration_inc,
    # link_args : ['-module', '-export-dynamic', '-avoid-version'],
    # link_args : ['-lz', '-licuuc', '-export-dynamic', '-lnetcdf', '-lnetcdf_c++', '-module', '-export-dynamic', '-avoid-version'],
    )

### => https://mesonbuild.com/Reference-manual_functions.htm#shared_module

## libspm_scancontrol.la   LDFLAGS = -module -export-dynamic -avoid-version
## libspm_scancontrol_la_SOURCES = spm_scancontrol.C

## libinet_json_external_scandata.la  LDFLAGS = -module -export-dynamic -avoid-version -lgsl -lgslcblas -lsoup-2.4
## libinet_json_external_scandata_la_SOURCES = inet_json_external_scandata.C jsmn.c jsmn.h resonance_fit.h

## libmultidim_movie_control.la LDFLAGS = -module -export-dynamic -avoid-version
## libmultidim_movie_control_la_SOURCES = multidim_movie_control.C


gnome = import('gnome')

# NEED THIS:
#    -DGNOME_ICONDIR=\""$(pixmapdir)"\"

# gxsm4_LDFLAGS = -export-dynamic $(GXSM4_LIBS)

# gxsm4_LDADD =  \
#	$(FFTW_LIBS) \
#	${libnetcdf_LIBS} \
#	-lpng -lGLEW -lGL -lz -lfreetype -licuuc -lX11


# The returned source would be passed to another target
gdbus_src = gnome.gdbus_codegen('gxsm4-control-gschema',
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

shared_module('spm_scancontrol',
               'spm_scancontrol.cpp',
               dependencies : deps,
               install_dir: install_path,
               install: true
)

executable(
    meson.project_name(),
    dependencies : deps,
    install: true
)

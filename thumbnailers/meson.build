
deps_nc4 = declare_dependency(
    compile_args : all_cflags,
    dependencies : [glib, netcdf4, png, popt],
)

executable(
    'nctopng',
    'nc4topng.cpp',
    'writepng.cpp',
    dependencies : deps_nc4,
    install: true
)

install_data(
  'gxsm-nc.thumbnailer',
  install_dir: join_paths(get_option('datadir'), 'thumbnailers')
)

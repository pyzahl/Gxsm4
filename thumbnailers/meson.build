
deps = declare_dependency(
    compile_args : all_cflags,
    dependencies : [glib, netcdf, png, popt],
)

executable(
    'nctopng',
    'nctopng.cpp',
    'writepng.cpp',
    dependencies : deps,
    install: true
)

install_data(
  'gxsm-nc.thumbnailer',
  install_dir: join_paths(get_option('datadir'), 'thumbnailers')
)

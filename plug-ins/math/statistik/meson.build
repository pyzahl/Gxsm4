plugin_cat = 'math' # statsitik
install_path = gxsm4_plugin_dir / plugin_cat

plugin_list = [
    { 'name': 'add_trail' },
    { 'name': 'AngularAnalysis' },
    { 'name': 'autocorrelation' },
    { 'name': 'average_profile' },
#    { 'name': 'baseinfo' }, ## needs GTK4 porting
    { 'name': 'crosscorrelation' },
    { 'name': 'histogram' },
    { 'name': 'histoHOP' },
#    { 'name': 'islandlbl', 'sources': [ 'islandlbl.cpp', 'mathilbl.cpp' ] },
    { 'name': 'nndistribution' },
    { 'name': 'polarhist' },
    { 'name': 'slopeabs' },
    { 'name': 'slopedir' },
    { 'name': 'spasim' },
    { 'name': 'spasimkz' },
    { 'name': 'stepcount' },
    { 'name': 'VacancyLineAnalysis' },
#    { 'name': 'vorlage' },
]

if opencv.found()
    plugin_list += [    { 'name': 'OpenCV_match', 'deps': [opencv] },
                        { 'name': 'OpenCV_recenter', 'deps': [opencv]  },
                    ]
endif
    
settings_schemas_list = []

foreach plugin : plugin_list
  plugin_name = plugin.get('name')
  plugin_sources = [ plugin.get('sources', plugin_name + '.cpp') ]
  plugin_deps = plugin.get('deps', []) + all_plugin_deps
  plugin_cflags = plugin.get('cflags', []) + all_cflags
  plugin_schemas = plugin.get('gschemas', [])
  settings_schemas_list += plugin_schemas

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
      input : gxsm_plugins_rc.full_path(),
      output: plugin_name + '.rc',
      copy: true,
    )

    plugin_sources += windows.compile_resources(
      plugin_rc,
      args: [
        '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
        '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
        '--define', 'TOP_SRCDIR="@0@"'          .format(meson.current_source_dir()),
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
    dependencies: deps,
    link_with: [ lib_global_gxsm, lib_utils_gxsm, lib_app_gxsm, lib_pcs_gxsm ],
    install: true,
    install_dir: install_path
  )
endforeach

if plugin_schemas.length() > 0
      gnome.compile_schemas(depend_files: files(settings_schemas_list))
#      meson.add_install_script('glib-compile-schemas', schemas_dir)
endif


install_data(
    sources: settings_schemas_list,
    install_dir : schemas_dir
)

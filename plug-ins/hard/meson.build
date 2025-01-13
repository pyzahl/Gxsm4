plugin_cat = 'hard'
install_path = gxsm4_plugin_dir / plugin_cat

plugin_list = [
    { 'name': 'spm_simulator_hwi',
      'gschemas': [ 'org.gnome.gxsm4.pcsadjustments.plugin-libspm-simulator-hwi.gschema.xml',
                    'org.gnome.gxsm4.pcs.plugin-libspm-simulator-hwi.gschema.xml',
		    'org.gnome.gxsm4.hwi.spm-sim-control.gschema.xml'] },
    { 'name': 'spm_template_hwi',
      'sources': [ 'spm_template_hwi.cpp',
		   'spm_template_hwi_dev.cpp',
		   'spm_template_hwi_eventhandling.cpp',
		   'spm_template_hwi_vectorgen.cpp',
		   'spm_template_hwi_emulator.cpp' ],
      'gschemas': [ 'org.gnome.gxsm4.pcsadjustments.plugin-libspm-template-hwi.gschema.xml',
                    'org.gnome.gxsm4.pcs.plugin-libspm-template-hwi.gschema.xml',
		    'org.gnome.gxsm4.hwi.spm-template-control.gschema.xml'] },
    { 'name': 'sranger_mk2_hwi',
      'sources': [ 'sranger_mk2_hwi.cpp',
                   'sranger_mk2_hwi_control.cpp',
                   'sranger_mk2_hwi_mover.cpp',
                   'sranger_mk2_hwi_spm.cpp',
                   'sranger_mk2_hwi_dev.cpp',
                   'sranger_mk2_hwi_eventhandling.cpp',
                   'sranger_mk2_hwi_vectorgen.cpp',
                   'sranger_mk3_hwi_dev.cpp',
                   'sranger_mk3_hwi_pac.cpp',
                   'sranger_mk3_hwi_spm.cpp' ],
      'gschemas': [ 'org.gnome.gxsm4.hwi.sranger_mk23.gschema.xml',
                    'org.gnome.gxsm4.hwi.sranger_mk23_mover.gschema.xml',
		    'org.gnome.gxsm4.pcs.plugin-libsranger-mk2-hwi.gschema.xml',
                    'org.gnome.gxsm4.pcsadjustments.plugin-libsranger-mk2-hwi.gschema.xml',
		    'org.gnome.gxsm4.hwi.rpspmc-control.gschema.xml'
                   ] },
#    { 'name': 'tc211_ccd' }, ## may be port that some day
]

if gsl.found() and soup.found() and websocketpp.found()
    plugin_list += [
    { 'name': 'rpspmc_pacpll',
      'sources' : ['rpspmc_pacpll.cpp',
      		   'rpspmc_pacpll_json_data.cpp',
		   'json_talk.cpp',
      		   '../control/jsmn.cpp',
		   'rpspmc_hwi_dev.cpp',
		   'rpspmc_stream.cpp',
		   'rpspmc_hwi_eventhandling.cpp',
		   'rpspmc_hwi_vectorgen.cpp',
		   'rpspmc_gvpmover.cpp'
		   ],
      'deps': [ soup, gsl, lz, websocketpp ],	
      'gschemas': [ 'org.gnome.gxsm4.rpspmc-pacpll-settings.gschema.xml',
                    'org.gnome.gxsm4.pcs.plugin-librpspmc-pacpll.gschema.xml',
                    'org.gnome.gxsm4.pcsadjustments.plugin-librpspmc-pacpll.gschema.xml',
		    'org.gnome.gxsm4.hwi.rpspmc-gvpmover.gschema.xml',
                    'org.gnome.gxsm4.pcsadjustments.plugin-librpspmc-gvpmover.gschema.xml'
		    ] },
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

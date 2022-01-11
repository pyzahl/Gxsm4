plugin_cat = 'scan'
install_path = gxsm4_plugin_dir / plugin_cat

plugin_list = [
    { 'name': 'ascii_data_im_export', },
    { 'name': 'sdfimport' },
    { 'name': 'bin_import' },
    { 'name': 'cube_import' },
    # { 'name': 'external_converter' }, ## did gxsm lib related ports, few GTK4 ports pending, missing show_info_callbcak ?!?!?
    # { 'name': 'converter' }, ## few GTK4 port pending, missing show_info_callback ?!?!?
    { 'name': 'primitiveimexport' },
    { 'name': 'PsiHDF_im_export' },
    { 'name': 'spa4_d2d_im_export' },
    { 'name': 'uksoft2001_im_export' },
    { 'name': 'wip_im_export' },
    { 'name': 'g_dat_im_export' },
    { 'name': 'gmeyer_im_export' },
    { 'name': 'nanoimport' },
    { 'name': 'omicron_io' },
    { 'name': 'png_im_export' },
    { 'name': 'rhk2000_import' },
    { 'name': 'v5d_export', 'sources': ['v5d_binio.cpp', 'v5d_v5d.cpp'] },
    { 'name': 'rhk_spm32_import' },
    { 'name': 'WSxM_io', 'sources': ['WSxM_header.cpp'] },
]

if quicktime.found()
    plugin_list += [ { 'name': 'quicktime_im_export', 'deps' : [quicktime] }, ]
endif

foreach plugin : plugin_list
  plugin_name = plugin.get('name')
  plugin_sources = [ plugin.get('sources', plugin_name + '.cpp') ]
  plugin_deps = plugin.get('deps', []) + all_plugin_deps
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
    dependencies: deps,
    link_with: [ lib_global_gxsm, lib_utils_gxsm, lib_app_gxsm, lib_pcs_gxsm ],
    install: true,
    install_dir: install_path
  )
endforeach


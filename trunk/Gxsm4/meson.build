project('gxsm4', 'cpp', version : '4.0.0')

as_version = meson.project_version() # set in project() below
ver_arr = as_version.split('.')

as_major_version = ver_arr[0]
as_minor_version = ver_arr[1]
as_micro_version = ver_arr[2]

soup = dependency('libsoup-2.4', version : '>= 2.24')
gtk  = dependency('gtk4', version : '>= 4.1')
gtksourceview = dependency('gtksourceview-5', version : '>= 5.0')

conf = configuration_data()
# Surround the version in quotes to make it a C string
conf.set_quoted('VERSION', as_version)

conf.set('GXSM_VERSION_NAME', '"GXSM4GTK4 Beta Evaluation"') # [GXSM version identifier screen name]
conf.set('GXSM_RES_BASE_PATH_DOT',     '"org.gnome.gxsm4"') # [GSettings and Builder dot base path without leadiong or trailing slashes]
conf.set('GXSM_RES_BASE_PATH',         '"org/gnome/gxsm4"') # [GSettings and Builder base path without leadiong or trailing slashes]
conf.set('GXSM_RES_PREFERENCES_PATH',  '"org/gnome/gxsm4/preferences"') # [GSettings path for preferences]
conf.set('GXSM_RES_GL_PATH',           '"org/gnome/gxsm4/gl"') # [GSettings path for gl preferences]
conf.set('GXSM_MONITOR_VMEMORY_USAGE', '') # [define this for adding a time stamp in sec and VmSize to Ev_log file]

# conf.set('GXSM_ENABLE_SCAN_USER_PLUGIN_PATH', 'yes') # [enable scanning in user path for plugins]
conf.set('XSM_DEBUG_OPTION', '"yes"') # [Global Enable/Disable of any debugging code]
#conf.set('COMPILEDBYNAME', os.environ['USER']+'@'+os.environ['HOSTNAME']+'; `date`') # [GXSM user@host CC-time info string]
conf.set('COMPILEDBYNAME', '"PyZahl@phenom -- FIX-ME in meson.build"') # [GXSM user@host CC-time info string]

### HOW correctly to do this one???
#dnl Set PACKAGE_PALETTE_DIR in config.h.
#if test 'x${prefix}' = 'xNONE'; then
#  AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR, '${ac_default_prefix}/share/${PACKAGE}/palette', [Define gxsm package install prefix])
#else
#  AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR, '${prefix}/share/${PACKAGE}/palette', [Define gxsm package install prefix])
#fi
# conf.set('AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR', os.environ['MESON_INSTALL_PREFIX']+'/share/gxsm4/palette') # [Define gxsm package install prefix])
conf.set('AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR', '"/usr/local/share/gxsm4/palette"') # [Define gxsm package install prefix])

#dnl Set PACKAGE_GL400_DIR in config.h.
#if test 'x${prefix}' = 'xNONE'; then
#  AC_DEFINE_UNQUOTED(PACKAGE_GL400_DIR, '${ac_default_prefix}/share/${PACKAGE}/gl-400', [Define gxsm package install prefix])
#else
#  AC_DEFINE_UNQUOTED(PACKAGE_GL400_DIR, '${prefix}/share/${PACKAGE}/gl-400', [Define gxsm package install prefix])
#fi
# conf.set('PACKAGE_GL400_DIR', os.environ['MESON_INSTALL_PREFIX']+'/share/gxsm4/gl-400') # [Define gxsm package install prefix]
conf.set('PACKAGE_GL400_DIR', '"/usr/local/share/gxsm4/gl-400"') # [Define gxsm package install prefix]


configure_file(output : 'config.h',
               configuration : conf)

cc = meson.get_compiler('cpp')
cdata = configuration_data()
check_headers = [
  ['HAVE_LQT_QUICKTIME_H', 'lqt/quicktime.h'],
  ['HAVE_PYTHON3_7M_PYTHON_H', 'python3.7m/python.h'],
  ['HAVE_PYTHON3_7_PYTHON_H', 'python3.7/python.h'],
  ['HAVE_PYTHON3_8M_PYTHON_H', 'python3.8m/python.h'],
  ['HAVE_PYTHON3_8_PYTHON_H', 'python3.8/python.h'],
  ['HAVE_PYTHON3_9_PYTHON_H', 'python3.9/python.h'],
  ['HAVE_PYTHON3_9M_PYTHON_H', 'python3.9m/python.h'],
]

foreach h : check_headers
  if cc.has_header(h.get(1))
    cdata.set(h.get(0), 1)
  endif
endforeach

#dependency('netcdf', language: 'c')   # for the C NetCDF headers and libraries
dependency('netcdf', language: 'cpp') # for the C++ NetCDF headers and libraries

subdir('core-source')
#subdir('plug-ins')
#subdir('gl-400')
#subdir('palette')
#subdir('thumbnailers')
#subdir('tools')
#subdir('icons')


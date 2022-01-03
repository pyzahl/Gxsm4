project('gxsm4', ['c','cpp'], version : '4.0.0')

as_version = meson.project_version() # set in project() below
ver_arr = as_version.split('.')

as_major_version = ver_arr[0]
as_minor_version = ver_arr[1]
as_micro_version = ver_arr[2]

add_project_arguments('-DMYTEST=projname', language : 'cpp')

dglib = dependency('glib-2.0')
dgio = dependency('gio-2.0')
dgtk  = dependency('gtk4', version : '>= 4.1')
dsoup = dependency('libsoup-2.4', version : '>= 2.24')
dgtksourceview = dependency('gtksourceview-5', version : '>= 5.0')
dcairo = dependency('cairo-gobject')
dfreet =  dependency('freetype2')
dgset = dependency('gsettings-desktop-schemas')
dx11 = dependency('X11')
dgl = dependency('gl')
dglew = dependency('glew')
#dependency('netcdf', language: 'c')   # for the C NetCDF headers and libraries
dnetcdf = dependency('netcdf', language: 'cpp') # for the C++ NetCDF headers and libraries
dpng = dependency('libpng')
dfftw = dependency('fftw3')
## dlz =  dependency('libz')  # ???
dxml = dependency('libxml-2.0')
dgmod = dependency('gmodule-2.0')
dgmodexp = dependency('gmodule-export-2.0')
# dgdkpixbu = dependency('gdk-pixbuf-2.0')


conf = configuration_data()

conf.set('PACKAGE', '"'+meson.project_name()+'"')
conf.set('PACKAGE_NAME', '"'+meson.project_name()+'"')
conf.set('GETTEXT_PACKAGE', '"'+meson.project_name()+'"')
conf.set('VERSION', '"'+meson.project_version()+'"')

## fix-me: paths to be made auto!
conf.set('PACKAGE_PLUGIN_DIR', '"/usr/local/lib/gxsm4-plugins"', description : 'This is the path where gxsm plugins will be installed')
conf.set('PACKAGE_ICON_DIR', '"/usr/local/share/gxsm4/icons"', description : 'This is the path where icons will be installed')
conf.set('PACKAGE_PROFILEPLOT_DIR', '"/usr/local/share/gxsm4/profileplot"', description : 'defines gxsm profileplot scripts') ## obsoleted soon



conf.set('GXSM_VERSION_NAME', '"GXSM4GTK4 Beta Evaluation"', description : 'GXSM version identifier screen name')
conf.set('GXSM_RES_BASE_PATH_DOT',     '"org.gnome.gxsm4"', description : 'GSettings and Builder dot base path without leadiong or trailing slashes')
conf.set('GXSM_RES_BASE_PATH',         '"org/gnome/gxsm4"', description : 'GSettings and Builder base path without leadiong or trailing slashes')
conf.set('GXSM_RES_PREFERENCES_PATH',  '"org/gnome/gxsm4/preferences"', description : 'GSettings path for preferences')
conf.set('GXSM_RES_GL_PATH',           '"org/gnome/gxsm4/gl"', description : 'GSettings path for gl preferences')
conf.set('GXSM_MONITOR_VMEMORY_USAGE', '', description : 'define this for adding a time stamp in sec and VmSize to Ev_log file')

# conf.set('GXSM_ENABLE_SCAN_USER_PLUGIN_PATH', 'yes', description : 'enable scanning in user path for plugins')
conf.set('XSM_DEBUG_OPTION', '"yes"', description : 'Global Enable/Disable of any debugging code')
#conf.set('COMPILEDBYNAME', os.environ['USER']+'@'+os.environ['HOSTNAME']+'; `date`', description : 'GXSM user@host CC-time info string')
conf.set('COMPILEDBYNAME', '"PyZahl@phenom -- FIX-ME in meson.build"', description : 'GXSM user@host CC-time info string')

### HOW correctly to do this one???
#dnl Set PACKAGE_PALETTE_DIR in config.h.
#if test 'x${prefix}' = 'xNONE'; then
#  AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR, '${ac_default_prefix}/share/${PACKAGE}/palette', [Define gxsm package install prefix])
#else
#  AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR, '${prefix}/share/${PACKAGE}/palette', [Define gxsm package install prefix])
#fi
# conf.set('AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR', os.environ['MESON_INSTALL_PREFIX']+'/share/gxsm4/palette', description : 'Define gxsm package install prefix]')
conf.set('PACKAGE_PALETTE_DIR', '"/usr/local/share/gxsm4/palette"', description : 'Define gxsm package install prefix]')

#dnl Set PACKAGE_GL400_DIR in config.h.
#if test 'x${prefix}' = 'xNONE'; then
#  AC_DEFINE_UNQUOTED(PACKAGE_GL400_DIR, '${ac_default_prefix}/share/${PACKAGE}/gl-400', [Define gxsm package install prefix])
#else
#  AC_DEFINE_UNQUOTED(PACKAGE_GL400_DIR, '${prefix}/share/${PACKAGE}/gl-400', [Define gxsm package install prefix])
#fi
# conf.set('PACKAGE_GL400_DIR', os.environ['MESON_INSTALL_PREFIX']+'/share/gxsm4/gl-400', description : 'Define gxsm package install prefix')
conf.set('PACKAGE_GL400_DIR', '"/usr/local/share/gxsm4/gl-400"', description : 'Define gxsm package install prefix')

#

check_headers = [
  ['HAVE_LQT_QUICKTIME_H', 'lqt/quicktime.h'],
  ['HAVE_PYTHON3_7M_PYTHON_H', 'python3.7m/python.h'],
  ['HAVE_PYTHON3_7_PYTHON_H', 'python3.7/python.h'],
  ['HAVE_PYTHON3_8M_PYTHON_H', 'python3.8m/python.h'],
  ['HAVE_PYTHON3_8_PYTHON_H', 'python3.8/python.h'],
  ['HAVE_PYTHON3_9_PYTHON_H', 'python3.9/python.h'],
  ['HAVE_PYTHON3_9M_PYTHON_H', 'python3.9m/python.h'],
]

cc = meson.get_compiler('cpp')
foreach h : check_headers
  if cc.has_header(h.get(1))
    conf.set(h.get(0), 1)
  endif
endforeach

# write config.h from conf object
####################################
configure_file(output : 'config.h',
               configuration : conf)

rootpath = meson.project_source_root()


### some issue here, does not find config.h
configuration_inc = include_directories('.')

## include_directories('@0@/..'.format(meson.current_build_dir()))


# sub dirs to build
####################################

subdir('core-source')
#subdir('plug-ins')
#subdir('gl-400')
#subdir('palette')
#subdir('thumbnailers')
#subdir('tools')
#subdir('icons')

#subdir('po')

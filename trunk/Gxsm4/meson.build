project('gxsm4', ['c','cpp'], version : '4.0.0')
version_name = 'GXSM4GTK4 Beta Evaluation'

cc = meson.get_compiler('cpp')

as_version = meson.project_version() # set in project() below
ver_arr = as_version.split('.')

as_major_version = ver_arr[0]
as_minor_version = ver_arr[1]
as_micro_version = ver_arr[2]

# Dependencies: required libs
##################################################################

glib = dependency('glib-2.0')
gio = dependency('gio-2.0')
gtk  = dependency('gtk4', version : '>= 4.1')
soup = dependency('libsoup-2.4', version : '>= 2.24')
gtksourceview = dependency('gtksourceview-5', version : '>= 5.0')
cairo = dependency('cairo-gobject')
freet =  dependency('freetype2')
gset = dependency('gsettings-desktop-schemas')
x11 = dependency('X11')
gl = dependency('gl')
glew = dependency('glew')
# netcdf = dependency('netcdf-cxx4')   # for the C/C++ NetCDF headers and libraries
# netcdfc = dependency('netcdf', language: 'c')   # for the C NetCDF headers and libraries
# netcdfcpp = dependency('netcdf', language: 'cpp') # for the C++ NetCDF headers and libraries (need both!!)
netcdf = declare_dependency(
    dependencies: [
        cc.find_library('netcdf'),
        cc.find_library('netcdf_c++'),
    ]
)
png = dependency('libpng')
fftw = dependency('fftw3')
xml = dependency('libxml-2.0')
gmod = dependency('gmodule-2.0')
gmodexp = dependency('gmodule-export-2.0')
lz =  dependency('zlib', method : 'system')

icuuc = declare_dependency(
    dependencies: [
        cc.find_library('icuuc'),
    ],
)

python = dependency('python3')
# gdkpixbu = dependency('gdk-pixbuf-2.0')

## plugins do need for dynamic load lib aka "-ldl"
dl = declare_dependency(
    dependencies: [
        cc.find_library('dl'),
    ],
)


absolute_data_dir = get_option('prefix')+'/'+get_option('datadir')+'/gxsm4'
absolute_plugin_libs_dir = get_option('prefix')+'/lib/gxsm4-plugins'

## grab some info -- not essential, can defined const
today    = run_command('sh', '-c', 'date').stdout().strip()
user     = run_command('sh', '-c', 'echo $USER').stdout().strip()
hostname = run_command('sh', '-c', 'echo $HOSTNAME').stdout().strip()

## Build config.h
################################################################################

conf = configuration_data()

conf.set('__BUILD_PREFIX', '"'+get_option('prefix')+'"') # => /usr/local
conf.set('__BUILD_DATADIR', '"'+get_option('datadir')+'"') # -> share

conf.set('PACKAGE', '"'+meson.project_name()+'"')
conf.set('PACKAGE_NAME', '"'+meson.project_name()+'"')
conf.set('GETTEXT_PACKAGE', '"'+meson.project_name()+'"')
conf.set('VERSION', '"'+meson.project_version()+'"')
conf.set('GXSM_VERSION_NAME', '"'+version_name+'"', description : 'GXSM version identifier screen name')
conf.set('GXSM_BUILD_DATE',   '"'+today+'"')
conf.set('GXSM_MAJOR_VERSION', as_major_version)
conf.set('GXSM_MINOR_VERSION', as_minor_version)
conf.set('GXSM_MICRO_VERSION', as_micro_version)

## fix-me: paths to be made auto!
conf.set('PACKAGE_PLUGIN_DIR', '"'+absolute_plugin_libs_dir+'"', description : 'This is the path where gxsm plugins will be installed')
conf.set('PACKAGE_ICON_DIR',   '"'+absolute_data_dir+'/icons"', description : 'This is the path where icons will be installed')
conf.set('PACKAGE_PROFILEPLOT_DIR', '"'+absolute_data_dir+'/profileplot"', description : 'defines gxsm profileplot scripts') ## obsoleted soon


## SCHEMA RES PATHs
conf.set('GXSM_RES_BASE_PATH_DOT',     '"org.gnome.gxsm4"', description : 'GSettings and Builder dot base path without leadiong or trailing slashes')
conf.set('GXSM_RES_BASE_PATH',         '"org/gnome/gxsm4"', description : 'GSettings and Builder base path without leadiong or trailing slashes')
conf.set('GXSM_RES_PREFERENCES_PATH',  '"org/gnome/gxsm4/preferences"', description : 'GSettings path for preferences')
conf.set('GXSM_RES_GL_PATH',           '"org/gnome/gxsm4/gl"', description : 'GSettings path for gl preferences')
conf.set('GXSM_MONITOR_VMEMORY_USAGE', '', description : 'define this for adding a time stamp in sec and VmSize to Ev_log file')

# conf.set('GXSM_ENABLE_SCAN_USER_PLUGIN_PATH', 'yes', description : 'enable scanning in user path for plugins')
conf.set('XSM_DEBUG_OPTION', '"yes"', description : 'Global Enable/Disable of any debugging code')
#conf.set('COMPILEDBYNAME', os.environ['USER']+'@'+os.environ['HOSTNAME']+'; `date`', description : 'GXSM user@host CC-time info string')
conf.set('COMPILEDBYNAME', '"'+user+'@'+hostname+' '+today+'"', description : 'GXSM user@host CC-time info string')
conf.set('PACKAGE_PALETTE_DIR', '"'+absolute_data_dir+'/palette"', description : 'Define gxsm palette install prefix]')
conf.set('PACKAGE_GL400_DIR', '"'+absolute_data_dir+'/gl-400"', description : 'Define gxsm GLSL code install prefix')

# write config.h from conf object
####################################
configure_file(output : 'config.h',
               configuration : conf)

rootpath = meson.project_source_root()

### some issue here, does not find config.h
configuration_inc = include_directories('.')

# sub dirs to build
####################################

subdir('core-source')
subdir('plug-ins')
subdir('gl-400')
subdir('palette')
subdir('icons')

#subdir('thumbnailers')
#subdir('tools')
#subdir('po')

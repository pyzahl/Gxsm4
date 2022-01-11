project('gxsm4', ['c','cpp'], version : '4.0.0')
version_name = 'GXSM4GTK4 Beta Evaluation'

as_version = meson.project_version() # set in project() below
ver_arr = as_version.split('.')

as_major_version = ver_arr[0]
as_minor_version = ver_arr[1]
as_micro_version = ver_arr[2]

################################################################################
# Get configuration and Meson modules

pkgconfig = import('pkgconfig')
i18n      = import('i18n')
gnome     = import('gnome')
pythonmod = import('python')
cc        = meson.get_compiler('c')
cxx       = meson.get_compiler('cpp')
prefix    = get_option('prefix')
buildtype = get_option('buildtype')
compiler_args = []
linker_args   = []

conf      = configuration_data()

################################################################################
# Host system detection

host_cpu_family = host_machine.cpu_family()
message('Host machine cpu family: ' + host_cpu_family)

host_cpu_family = host_machine.cpu_family()
if   host_cpu_family == 'x86'
  have_x86 = true
  conf.set10('ARCH_X86',    true)
  conf.set('WORDS_BIGENDIAN', 0)

elif host_cpu_family == 'x86_64'
  have_x86 = true
  conf.set10('ARCH_X86',    true)
  conf.set10('ARCH_X86_64', true)
  conf.set('WORDS_BIGENDIAN', 0)
elif host_cpu_family == 'ppc'
  have_ppc = true
  conf.set10('ARCH_PPC',    true)
  conf.set('WORDS_BIGENDIAN', 1)
elif host_cpu_family == 'ppc64'
  have_ppc = true
  conf.set10('ARCH_PPC',    true)
  conf.set10('ARCH_PPC64',  true)
  conf.set('WORDS_BIGENDIAN', 1)
endif


host_os = host_machine.system().to_lower()
message('Host os: ' + host_os)

platform_linux = (
  host_os.contains('linux')
)

platform_windows = (
  host_os.contains('mingw') or
  host_os.contains('cygwin') or
  host_os.contains('windows')
)

platform_osx = (
  host_os.contains('machten') or
  host_os.contains('rhapsody') or
  host_os.contains('darwin')
)

if platform_osx
  conf.set('PLATFORM_OSX', 1)
endif

if platform_windows
  windows = import('windows')
  # AC_CHECK_PROG(ms_librarian, lib.exe, yes, no)
  # AM_CONDITIONAL(MS_LIB_AVAILABLE, test "x$ms_librarian" = xyes)
  # compiler_args += '-Wl,--large-address-aware'
endif

# on OSX ObjC and C sources are mixed so adding objc to the linkflags

osx_ldflags = []
if platform_osx
  add_languages('objc')
  osx_ldflags += ['-Wl,-framework,Foundation', '-Wl,-framework,AppKit', '-ObjC']
  add_project_link_arguments(osx_ldflags, language : ['objc', 'c'])
endif

if cc.get_id() == 'gcc' and cc.version() == '7.2.0'
  gcc_warning = '''
    GCC 7.2.0 has a serious bug affecting GEGL/GIMP. We advise
    against using this version of the compiler (previous and
    further versions are fine).
    See https://bugzilla.gnome.org/show_bug.cgi?id=787222
  '''
  warning(gcc_warning)
  warnings += gcc_warning
endif


# Dependencies: required libs
################################################################################
no_dep = dependency('', required: false)

# Mandatory Dependencies
math = cc.find_library('m')
glib = dependency('glib-2.0')
gio  = dependency('gio-2.0')
gtk  = dependency('gtk4', version : '>= 4.1')
soup = dependency('libsoup-2.4', version : '>= 2.24')
cairo = dependency('cairo-gobject')
#cairo = dependency('cairo')
freet = dependency('freetype2')
gset  = dependency('gsettings-desktop-schemas')
x11   = dependency('x11')
gl    = dependency(['gl'], required : false)
#glew  = dependency(['glew'], required : false) ## work perfect for local Debian Testing build, not recognized via flatpak 
glew  = cc.find_library('GLEW')   ## required : false)  ## well, still required, compile will fails with missing GL/gl.h/glm.h.....

build_gxsm_view3d = false
if gl.found() and glew.found() and build_gxsm_view3d
    conf.set('ENABLE_3DVIEW_HAVE_GL_GLEW', 1)
else
    conf.set('ENABLE_3DVIEW_HAVE_GL_GLEW', 0)
endif

# netcdf = dependency('netcdf-cxx4')   # for the C/C++ NetCDF headers and libraries, new but incopatible w older version and gxsm
# netcdfc = dependency('netcdf', language: 'c')   # for the C NetCDF headers and libraries
# netcdfcpp = dependency('netcdf', language: 'cpp') # for the C++ NetCDF headers and libraries (need both!!)
netcdf = declare_dependency(
    dependencies: [
        cc.find_library('netcdf'),      ## OK
        cc.find_library('netcdf_c++'),  ## need this 3.2 legacy verion, use this local in Debain Testing
    ]
)

fontconfig = dependency('fontconfig')

png   = dependency('libpng')
fftw  = dependency('fftw3')
xml   = dependency('libxml-2.0')
gmod  = dependency('gmodule-2.0')
gmodexp = dependency('gmodule-export-2.0')
#lz    = dependency('zlib', method : 'system')
lz    = cc.find_library('z')

icuuc = cc.find_library('icuuc')

# inet-json... for fitting
gsl   = dependency('gsl', required : false)
##        dependency('gslcblas', required : false)

build_gxsm_view3d = false

if gsl.found()
  conf.set('ENABLE_FITTING_HAVE_GSL', 1)
else
  conf.set('ENABLE_FITTING_HAVE_GSL', 0)
endif

# for pyremote...
gtksourceview = dependency('gtksourceview-5', version : '>= 5.0')
python = dependency(['python3-embed'])
# gdkpixbu = dependency('gdk-pixbuf-2.0')

## plugins do need for dynamic load lib aka "-ldl"
dl   = platform_windows ? no_dep : cc.find_library('dl')

## OpenCV for feature match, etc. plugins:  old: -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_objdetect
opencv = dependency('opencv4', required : false)
if opencv.found()
  conf.set('ENABLE_FEATUREMATCH_HAVE_OPENCV', 1)
else
  conf.set('ENABLE_FEATUREMATCH_HAVE_OPENCV', 0)
endif

## export quicktime movie plugin
quicktime = dependency('libquicktime', required : false)
if quicktime.found()
  conf.set('ENABLE_MOVIEEXPORT_HAVE_QT', 1)
else
  conf.set('ENABLE_MOVIEEXPORT_HAVE_QT', 0)
endif


################################################################################
# Check if Pango is built with a recent fontconfig

pango_check = cc.links(
  '''#include <fontconfig/fontconfig.h>
  int main() {
    FcObjectSet *os; os = FcObjectSetBuild (FC_FAMILY, FC_WIDTH);
  }''',
  dependencies: fontconfig,
)

if not pango_check
  pango_warning = '\n  *** '.join([
  'You have a fontconfig >= fontconfig_required_version installed on your',
  'system, but your Pango library is using an older version. This old version',
  'is probably in /usr/X11R6. Look at the above output, and note that the',
  'result for FONTCONFIG_CFLAGS is not in the result for PANGOCAIRO_CFLAGS,',
  'and that there is likely an extra -I line, other than the ones for GLIB,',
  'Freetype, and Pango itself. That\'s where your old fontconfig files are.',
  'Rebuild pango, and make sure that it uses the newer fontconfig.',
  'The easiest way be sure of this is to simply get rid of the old',
  'fontconfig. When you rebuild pango, make sure the result for',
  'FONTCONFIG_CFLAGS is the same as the result here.',
  ])
  warning(pango_warning)
  warnings += pango_warning
endif



gxsm4_plugin_dir = get_option('libdir') / 'gxsm4-plugins'
gxsm4_data_dir   = get_option('datadir') / 'gxsm4'
localedir        = get_option('datadir') / 'locale'

lib_global_gxsm_name  =  'lib-global-'+meson.project_name() 
lib_pcs_gxsm_name     =  'lib-pcs-'+meson.project_name() 
lib_utils_gxsm_name   =  'lib-utils-'+meson.project_name() 
lib_app_gxsm_name     =  'lib-app-'+meson.project_name() 

absolute_data_dir = get_option('prefix') / get_option('datadir')+'/gxsm4'
absolute_plugin_dir = get_option('prefix') / get_option('libdir') / 'gxsm4-plugins'

gxsm_lib_includes = 'core-source'

## grab some info -- not essential, can defined const
today    = run_command('sh', '-c', 'date').stdout().strip()
user     = run_command('sh', '-c', 'echo $USER').stdout().strip()
hostname = run_command('sh', '-c', 'echo $HOSTNAME').stdout().strip()

## Build config.h
################################################################################


conf.set_quoted('M__BUILD_PREFIX',  prefix) # => /usr/local
conf.set_quoted('M__BUILD_DATADIR', get_option('datadir')) # -> share
conf.set_quoted('M__BINDIR',        prefix / get_option('bindir'))
conf.set_quoted('M__DATAROOTDIR',   prefix / get_option('datadir'))
conf.set_quoted('M__INFODIR',       prefix / get_option('infodir'))
conf.set_quoted('M__LIBDIR',        prefix / get_option('libdir'))

conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GXSM_VERSION_NAME', version_name, description : 'GXSM version identifier screen name')
conf.set_quoted('GXSM_BUILD_DATE',   today)
conf.set('GXSM_MAJOR_VERSION', as_major_version)
conf.set('GXSM_MINOR_VERSION', as_minor_version)
conf.set('GXSM_MICRO_VERSION', as_micro_version)

conf.set('USE_MAIN_GET_APP_IN_MEM2D_FOR_PROGRESS', 1)

## for Gxsm plugin autodetection (installed location)
conf.set_quoted('PACKAGE_PLUGIN_DIR', absolute_plugin_dir, description : 'This is the path where gxsm plugins will be installed')
conf.set_quoted('PACKAGE_ICON_DIR',   absolute_data_dir+'/icons', description : 'This is the path where icons will be installed')
conf.set_quoted('PACKAGE_PROFILEPLOT_DIR', absolute_data_dir+'/profileplot', description : 'defines gxsm profileplot scripts') ## obsoleted soon
## potential relative location
conf.set_quoted('GXSM_PLUGIN_DIR', gxsm4_plugin_dir)
conf.set_quoted('GXSM_DATA_DIR',   gxsm4_data_dir)


## SCHEMA RES PATHs
conf.set_quoted('GXSM_RES_BASE_PATH_DOT',     'org.gnome.gxsm4', description : 'GSettings and Builder dot base path without leadiong or trailing slashes')
conf.set_quoted('GXSM_RES_BASE_PATH',         'org/gnome/gxsm4', description : 'GSettings and Builder base path without leadiong or trailing slashes')
conf.set_quoted('GXSM_RES_PREFERENCES_PATH',  'org/gnome/gxsm4/preferences', description : 'GSettings path for preferences')
conf.set_quoted('GXSM_RES_GL_PATH',           'org/gnome/gxsm4/gl', description : 'GSettings path for gl preferences')
conf.set('GXSM_MONITOR_VMEMORY_USAGE', '', description : 'define this for adding a time stamp in sec and VmSize to Ev_log file')

# conf.set('GXSM_ENABLE_SCAN_USER_PLUGIN_PATH', 'yes', description : 'enable scanning in user path for plugins')
conf.set_quoted('XSM_DEBUG_OPTION', 'yes', description : 'Global Enable/Disable of any debugging code')
#conf.set('COMPILEDBYNAME', os.environ['USER']+'@'+os.environ['HOSTNAME']+'; `date`', description : 'GXSM user@host CC-time info string')
conf.set_quoted('COMPILEDBYNAME', user+'@'+hostname+' '+today, description : 'GXSM user@host CC-time info string')
conf.set_quoted('PACKAGE_PALETTE_DIR', absolute_data_dir+'/palette', description : 'Define gxsm palette install prefix]')
conf.set_quoted('PACKAGE_GL400_DIR', absolute_data_dir+'/gl-400', description : 'Define gxsm GLSL code install prefix')

# write config.h from conf object
####################################
configure_file(output : 'config.h',
               configuration : conf)

rootpath = meson.project_source_root()

### some issue here, does not find config.h
configuration_inc = include_directories('.')

# sub dirs to build
####################################

rootInclude = include_directories('.')
appInclude  = include_directories(gxsm_lib_includes)


all_cflags = ['-fpermissive', '-w']
all_plugin_cflags = ['-fpermissive', '-w']
all_plugin_deps = [icuuc, gtk, cairo, freet, gset, netcdf, fftw, xml, png, gmod, gmodexp, dl]
all_plugin_link_args = ['-Mmodules', '-export-dynamic', '-fno-version']


gxsm4_gdbus_src = files()

subdir('po')
subdir('core-source')
subdir('plug-ins')   ## still not figure how to build the .so plugin files with libtool ???
subdir('gl-400')
subdir('palette')
subdir('icons')

#subdir('thumbnailers')
#subdir('tools')



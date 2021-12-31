dnl Process this file with autoconf to produc a configure script.
# grep -rl gtk_object_ . | xargs sed -i s/gtk_object_/g_object_/g

AC_PREREQ([2.71])

m4_define(gxsm4_major_version, 4)
m4_define(gxsm4_minor_version, 0)
m4_define(gxsm4_micro_version, 0)
m4_define(gxsm4_version, gxsm4_major_version.gxsm4_minor_version.gxsm4_micro_version)

AC_INIT([gxsm4],[gxsm4_version],[http://sourceforge.net/tracker/?func=add&group_id=12992&atid=112992],[gxsm4],[http://gxsm.sf.net])

# AX_IS_RELEASE([minor_version])
# AX_REQUIRE_DEFINED([YELP_HELP_INIT])
# AX_REQUIRE_DEFINED([APPSTREAM_XML])
# AX_REQUIRE_DEFINED([GTK_DOC_CHECK])

AC_DEFINE_UNQUOTED(GXSM_VERSION_NAME, "GXSM4GTK4 Beta Evaluation","GXSM version identifier screen name")
AC_DEFINE_UNQUOTED(GXSM_WIDGET_PAD, 5, "default widget padding (replaces obsolete GNOME_PAD)")
AC_DEFINE_UNQUOTED(GXSM_RES_BASE_PATH_DOT,     "org.gnome.gxsm4",              "GSettings and Builder dot base path without leadiong or trailing slashes")
AC_DEFINE_UNQUOTED(GXSM_RES_BASE_PATH,         "org/gnome/gxsm4",              "GSettings and Builder base path without leadiong or trailing slashes")
AC_DEFINE_UNQUOTED(GXSM_RES_PREFERENCES_PATH,  "org/gnome/gxsm4/preferences",  "GSettings path for preferences")
AC_DEFINE_UNQUOTED(GXSM_RES_GL_PATH,           "org/gnome/gxsm4/gl",           "GSettings path for gl preferences")

AC_DEFINE_UNQUOTED(GXSM_MONITOR_VMEMORY_USAGE, "", "define this for adding a time stamp in sec and VmSize to Ev_log file")

## AC_DEFINE_UNQUOTED(GXSM_ENABLE_SCAN_USER_PLUGIN_PATH, "yes", "enable scanning in user path for plugins");

# Dependencies
GLIB_REQUIRED=2.48.1
 
GTK_REQUIRED=4.0.0
#GDK_REQUIRED=4.0.0

GTKSOURCEVIEW_REQUIRED=5.0.0
# LIBPEAS_REQUIRED=1.7.0
LIBXML_REQUIRED=2.5.0
# ENCHANT_REQUIRED=1.2.0
# ISO_CODES_REQUIRED=0.35
PYGOBJECT_REQUIRED=4.0.0

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(core-source/gxsm_main.C)
AC_CONFIG_MACRO_DIR([m4])

AC_DEFINE(GXSM4_MAJOR_VERSION, gxsm4_major_version, [Gxsm4 major version])
AC_SUBST(GXSM4_MAJOR_VERSION, gxsm4_major_version)
AC_DEFINE(GXSM4_MINOR_VERSION, gxsm4_minor_version, [Gxsm4 minor version])
AC_SUBST(GXSM4_MINOR_VERSION, gxsm4_minor_version)
AC_DEFINE(GXSM4_MICRO_VERSION, gxsm4_micro_version, [Gxsm4 micro version])
AC_SUBST(GXSM4_MICRO_VERSION, gxsm4_micro_version)

AM_INIT_AUTOMAKE([1.11 tar-ustar dist-xz no-dist-gzip -Wno-portability subdir-objects])
AM_MAINTAINER_MODE([enable])
# AM_GNU_GETTEXT([external])

AM_SILENT_RULES([yes])

# Checks for programs
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_YACC
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT
AC_SYS_LARGEFILE
PKG_PROG_PKG_CONFIG
# needed on osx
AC_PROG_OBJC


# Internationalisation
GETTEXT_PACKAGE=gxsm4
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[gettext package name])
IT_PROG_INTLTOOL([0.50.2])

# NetCDF
libnetcdf_LIBS="-lnetcdf -lnetcdf_c++"
AC_SUBST(libnetcdf_LIBS)

# NEW -- bullshit non compat
# AX_LIB_NETCDF4()


# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT(disable-static)

dnl ===============================================================
dnl Expanded dirs
dnl ===============================================================

# This macro expands DIR and assigns it to RET.
# If DIR is NONE, then it's replaced by DEFAULT.
# Based on AC_DEFINE_DIR
AC_DEFUN([GXSM4_FULLPATH], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_expand=[$]$1
  test "x$ac_expand" = xNONE && ac_expand="[$]$2"
  ac_expand=`eval echo [$]ac_expand`
  $3=`eval echo [$]ac_expand`
])

dnl FULL_LIBEXECDIR is used for X-GNOME-Bugzilla-ExtraInfoScript expansion
dnl in data/org.gnome.gxsm4.desktop.in.in
GXSM4_FULLPATH(libexecdir, NONE, FULL_LIBEXECDIR)
AC_SUBST(FULL_LIBEXECDIR)


AC_ARG_ENABLE(gtksourceview, AS_HELP_STRING([--enable-gtksourceview],[Syntax highlighting in Python wrapper console]),[],
                                    [enable_gtksourceview=yes])

dnl Subst proivate PACKAGE_ICON_DIR and set it in config.h.
if test "x${prefix}" = "xNONE"; then
        AC_DEFINE_UNQUOTED(PACKAGE_ICON_DIR, "${ac_default_prefix}/share/${PACKAGE}/icons",This is the path where icons will be instaled)
        PACKAGE_ICON_DIR="${ac_default_prefix}/share/${PACKAGE}/pixmaps"
else
        AC_DEFINE_UNQUOTED(PACKAGE_ICON_DIR, "${prefix}/share/${PACKAGE}/icons",This is the path where icons will be instaled)
        PACKAGE_ICON_DIR="${prefix}/share/${PACKAGE}/icons"
fi
AC_SUBST(PACKAGE_ICON_DIR)

dnl Subst PACKAGE_DATA_DIR and set it in config.h.
if test "x${prefix}" = "xNONE"; then
        AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${ac_default_prefix}/share/${PACKAGE}",This is the path where data will be instaled)
        PACKAGE_DATA_DIR="${ac_default_prefix}/share/${PACKAGE}"
else
        AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}",This is the path where data will be instaled)
        PACKAGE_DATA_DIR="${prefix}/share/${PACKAGE}"
fi
AC_SUBST(PACKAGE_DATA_DIR)

dnl Subst PACKAGE_DOC_DIR and set it in config.h.
if test "x${prefix}" = "xNONE"; then
        AC_DEFINE_UNQUOTED(PACKAGE_DOC_DIR, "${ac_default_prefix}/share/doc/${PACKAGE}",This is the path where docs will be instaled)
        PACKAGE_DOC_DIR="${ac_default_prefix}/share/doc/${PACKAGE}"
else
        AC_DEFINE_UNQUOTED(PACKAGE_DOC_DIR, "${prefix}/share/doc/${PACKAGE}",This is the path where docs will be instaled)
        PACKAGE_DOC_DIR="${prefix}/share/doc/${PACKAGE}"
fi
AC_SUBST(PACKAGE_DOC_DIR)

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${ac_default_prefix}/share/locale", [Define gxsm package local dir])
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/share/locale", [Define gxsm package local dir])
fi

dnl Set PACKAGE_PLUGIN_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_PLUGIN_DIR, "${ac_default_prefix}/lib/${PACKAGE}-plugins", [Define gxsm package plugin search path])
else
  AC_DEFINE_UNQUOTED(PACKAGE_PLUGIN_DIR, "${prefix}/lib/${PACKAGE}-plugins", [Define gxsm package plugin search path])
fi

if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_PROFILEPLOT_DIR, "${ac_default_prefix}/share/${PACKAGE}/profileplot", [Define gxsm profileplot scripts])
else
  AC_DEFINE_UNQUOTED(PACKAGE_PROFILEPLOT_DIR, "${prefix}/share/${PACKAGE}/profileplot", [Define gxsm profileplot scripts])
fi

dnl Set PACKAGE_PALETTE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR, "${ac_default_prefix}/share/${PACKAGE}/palette", [Define gxsm package install prefix])
else
  AC_DEFINE_UNQUOTED(PACKAGE_PALETTE_DIR, "${prefix}/share/${PACKAGE}/palette", [Define gxsm package install prefix])
fi

dnl Set PACKAGE_GL400_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_GL400_DIR, "${ac_default_prefix}/share/${PACKAGE}/gl-400", [Define gxsm package install prefix])
else
  AC_DEFINE_UNQUOTED(PACKAGE_GL400_DIR, "${prefix}/share/${PACKAGE}/gl-400", [Define gxsm package install prefix])
fi

dnl Check if WORDS are big endian (as Motorola, Sparc -- not Intel)
AC_C_BIGENDIAN

################
# Check for libz
################

if test -z "$LIBZ"; then
  AC_CHECK_LIB(z, gzsetparams,
    [AC_CHECK_HEADER(zlib.h,
      PSP='psp$(EXEEXT)'; LIBZ='-lz',
      [AC_MSG_WARN(*** PSP and PNG plug-ins will not be built (ZLIB header files not found) ***)])],
    [AC_MSG_WARN(*** PSP and PNG plug-ins will not be built (ZLIB library not found) ***)])
fi

AC_SUBST(LIBZ)
AC_SUBST(PSP)


##################
# Check for libpng
##################

AC_ARG_WITH(libpng,  [  --without-libpng        build without PNG support])
  if test x$with_libpng != xno && test -z "$LIBPNG" && test -n "$LIBZ"; then
    AC_CHECK_LIB(png, png_read_info,
      [AC_CHECK_HEADER(png.h,
        png_ok=yes,
        png_ok=no)],
      [AC_MSG_WARN(*** PNG plug-in will not be built (PNG library not found) ***)], -lz -lm)
    if test "$png_ok" = yes; then
      AC_MSG_CHECKING([for png_structp in png.h])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <png.h>]], [[png_structp pp; png_infop info; png_colorp cmap; png_create_read_struct;]])],[png_ok=yes],[png_ok=no])
      AC_MSG_RESULT($png_ok)
      if test "$png_ok" = yes; then
        PNG='png$(EXEEXT)'; LIBPNG="-lpng $LIBZ"
      else
        AC_MSG_WARN(*** PNG plug-in will not be built (PNG library is too old) ***)
      fi
    else
      AC_MSG_WARN(*** PNG plug-in will not be built (PNG header file not found) ***)
    fi
  fi

  if test x$with_libpng != xno && test -z "$LIBPNG"; then
     AC_MSG_ERROR([
*** Checks for PNG library failed. You can build without it by passing
*** --without-libpng to configure but you won't be able to use PNGs then.])
  fi

AC_SUBST(PNG)
AC_SUBST(LIBPNG)

#############################################################################
# GtkSourceView
if test "x$enable_gtksourceview" != "xno"; then
  PKG_CHECK_MODULES(GTKSOURCEVIEW,[gtksourceview-5],
                    [AC_DEFINE(HAVE_GTKSOURCEVIEW,1,
                               [Define if we have the GtkSourceView package.])],
                    [enable_gtksourceview=no; GTKSOURCEVIEW_WARN=" (needs GtkSourceView)"])
fi
if test "x$enable_gtksourceview" != "xno"; then
  GTKSOURCEVIEW_DEPENDENCY=gtksourceview-5
else
  GTKSOURCEVIEW_DEPENDENCY=
  GTKSOURCEVIEW_CFLAGS="$GTK_CFLAGS"
  GTKSOURCEVIEW_LIBS="$GTK_LIBS"
fi
AC_SUBST([GTKSOURCEVIEW_DEPENDENCY])

############################################################
# Checks for FFTW header and library.
############################################################

# Installed fftw library, if any.
FFTW_LIBS='-lfftw3'
AC_SUBST(FFTW_LIBS)

#have_fftw_header=no
#AC_CHECK_HEADERS(fftw3.h, [have_fftw_header=yes; break])
#if test "$have_fftw_header" = yes; then
#    AC_CHECK_LIB(fftw3, fftw_plan_dft_r2c_2d, FFTW_LIBS="-lfftw3")
#else
#    with_fftw=no
#fi
#
#if test "$with_fftw" = yes; then
  FFT_DIR=''
  AC_DEFINE(HAVE_FFTW, 1, [Define if the FFTW library is available.])
#fi

AC_DEFINE_UNQUOTED(XSM_DEBUG_OPTION, "yes", Global Enable/Disable of any debugging code)

# Info only
AC_DEFINE_UNQUOTED(COMPILEDBYNAME, "$USER@`hostname`; `date`", GXSM user@host CC-time info string)


# Quicktime library for Im/Export Plugin
AC_CHECK_HEADERS(lqt/quicktime.h)


# Now requires Python 3.7 or higher!
AC_CHECK_LIB(python3.7, main)
AC_CHECK_HEADERS(python3.7/Python.h)
AC_CHECK_LIB(python3.7m, main)
AC_CHECK_HEADERS(python3.7m/Python.h)

AC_CHECK_LIB(python3.8, main)
AC_CHECK_HEADERS(python3.8/Python.h)
AC_CHECK_LIB(python3.8m, main)
AC_CHECK_HEADERS(python3.8m/Python.h)

AC_CHECK_LIB(python3.9, main)
AC_CHECK_HEADERS(python3.9/Python.h)

# Checks for header files.
AC_PATH_X
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([argz.h fcntl.h langinfo.h libintl.h limits.h locale.h malloc.h memory.h nl_types.h stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UID_T

dnl Checks for library functions.
AC_FUNC_FNMATCH
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([__argz_count __argz_next __argz_stringify dcgettext floor getcwd gethostname getwd memchr memmove mempcpy memset mkdir munmap nl_langinfo pow putenv rint setenv setlocale sqrt stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strstr strtoul])

dnl Checks for OpenCV availability
dnl AC_SEARCH_LIBS([matchTemplate], [opencv_imgproc opencv_highgui opencv_ml opencv_video opencv_features2d opencv_calib3d opencv_objdetect opencv_contrib opencv_legacy opencv_flann], [
dnl  AC_DEFINE(HAS_OPENCV, 1, [Define if the OpenCV is available.])
dnl ], [
dnl  AC_MSG_ERROR([unable to find the opencv libraries. --> apt-get install libopencv-dev])
dnl ])


###############################

localedir=${datadir}/locale
AC_SUBST(localedir)

plugindir=${libdir}/gxsm4-plugins
AC_SUBST(plugindir)


###############################
# only for memory debugging
# -fpermissive -- to force ignore silly issue with C++/C type conversion to into after xor "|" of gtk flags....
CFLAGS="-O0 -g -fpermissive -w"
CXXFLAGS="-O0 -g -std=c++11 -fpermissive"
#OBJCFLAGS="-O0 -g"
#LTCFLAGS="-O0 -g"

# Dependencies

PKG_CHECK_MODULES(GXSM4, [
	libxml-2.0 >= $LIBXML_REQUIRED
	glib-2.0 >= $GLIB_REQUIRED
	gio-2.0 >= $GLIB_REQUIRED
	cairo-gobject
	gmodule-2.0
	gmodule-export-2.0
	gl
	netcdf
	fftw3
	gdk-pixbuf-2.0 >= gdk_pixbuf_minimum_version
	libsoup-2.4
	gsettings-desktop-schemas
	gtk4 >= $GTK_REQUIRED
	gtksourceview-5 >= $GTKSOURCEVIEW_REQUIRED
])

# gail-4.0

GXSM4_CFLAGS="$CFLAGS $GXSM4_CFLAGS $X11_CFLAGS $UNIX_CFLAGS"
GXSM4_LIBS="$GXSM4_LIBS $X11_LIBS $UNIX_LIBS"

AC_SUBST(GXSM4_CFLAGS)
AC_SUBST(GXSM4_LIBS)

dnl ================================================================
dnl libgd
dnl ================================================================

### LIBGD_INIT([tagged-entry static])

dnl ================================================================
dnl GSettings stuff
dnl ================================================================

GLIB_GSETTINGS

dnl ================================================================
dnl Misc
dnl ================================================================
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

# AC_DEFINE(GDK_VERSION_MIN_REQUIRED, GDK_VERSION_3_8, [minimum gdk version])

# GNOME_COMPILE_WARNINGS([maximum])
# AX_COMPILER_FLAGS([WARN_CFLAGS],[WARN_LDFLAGS])
AX_COMPILER_FLAGS()

AC_ARG_ENABLE(deprecations,
              [AS_HELP_STRING([--enable-deprecations],
                              [warn about deprecated usages [default=no]])],
              [enable_deprecations=$enableval], [])

AC_MSG_CHECKING([whether to forbid deprecated symbols])
if test "$enable_deprecations" = "yes"; then
	AC_MSG_RESULT(yes)
	DISABLE_DEPRECATED_CFLAGS="\
-DGSEAL_ENABLE \
-DG_DISABLE_DEPRECATED \
#-DGDK_DISABLE_DEPRECATED \
-DGTK_DISABLE_DEPRECATED \
#-DGDK_PIXBUF_DISABLE_DEPRECATED \
-DGNOME_DISABLE_DEPRECATED \
-DPEAS_DISABLE_DEPRECATED"

	AC_SUBST(DISABLE_DEPRECATED_CFLAGS)
else
	AC_MSG_RESULT(no)
fi

# on win32 plugins must be able to resolve all symbols at link time
# so we create a lib
if test "$platform_win32" = yes; then
	GXSM4_IMPLIB='-L${libdir}/gxsm4 -lgxsm4'
	PLUGIN_LIBTOOL_FLAGS="-module -avoid-version -no-undefined"
	if test "$os_win32" = yes; then
		PLUGIN_LIBTOOL_FLAGS="${PLUGIN_LIBTOOL_FLAGS} -mwindows"
	fi
else
	GXSM4_IMPLIB=""
	PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
fi
AC_SUBST(GXSM4_IMPLIB)
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

GXSM4_PLUGINS_DATA_DIR="$datadir/gxsm4/plugins"
AC_SUBST(GXSM4_PLUGINS_DATA_DIR)

GXSM4_PLUGINS_LIBS_DIR="$libdir/gxsm4/plugins"
AC_SUBST(GXSM4_PLUGINS_LIBS_DIR)

YELP_HELP_INIT

# Introspection

#GOBJECT_INTROSPECTION_REQUIRE

#GOBJECT_INTROSPECTION_CHECK([0.9.3])

#if test "$found_introspection" = "yes"; then
#	enable_introspection=yes
#	AC_DEFINE([ENABLE_INTROSPECTION], [1], [Define to enable GObject Introspection])
#else
#	enable_introspection=no
#fi

# Vala
#VAPIGEN_CHECK([0.25.1])


AC_CONFIG_FILES([
Makefile
core-source/Makefile
plug-ins/Makefile
plug-ins/common/Makefile
plug-ins/control/Makefile
plug-ins/math/Makefile
plug-ins/math/arithmetic/Makefile
plug-ins/math/background/Makefile
plug-ins/math/convert/Makefile
plug-ins/math/filter1d/Makefile
plug-ins/math/filter2d/Makefile
plug-ins/math/misc/Makefile
plug-ins/math/probe/Makefile
plug-ins/math/statistik/Makefile
plug-ins/math/transform/Makefile
plug-ins/scan/Makefile
plug-ins/hard/Makefile
palette/Makefile
gl-400/Makefile
tools/Makefile
thumbnailers/Makefile
icons/Makefile
po/Makefile.in
data/Makefile
])

AC_OUTPUT

echo "

Configuration:

	Source code location:	${srcdir}
	Compiler:		${CC}
	CFLAGS:			${CFLAGS}
	GXSM4_CFLAGS:		${GXSM4_CFLAGS}
	Spell Plugin enabled:	$enable_enchant
	Gvfs metadata enabled:	$enable_gvfs_metadata
	Deprecations enabled:	$enable_deprecations
	GObject Introspection:	$enable_introspection
	GDK Backend:            $gdk_windowing
	Python support:         $enable_python
	Vala support:           $enable_vala
"

if expr ${GXSM4_MINOR_VERSION} % 2 > /dev/null; then
	stable_version=`expr ${GXSM4_MINOR_VERSION} - 1`

cat << EOF
 *** IMPORTANT ***

This is not the lastest and producion approved version of Gxsm-4.

Thanks,
The Gxsm team

 *** END OF WARNING ***
EOF

fi

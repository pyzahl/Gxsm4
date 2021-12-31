SUBDIRS = core-source plug-ins gl-400 palette thumbnailers tools icons

# data
# po commented out, see discussion in the forums.

# man_MANS = man/gxsm2.1 man/vpdata2ascii.1 man/nctopng.1 man/gmetopng.1 man/rhkspm32topng.1 man/scalatopng.1

EXTRA_DIST = gxsm4.desktop

desktopdir = $(datadir)/applications
#desktop_in_files = gxsm4.desktop.in.in
#desktop_DATA 	 = $(desktop_in_files:.desktop.in.in=.1esktop)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gxsm4$(GUCHARMAP_API_PC_VERSION).pc

#desktop-file-validate: $(desktop_DATA)
#	for f in $^; do \
#		$(DESKTOP_FILE_VALIDATE) $$f; \
#	done

#check-local: desktop-file-validate

Applicationdir = $(datadir)/applications
gxsmdatadir = $(datadir)/gxsm4/data

#ac_default_prefix = /usr
ac_default_prefix = /usr/local/gxsm

#the application icon
appicondir=$(datadir)/icons/hicolor/scalable/apps
appicon_DATA=icons/gxsm4-icon.svg

#icons in the application
NULL =

private_icons = \
        gxsm4-icon.svg \
	arrow-up-symbolic.svg \
	arrow-down-symbolic.svg \
	arrow-left-symbolic.svg \
	arrow-right-symbolic.svg \
	goto-position-symbolic.svg \
	goto-center-symbolic.svg \
	goto-home-symbolic.svg \
	approach-symbolic.svg \
	process-stopall-symbolic.svg \
	retract-symbolic.svg \
	seek-backward-symbolic.svg \
	seek-forward-symbolic.svg \
	seek-left-symbolic.svg \
	seek-right-symbolic.svg \
	system-busy-symbolic.svg \
	system-off-symbolic.svg \
	xymode-symbolic.svg \
	$(NULL)

install-icons:
	for icon in $(private_icons); do \
		mkdir -p $(DESTDIR)$(pkgdatadir)/icons/; \
		$(INSTALL_DATA) $(srcdir)/icons/$$icon $(DESTDIR)$(pkgdatadir)/icons/; \
	done

install-data-local: install-icons


# gsettings_SCHEMAS is a list of all the schemas you want to install
#gsettings_SCHEMAS = org.gnome.desktop.gxsm4.gschema.xml 
# include the appropriate makefile rules for schema handling
#@GSETTINGS_RULES@


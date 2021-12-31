#!/bin/sh
#
# Installs a variety of Gnome 2.6 thumbnailers
#

# Set this to where Gnome is installed
GNOME_PREFIX=

echo "(re)installing thumbnailer rules for nautilus:"
export GCONF_CONFIG_SOURCE=xml::$GNOME_PREFIX/etc/gconf/gconf.xml.defaults
gconftool-2 --makefile-install-rule thumbnail.schemas

# you may change the destination directory here
echo "(re)installing thumbnailer scripte:"
install -v nautilus-gxsm-thumbnailer /usr/local/bin/
install -v nautilus-scala-thumbnailer /usr/local/bin/
install -v nautilus-gmedat-thumbnailer /usr/local/bin/
install -v nautilus-uksoft2001-thumbnailer /usr/local/bin/
install -v nautilus-rhk-spm32-thumbnailer /usr/local/bin/

# install mime type descriptions, check for destination path exisitance
if test -d "/usr/local/share/mime"; then
    if test -d "/usr/local/share/mime/packages"; then
	echo "(re)installing new mime types:"
    else
	echo "making packages directory for new mime types:"
	mkdir /usr/local/share/mime/packages
    fi
else
    echo "making packages directory for new mime types:"
    mkdir /usr/local/share/mime
    mkdir /usr/local/share/mime/packages
fi    

install -v omicron-scala.xml /usr/local/share/mime/packages/
install -v rhk-spm32.xml /usr/local/share/mime/packages/
install -v gme-data.xml /usr/local/share/mime/packages/
install -v uksoft2001.xml /usr/local/share/mime/packages/

# register new mime types: update database
update-mime-database /usr/local/share/mime

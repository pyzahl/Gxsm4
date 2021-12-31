# This is the Makefile.am for the "tools" of the gxsm project
#

DEFS = @DEFS@ \
        -Wall

# -DCOMPILEDBYNAME=\""$(USER)@`hostname`, `date`"\"

AM_CPPFLAGS = -I.. -I$(includedir) `pkg-config --cflags glib-2.0`

bin_PROGRAMS = \
	nctopng \
	scalatopng \
	gmetopng \
	uksoft2001topng \
	rhkspm32topng \
	vpdata2ascii \
	wizard \
	nc2top

nctopng_LDFLAGS = -export-dynamic -L$(libdir) `pkg-config --libs glib-2.0`
nctopng_LDADD = \
	-lnetcdf_c++ -lnetcdf \
	-lpng \
	-lpopt \
	$(INTLLIBS) # for i18n
nctopng_SOURCES = nctopng.C thumbpal.h writepng.C writepng.h


scalatopng_LDFLAGS = -export-dynamic -L$(libdir) `pkg-config --libs glib-2.0`
scalatopng_LDADD = -lpng -lpopt
scalatopng_SOURCES = scalatopng.C thumbpal.h writepng.C writepng.h


gmetopng_LDFLAGS = -export-dynamic -L$(libdir) `pkg-config --libs glib-2.0`
gmetopng_LDADD = -lpng -lz -lpopt
gmetopng_SOURCES = gmetopng.C thumbpal.h writepng.C writepng.h

uksoft2001topng_LDFLAGS = -export-dynamic -L$(libdir) `pkg-config --libs glib-2.0`
uksoft2001topng_LDADD = -lpng -lpopt
uksoft2001topng_SOURCES = uksoft2001topng.C thumbpal.h writepng.C writepng.h


rhkspm32topng_LDFLAGS = -export-dynamic -L$(libdir) `pkg-config --libs glib-2.0`
rhkspm32topng_LDADD = -lpng -lpopt
rhkspm32topng_SOURCES = rhkspm32topng.C thumbpal.h writepng.C writepng.h

vpdata2ascii_SOURCES = vpdata2ascii.c

wizard_SOURCES = wizard.c

nc2top_LDFLAGS = -export-dynamic -L$(libdir) `pkg-config --libs glib-2.0` -lpopt -L/usr/lib/netcdf-3/ -lnetcdf_c++ -lnetcdf -I/usr/include/netcdf-3 
nc2top_LDADD = -lpng ../plug-ins/scan/WSxM_header.C
nc2top_SOURCES = nc2top.C nc2top.h 


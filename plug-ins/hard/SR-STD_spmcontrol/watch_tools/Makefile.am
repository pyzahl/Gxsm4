pixmapdir = $(datadir)/pixmaps

INCLUDES = `pkg-config --cflags libgnomeui-2.0`

bin_PROGRAMS = spm_watch

spm_watch_SOURCES = \
	gtkled.c	\
	gtkled.h	\
	gtkledbar.c	\
	gtkledbar.h	\
	main.c

spm_watch_LDFLAGS = `pkg-config --libs libgnomeui-2.0` -static

spm_watch_LDADD = -lgtkdatabox




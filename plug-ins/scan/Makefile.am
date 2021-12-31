libdir = $(plugindir)/scan
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4

lib_LTLIBRARIES = \
	libnanoimport.la \
	libomicronio.la \
	libWSxMio.la \
	libprimitiveimexport.la \
        libUK2k_import.la \
	libPsiHDF_im_export.la \
	libspa4_d2d_im_export.la \
	libascii_data_im_export.la \
	libgmeyer_im_export.la \
	libg_dat_im_export.la \
	libpng_im_export.la \
	libsdfimport.la \
	libconverter.la \
	librhk2000_import.la \
	libuksoft2001_im_export.la \
	libquicktime_im_export.la \
	libwip_im_export.la \
	libv5d_export.la \
	libbin_import.la \
	libcube_import.la \
	librhk_spm32_import.la

#	libexternal_converter.la
#


libnanoimport_la_LDFLAGS = -module -export-dynamic -avoid-version
libnanoimport_la_SOURCES = nanoimport.C

libomicronio_la_LDFLAGS = -module -export-dynamic -avoid-version
libomicronio_la_SOURCES = omicron_io.C

libWSxMio_la_LDFLAGS = -module -export-dynamic -avoid-version
libWSxMio_la_SOURCES = WSxM_io.C WSxM_header.h WSxM_header.C

libsdfimport_la_LDFLAGS = -module -export-dynamic -avoid-version
libsdfimport_la_SOURCES = sdfimport.C

libbin_import_la_LDFLAGS = -module -export-dynamic -avoid-version
libbin_import_la_SOURCES = bin_import.C

libcube_import_la_LDFLAGS = -module -export-dynamic -avoid-version
libcube_import_la_SOURCES = cube_import.C

libprimitiveimexport_la_LDFLAGS = -module -export-dynamic -avoid-version
libprimitiveimexport_la_SOURCES = primitiveimexport.C

libPsiHDF_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libPsiHDF_im_export_la_SOURCES = PsiHDF_im_export.C

libspa4_d2d_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libspa4_d2d_im_export_la_SOURCES = spa4_d2d_im_export.C

libascii_data_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libascii_data_im_export_la_SOURCES = ascii_data_im_export.C

libUK2k_import_la_LDFLAGS = -module -export-dynamic -avoid-version
libUK2k_import_la_SOURCES = UK2k_import.C

libgmeyer_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libgmeyer_im_export_la_LIBADD  = -lz
libgmeyer_im_export_la_SOURCES = gmeyer_im_export.C

libg_dat_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libg_dat_im_export_la_SOURCES = g_dat_im_export.C

libuksoft2001_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version 
libuksoft2001_im_export_la_SOURCES = uksoft2001_im_export.C

libwip_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version 
libwip_im_export_la_SOURCES = wip_im_export.C

libquicktime_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libquicktime_im_export_la_LIBADD = -L/sw/lib -lquicktime
libquicktime_im_export_la_SOURCES = quicktime_im_export.C

# note: /sw/lib is for fink on darwin, should not distrub otherwise
libpng_im_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libpng_im_export_la_LIBADD = -L/sw/lib -lpng
libpng_im_export_la_SOURCES = png_im_export.C

libconverter_la_LDFLAGS = -module -export-dynamic -avoid-version
libconverter_la_SOURCES = converter.C

#libexternal_converter_la_LDFLAGS = -module -export-dynamic -avoid-version
#libexternal_converter_la_SOURCES = external_converter.C external_converter.h

librhk2000_import_la_LDFLAGS = -module -export-dynamic -avoid-version
librhk2000_import_la_SOURCES = rhk2000_import.C

librhk_spm32_import_la_LDFLAGS = -module -export-dynamic -avoid-version
librhk_spm32_import_la_SOURCES = rhk_spm32_import.C

libv5d_export_la_LDFLAGS = -module -export-dynamic -avoid-version
libv5d_export_la_SOURCES = v5d_export.C v5d_binio.C v5d_binio.h v5d_v5d.C v5d_v5d.h

libdir = $(plugindir)/math
AM_CPPFLAGS = $(GXSM4_CFLAGS) -I$(includedir) -I$(top_builddir)/gxsm4 -I/usr/include/opencv4/

lib_LTLIBRARIES = libaverage_profile.la \
	libAngularAnalysis.la \
	libVacancyLineAnalysis.la \
	libadd_trail.la \
	libstepcount.la \
	libnndistribution.la \
	libhistogram.la \
	libhistoHOP.la \
	libspasim.la \
	libspasimkz.la \
	libslopeabs.la \
	libslopedir.la \
	libpolarhist.la \
	libbaseinfo.la \
	libautocorrelation.la \
	libcrosscorrelation.la \
	libOpenCV_match.la \
	libOpenCV_recenter.la

#
#	libislandlbl.la \
#

libadd_trail_la_SOURCES = add_trail.C
libadd_trail_la_LDFLAGS = -module -export-dynamic -avoid-version

libbaseinfo_la_SOURCES = baseinfo.C
libbaseinfo_la_LDFLAGS = -module -export-dynamic -avoid-version

libOpenCV_match_la_SOURCES = OpenCV_match.C
libOpenCV_match_la_LDFLAGS = -module -export-dynamic -avoid-version -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_objdetect

### -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

libOpenCV_recenter_la_SOURCES = OpenCV_recenter.C
libOpenCV_recenter_la_LDFLAGS = -module -export-dynamic -avoid-version -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_objdetect

### -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

libnndistribution_la_SOURCES = nndistribution.C
libnndistribution_la_LDFLAGS = -module -export-dynamic -avoid-version

libhistogram_la_SOURCES = histogram.C
libhistogram_la_LDFLAGS = -module -export-dynamic -avoid-version

libpolarhist_la_SOURCES = polarhist.C
libpolarhist_la_LDFLAGS = -module -export-dynamic -avoid-version

libhistoHOP_la_SOURCES = histoHOP.C
libhistoHOP_la_LDFLAGS = -module -export-dynamic -avoid-version

libspasim_la_SOURCES = spasim.C
libspasim_la_LDFLAGS = -module -export-dynamic -avoid-version

libspasimkz_la_SOURCES = spasimkz.C
libspasimkz_la_LDFLAGS = -module -export-dynamic -avoid-version

libslopeabs_la_SOURCES = slopeabs.C
libslopeabs_la_LDFLAGS = -module -export-dynamic -avoid-version

libslopedir_la_SOURCES = slopedir.C
libslopedir_la_LDFLAGS = -module -export-dynamic -avoid-version

# libislandlbl_la_SOURCES = islandlbl.C mathilbl.C
# libislandlbl_la_LDFLAGS = -module -export-dynamic -avoid-version

libstepcount_la_SOURCES = stepcount.C
libstepcount_la_LDFLAGS = -module -export-dynamic -avoid-version

libautocorrelation_la_SOURCES = autocorrelation.C
libautocorrelation_la_LDFLAGS = -module -export-dynamic -avoid-version

libcrosscorrelation_la_SOURCES = crosscorrelation.C
libcrosscorrelation_la_LDFLAGS = -module -export-dynamic -avoid-version

libVacancyLineAnalysis_la_SOURCES = VacancyLineAnalysis.C
libVacancyLineAnalysis_la_LDFLAGS = -module -export-dynamic -avoid-version

libAngularAnalysis_la_SOURCES = AngularAnalysis.C
libAngularAnalysis_la_LDFLAGS = -module -export-dynamic -avoid-version

libaverage_profile_la_SOURCES = average_profile.C
libaverage_profile_la_LDFLAGS = -module -export-dynamic -avoid-version

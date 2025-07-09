SUBDIRS = plugins .

noinst_LTLIBRARIES = libopensand_sat.la

libopensand_sat_la_cpp = \
	BlockSatDispatcher.cpp \
	BlockSatAsymetricHandler.cpp

libopensand_sat_la_h = \
	BlockSatDispatcher.h \
	BlockSatAsymetricHandler.h

libopensand_sat_la_SOURCES = \
	$(libopensand_sat_la_cpp) \
	$(libopensand_sat_la_h)

libopensand_sat_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/physical_layer \
	-I$(top_srcdir)/src/common


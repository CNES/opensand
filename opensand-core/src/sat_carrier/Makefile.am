SUBDIRS = . tests

noinst_LTLIBRARIES = libopensand_satcarrier.la

libopensand_satcarrier_la_cpp = \
	BlockSatCarrier.cpp \
	sat_carrier_channel_set.cpp

libopensand_satcarrier_la_h = \
	BlockSatCarrier.h \
	sat_carrier_channel_set.h

libopensand_satcarrier_la_SOURCES = \
	$(libopensand_satcarrier_la_cpp) \
	$(libopensand_satcarrier_la_h)
libopensand_satcarrier_la_LIBADD = \
	$(PACKED_COMMON_LIBS)       

libopensand_satcarrier_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils

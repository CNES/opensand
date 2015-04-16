SUBDIRS = . tests

noinst_LTLIBRARIES = libopensand_satcarrier.la

libopensand_satcarrier_la_cpp = \
	BlockSatCarrier.cpp \
	sat_carrier_channel_set.cpp \
	sat_carrier_udp_channel.cpp

libopensand_satcarrier_la_h = \
	BlockSatCarrier.h \
	sat_carrier_channel_set.h \
	sat_carrier_udp_channel.h

libopensand_satcarrier_la_SOURCES = \
	$(libopensand_satcarrier_la_cpp) \
	$(libopensand_satcarrier_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb_rcs/utils


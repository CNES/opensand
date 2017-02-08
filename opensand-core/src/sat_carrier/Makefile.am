SUBDIRS = plugins . tests

noinst_LTLIBRARIES = libopensand_satcarrier.la

libopensand_satcarrier_la_cpp = \
	SatCarrierFifo.cpp \
	SatCarrierFifoElement.cpp \
	BlockSatCarrier.cpp \
	BlockSatCarrierSat.cpp \
	sat_carrier_channel_set.cpp \
	sat_carrier_udp_channel.cpp \
	SatDelayMap.cpp

libopensand_satcarrier_la_h = \
	SatCarrierFifo.h \
	SatCarrierFifoElement.h \
	BlockSatCarrier.h \
	BlockSatCarrierSat.h \
	sat_carrier_channel_set.h \
	sat_carrier_udp_channel.h \
	SatDelayMap.h

libopensand_satcarrier_la_SOURCES = \
	$(libopensand_satcarrier_la_cpp) \
	$(libopensand_satcarrier_la_h)
libopensand_satcarrier_la_LIBADD = \
	$(PACKED_COMMON_LIBS)       

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils

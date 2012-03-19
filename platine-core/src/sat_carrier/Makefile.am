noinst_LTLIBRARIES = libplatine_satcarrier.la

libplatine_satcarrier_la_cpp = \
	bloc_sat_carrier.cpp \
	sat_carrier_channel.cpp \
	sat_carrier_channel_set.cpp \
	sat_carrier_udp_channel.cpp

libplatine_satcarrier_la_h = \
	bloc_sat_carrier.h \
	sat_carrier_channel.h \
	sat_carrier_channel_set.h 
	sat_carrier_udp_channel.h

libplatine_satcarrier_la_SOURCES = \
	$(libplatine_satcarrier_la_cpp) \
	$(libplatine_satcarrier_la_h)

INCLUDES = \
	-I../common \
	-I../dvb_rcs/utils

#AM_CPPFLAGS = -g -Wall -Werror -DUTI_DEBUG_ON


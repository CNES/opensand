noinst_LTLIBRARIES = libplatine_dvb_switch.la

libplatine_dvb_switch_la_cpp = \
	AtmSwitch.cpp \
	MpegSwitch.cpp \
	GseSwitch.cpp \
	GenericSwitch.cpp
       
libplatine_dvb_switch_la_h = \
	AtmSwitch.h \
	MpegSwitch.h \
	GseSwitch.h \
	GenericSwitch.h

libplatine_dvb_switch_la_SOURCES = \
	$(libplatine_dvb_switch_la_cpp) \
	$(libplatine_dvb_switch_la_h)

INCLUDES = \
	-I. \
	-I$(top_srcdir)/src/common

AM_CPPFLAGS = -g -Wall -Werror -DUTI_DEBUG_ON


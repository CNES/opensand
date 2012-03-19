noinst_LTLIBRARIES = libplatine_dvb_core.la

libplatine_dvb_core_la_cpp = \
	DvbFrame.cpp \
	DvbRcsFrame.cpp \
	BBFrame.cpp \
	SatelliteTerminal.cpp \
	SatelliteTerminalList.cpp \
	SatSpot.cpp \
	PhysicStd.cpp \
	DvbRcsStd.cpp \
	DvbS2Std.cpp \
	bloc_dvb.cpp \
	bloc_dvb_rcs_sat.cpp \
	bloc_dvb_rcs_ncc.cpp \
	bloc_dvb_rcs_tal.cpp

libplatine_dvb_core_la_h = \
	DvbFrame.h \
	DvbRcsFrame.h \
	BBFrame.h \
	SatelliteTerminal.h \
	SatelliteTerminalList.h \
	SatSpot.h \
	PhysicStd.h \
	DvbRcsStd.h \
	DvbS2Std.h \
	bloc_dvb.h \
	bloc_dvb_rcs_sat.h \
	bloc_dvb_rcs_ncc.h \
	bloc_dvb_rcs_tal.h

libplatine_dvb_core_la_SOURCES = \
	$(libplatine_dvb_core_la_cpp) \
	$(libplatine_dvb_core_la_h)

AM_CPPFLAGS = -g -Wall -Werror -DUTI_DEBUG_ON

INCLUDES = \
	-I../utils \
	-I../dama \
	-I../modcod \
	-I../dra \
	-I../switch \
	-I../pep \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/common


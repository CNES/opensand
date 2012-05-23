noinst_LTLIBRARIES = libopensand_dvb_core.la

libopensand_dvb_core_la_cpp = \
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

libopensand_dvb_core_la_h = \
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

libopensand_dvb_core_la_SOURCES = \
	$(libopensand_dvb_core_la_cpp) \
	$(libopensand_dvb_core_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/dama \
	-I$(top_srcdir)/src/dvb_rcs/modcod \
	-I$(top_srcdir)/src/dvb_rcs/dra \
	-I$(top_srcdir)/src/dvb_rcs/switch \
	-I$(top_srcdir)/src/dvb_rcs/pep \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/common


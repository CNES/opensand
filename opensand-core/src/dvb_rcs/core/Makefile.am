noinst_LTLIBRARIES = libopensand_dvb_core.la

libopensand_dvb_core_la_cpp = \
	SatSpot.cpp \
	SatGw.cpp \
	PhysicStd.cpp \
	DvbRcsStd.cpp \
	DvbS2Std.cpp \
	BlockDvb.cpp \
	SpotDownward.cpp \
	SpotUpward.cpp \
	BlockDvbSat.cpp \
	BlockDvbNcc.cpp \
	BlockDvbTal.cpp

libopensand_dvb_core_la_h = \
	SatSpot.h \
	SatGw.h \
	PhysicStd.h \
	DvbRcsStd.h \
	DvbS2Std.h \
	BlockDvb.h \
	SpotDownward.h \
	SpotUpward.h \
	BlockDvbSat.h \
	BlockDvbNcc.h \
	BlockDvbTal.h

libopensand_dvb_core_la_SOURCES = \
	$(libopensand_dvb_core_la_cpp) \
	$(libopensand_dvb_core_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/utils/ip \
	-I$(top_srcdir)/src/dvb_rcs/dama \
	-I$(top_srcdir)/src/dvb_rcs/fmt \
	-I$(top_srcdir)/src/dvb_rcs/switch \
	-I$(top_srcdir)/src/dvb_rcs/pep \
	-I$(top_srcdir)/src/dvb_rcs/saloha \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/common


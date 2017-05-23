SUBDIRS = regenerative transparent

noinst_LTLIBRARIES = libopensand_dvb_core.la

libopensand_dvb_core_la_cpp = \
	SatGw.cpp \
	PhysicStd.cpp \
	DvbRcsStd.cpp \
	DvbS2Std.cpp \
	BlockDvb.cpp \
	DvbChannel.cpp \
	SpotDownward.cpp \
	SpotUpward.cpp \
	BlockDvbSat.cpp \
	BlockDvbNcc.cpp \
	BlockDvbTal.cpp \
	FileSimulator.cpp \
	RandomSimulator.cpp \
	RequestSimulator.cpp

libopensand_dvb_core_la_h = \
	SatGw.h \
	PhysicStd.h \
	DvbRcsStd.h \
	DvbS2Std.h \
	BlockDvb.h \
	DvbChannel.h \
	SpotDownward.h \
	SpotUpward.h \
	BlockDvbSat.h \
	BlockDvbNcc.h \
	BlockDvbTal.h \
	FileSimulator.h \
	RandomSimulator.h \
	RequestSimulator.h

libopensand_dvb_core_la_SOURCES = \
	$(libopensand_dvb_core_la_cpp) \
	$(libopensand_dvb_core_la_h)

libopensand_dvb_core_la_LIBADD = \
	regenerative/libopensand_dvb_core_regen.la \
	transparent/libopensand_dvb_core_transp.la

libopensand_dvb_core_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/dvb/simu \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/dvb/utils/ip \
	-I$(top_srcdir)/src/dvb/dama \
	-I$(top_srcdir)/src/dvb/fmt \
	-I$(top_srcdir)/src/dvb/switch \
	-I$(top_srcdir)/src/dvb/ncc_interface \
	-I$(top_srcdir)/src/dvb/saloha \
	-I$(top_srcdir)/src/dvb/scpc \
	-I$(top_srcdir)/src/dvb/core/regenerative \
	-I$(top_srcdir)/src/dvb/core/transparent \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common


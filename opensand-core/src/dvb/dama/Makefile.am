lib_LTLIBRARIES = libopensand_dama.la

libopensand_dama_la_cpp = \
	CircularBuffer.cpp \
	UplinkSchedulingRcsCommon.cpp \
	UplinkSchedulingRcs.cpp \
	UplinkSchedulingRcs2.cpp \
	ReturnSchedulingRcsCommon.cpp \
	ReturnSchedulingRcs.cpp \
	ReturnSchedulingRcs2.cpp \
	ForwardSchedulingS2.cpp \
	ScpcScheduling.cpp \
	DamaAgent.cpp \
	DamaAgentRcsCommon.cpp \
	DamaAgentRcs.cpp \
	DamaAgentRcsLegacy.cpp \
	DamaAgentRcsRrmQos.cpp \
	DamaAgentRcs2.cpp \
	DamaAgentRcs2Legacy.cpp \
	DamaCtrl.cpp \
	DamaCtrlRcsCommon.cpp \
	DamaCtrlRcs.cpp \
	DamaCtrlRcsLegacy.cpp \
	DamaCtrlRcs2.cpp \
	DamaCtrlRcs2Legacy.cpp

libopensand_dama_la_h = \
	CircularBuffer.h \
	Scheduling.h \
	UplinkSchedulingRcsCommon.h \
	UplinkSchedulingRcs.h \
	UplinkSchedulingRcs2.h \
	ReturnSchedulingRcsCommon.h \
	ReturnSchedulingRcs.h \
	ReturnSchedulingRcs2.h \
	ForwardSchedulingS2.h \
	ScpcScheduling.h \
	DamaAgent.h \
	DamaAgentRcsCommon.h \
	DamaAgentRcs.h \
	DamaAgentRcsLegacy.h \
	DamaAgentRcsRrmQos.h \
	DamaAgentRcs2.h \
	DamaAgentRcs2Legacy.h \
	DamaCtrl.h \
	DamaCtrlRcsCommon.h \
	DamaCtrlRcs.h \
	DamaCtrlRcsLegacy.h \
	DamaCtrlRcs2.h \
	DamaCtrlRcs2Legacy.h

libopensand_dama_la_SOURCES = \
	$(libopensand_dama_la_cpp) \
	$(libopensand_dama_la_h)

libopensand_dama_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/dvb/fmt \
	-I$(top_srcdir)/src/dvb/ncc_interface \
	-I$(top_srcdir)/src/dvb/core \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

libopensand_dama_la_LIBADD = \
	$(top_builddir)/src/dvb/fmt/libopensand_dvb_fmt.la

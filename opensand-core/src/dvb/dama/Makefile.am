lib_LTLIBRARIES = libopensand_dama.la

libopensand_dama_la_cpp = \
	CircularBuffer.cpp \
	Scheduling.cpp \
	ReturnSchedulingRcs2.cpp \
	ForwardSchedulingS2.cpp \
	ScpcScheduling.cpp \
	DamaAgent.cpp \
	DamaAgentRcs2.cpp \
	DamaAgentRcs2Legacy.cpp \
	DamaCtrl.cpp \
	DamaCtrlRcs2.cpp \
	DamaCtrlRcs2Legacy.cpp

libopensand_dama_la_h = \
	CircularBuffer.h \
	Scheduling.h \
	ReturnSchedulingRcs2.h \
	ForwardSchedulingS2.h \
	ScpcScheduling.h \
	DamaAgent.h \
	DamaAgentRcs2.h \
	DamaAgentRcs2Legacy.h \
	DamaCtrl.h \
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

lib_LTLIBRARIES = libopensand_dama.la

libopensand_dama_la_cpp = \
	CircularBuffer.cpp \
	UplinkSchedulingRcs.cpp \
	ReturnSchedulingRcs.cpp \
	ForwardSchedulingS2.cpp \
	DamaAgent.cpp \
	DamaAgentRcs.cpp \
	DamaAgentRcsLegacy.cpp \
	DamaCtrl.cpp \
	DamaCtrlRcs.cpp \
	DamaCtrlRcsLegacy.cpp
#	DamaCtrlRcsYes.cpp

libopensand_dama_la_h = \
	CircularBuffer.h \
	Scheduling.h \
	UplinkSchedulingRcs.h \
	ReturnSchedulingRcs.h \
	ForwardSchedulingS2.h \
	DamaAgent.h \
	DamaAgentRcs.h \
	DamaAgentRcsLegacy.h \
	DamaCtrl.h \
	DamaCtrlRcs.h \
	DamaCtrlRcsLegacy.h
#	DamaCtrlRcsYes.h

libopensand_dama_la_SOURCES = \
	$(libopensand_dama_la_cpp) \
	$(libopensand_dama_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/fmt \
	-I$(top_srcdir)/src/dvb_rcs/pep \
	-I$(top_srcdir)/src/dvb_rcs/core \
	-I$(top_srcdir)/src/common

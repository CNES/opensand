noinst_LTLIBRARIES = libopensand_dvb_utils.la

libopensand_dvb_utils_la_cpp = \
	UnitConverter.cpp \
	Sac.cpp \
	Ttp.cpp \
	Logon.cpp \
	Logoff.cpp \
	Sof.cpp \
	DvbFifo.cpp \
	MacFifoElement.cpp \
	sat_emulator_err.cpp \
	TerminalContext.cpp \
	TerminalContextRcs.cpp \
	FmtGroup.cpp \
	CarriersGroup.cpp \
	TerminalCategory.cpp \
	DvbFrame.cpp \
	DvbRcsFrame.cpp \
	BBFrame.cpp


libopensand_dvb_utils_la_h = \
	UnitConverter.h \
	Sac.h \
	Ttp.h \
	Logon.h \
	Logoff.h \
	Sof.h \
	DvbFifo.h \
	MacFifoElement.h \
	sat_emulator_err.h \
	TerminalContext.h \
	TerminalContextRcs.h \
	TerminalCategory.h \
	FmtGroup.h \
	CarriersGroup.h \
	OpenSandFrames.h \
	DvbFrame.h \
	DvbRcsFrame.h \
	BBFrame.h


libopensand_dvb_utils_la_SOURCES = \
	$(libopensand_dvb_utils_la_cpp) \
	$(libopensand_dvb_utils_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/dvb_rcs/fmt

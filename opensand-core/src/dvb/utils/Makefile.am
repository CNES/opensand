noinst_LTLIBRARIES = libopensand_dvb_utils.la

libopensand_dvb_utils_la_cpp = \
	UnitConverter.cpp \
	UnitConverterFixedBitLength.cpp \
	UnitConverterFixedSymbolLength.cpp \
	Sac.cpp \
	Ttp.cpp \
	Logon.cpp \
	Logoff.cpp \
	Sof.cpp \
	DvbFifo.cpp \
	MacFifoElement.cpp \
	TerminalContext.cpp \
	TerminalContextDama.cpp \
	TerminalContextDamaRcs.cpp \
	TerminalContextSaloha.cpp \
	FmtGroup.cpp \
	CarriersGroup.cpp \
	CarriersGroupDama.cpp \
	CarriersGroupSaloha.cpp \
	TerminalCategoryDama.cpp \
	TerminalCategorySaloha.cpp \
	DvbRcsFrame.cpp \
	BBFrame.cpp \
	Slot.cpp \
	SlottedAlohaPacketData.cpp \
	SlottedAlohaPacketCtrl.cpp \
	SlottedAlohaFrame.cpp

libopensand_dvb_utils_la_h = \
	UnitConverter.h \
	UnitConverterFixedBitLength.h \
	UnitConverterFixedSymbolLength.h \
	Sac.h \
	Ttp.h \
	Logon.h \
	Logoff.h \
	Sof.h \
	DvbFifo.h \
	MacFifoElement.h \
	TerminalContext.h \
	TerminalContextDama.h \
	TerminalContextDamaRcs.h \
	TerminalContextSaloha.h \
	TerminalCategory.h \
	TerminalCategoryDama.h \
	TerminalCategorySaloha.h \
	FmtGroup.h \
	CarriersGroup.h \
	CarriersGroupDama.h \
	CarriersGroupSaloha.h \
	OpenSandFrames.h \
	DvbFrame.h \
	DvbRcsFrame.h \
	BBFrame.h \
	Slot.h \
	SlottedAlohaPacket.h \
	SlottedAlohaPacketData.h \
	SlottedAlohaPacketCtrl.h \
	SlottedAlohaFrame.h


libopensand_dvb_utils_la_SOURCES = \
	$(libopensand_dvb_utils_la_cpp) \
	$(libopensand_dvb_utils_la_h)

libopensand_dvb_utils_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/dvb/fmt

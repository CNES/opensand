noinst_LTLIBRARIES = libopensand_dvb_fmt.la

libopensand_dvb_fmt_la_cpp = \
	ModulationTypes.cpp \
	CodingTypes.cpp \
	FmtDefinition.cpp \
	FmtDefinitionTable.cpp \
	StFmtSimu.cpp

libopensand_dvb_fmt_la_h = \
	ModulationTypes.h \
	CodingTypes.h \
	FmtDefinition.h \
	FmtDefinitionTable.h \
	StFmtSimu.h

libopensand_dvb_fmt_la_SOURCES = \
	$(libopensand_dvb_fmt_la_cpp) \
	$(libopensand_dvb_fmt_la_h)

libopensand_dvb_fmt_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common/

noinst_LTLIBRARIES = \
	libopensand_lan_adaptation.la

libopensand_lan_adaptation_la_cpp = \
	BlockLanAdaptation.cpp \
	BlockLanAdaptationInit.cpp \
	Evc.cpp \
	Ethernet.cpp

libopensand_lan_adaptation_la_h = \
	BlockLanAdaptation.h \
	EthernetHeader.h \
	Evc.h \
	Ethernet.h

libopensand_lan_adaptation_la_SOURCES = \
	$(libopensand_lan_adaptation_la_cpp) \
	$(libopensand_lan_adaptation_la_h)

libopensand_lan_adaptation_la_LIBADD = \
	$(top_builddir)/src/conf/libopensand_conf_core.la

libopensand_lan_adaptation_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils

libopensand_lan_adaptation_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-lm


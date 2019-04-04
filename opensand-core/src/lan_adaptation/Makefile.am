SUBDIRS = tests

noinst_LTLIBRARIES = \
	libopensand_lan_adaptation.la \
	libopensand_bridge.la

libopensand_bridge_la_SOURCES = \
	bridge_utils.h \
	bridge_utils.c

libopensand_lan_adaptation_la_cpp = \
	BlockLanAdaptation.cpp \
	BlockLanAdaptationInit.cpp \
	Ipv4Address.cpp \
	Ipv6Address.cpp

libopensand_lan_adaptation_la_h = \
	BlockLanAdaptation.h \
	Ipv4Address.h \
	Ipv6Address.h

libopensand_lan_adaptation_la_SOURCES = \
	$(libopensand_lan_adaptation_la_cpp) \
	$(libopensand_lan_adaptation_la_h)

#libopensand_lan_adaptation_la_LIBADD = \
#	libopensand_bridge.la

libopensand_lan_adaptation_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils

libopensand_lan_adaptation_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-lm


noinst_LTLIBRARIES = libopensand_dvb_saloha.la

libopensand_dvb_saloha_la_cpp = \
	SlottedAlohaBackoff.cpp \
	SlottedAlohaBackoffBeb.cpp \
	SlottedAlohaBackoffEied.cpp \
	SlottedAlohaBackoffMimd.cpp \
	SlottedAlohaAlgo.cpp \
	SlottedAlohaAlgoDsa.cpp \
	SlottedAlohaAlgoCrdsa.cpp \
	SlottedAloha.cpp \
	SlottedAlohaTal.cpp \
	SlottedAlohaNcc.cpp

libopensand_dvb_saloha_la_h = \
	SlottedAlohaBackoff.h \
	SlottedAlohaBackoffBeb.h \
	SlottedAlohaBackoffEied.h \
	SlottedAlohaBackoffMimd.h \
	SlottedAlohaAlgo.h \
	SlottedAlohaAlgoDsa.h \
	SlottedAlohaAlgoCrdsa.h \
	SlottedAloha.h \
	SlottedAlohaTal.h \
	SlottedAlohaNcc.h

libopensand_dvb_saloha_la_SOURCES = \
	$(libopensand_dvb_saloha_la_cpp) \
	$(libopensand_dvb_saloha_la_h)

libopensand_dvb_saloha_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/dvb/fmt \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/physical_layer

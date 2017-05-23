noinst_LTLIBRARIES = libopensand_dvb_core_regen.la

libopensand_dvb_core_regen_la_cpp = \
	SpotDownwardRegen.cpp \
	SpotUpwardRegen.cpp \
	BlockDvbSatRegen.cpp

libopensand_dvb_core_regen_la_h = \
	SpotDownwardRegen.h \
	SpotUpwardRegen.h \
	BlockDvbSatRegen.h

libopensand_dvb_core_regen_la_SOURCES = \
	$(libopensand_dvb_core_regen_la_cpp) \
	$(libopensand_dvb_core_regen_la_h) 

libopensand_dvb_core_regen_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/dvb/utils/ip \
	-I$(top_srcdir)/src/dvb/dama \
	-I$(top_srcdir)/src/dvb/fmt \
	-I$(top_srcdir)/src/dvb/switch \
	-I$(top_srcdir)/src/dvb/ncc_interface \
	-I$(top_srcdir)/src/dvb/saloha \
	-I$(top_srcdir)/src/dvb/scpc \
	-I$(top_srcdir)/src/dvb/core \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common


noinst_LTLIBRARIES = libopensand_dvb_core_transp.la

libopensand_dvb_core_transp_la_cpp = \
	SpotDownwardTransp.cpp \
	SpotUpwardTransp.cpp \
	BlockDvbSatTransp.cpp

libopensand_dvb_core_transp_la_h = \
	SpotDownwardTransp.h \
	SpotUpwardTransp.h \
	BlockDvbSatTransp.h

libopensand_dvb_core_transp_la_SOURCES = \
	$(libopensand_dvb_core_transp_la_cpp) \
	$(libopensand_dvb_core_transp_la_h)
	$(top_srcdir)/src/dvb/core/SpotDownward.cpp\
	$(top_srcdir)/src/dvb/core/SpotUpward.cpp

libopensand_dvb_core_transp_la_CPPFLAGS = \
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


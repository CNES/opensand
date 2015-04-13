noinst_LTLIBRARIES = libopensand_dvb_scpc.la

libopensand_dvb_scpc_la_cpp = \
	ScpcScheduling.cpp \
	DvbScpcStd.cpp

libopensand_dvb_scpc_la_h = \
	ScpcScheduling.h \
	DvbScpcStd.h

libopensand_dvb_scpc_la_SOURCES = \
	$(libopensand_dvb_scpc_la_cpp) \
	$(libopensand_dvb_scpc_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/fmt \
	-I$(top_srcdir)/src/dvb_rcs/dama \
	-I$(top_srcdir)/src/dvb_rcs/core \
	-I$(top_srcdir)/src/common

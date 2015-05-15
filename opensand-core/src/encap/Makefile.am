noinst_LTLIBRARIES = libopensand_encap.la

libopensand_encap_la_cpp = \
	BlockEncap.cpp \
	BlockEncapSat.cpp


libopensand_encap_la_h = \
	BlockEncap.h \
	BlockEncapSat.h


libopensand_encap_la_SOURCES = \
	$(libopensand_encap_la_cpp) \
	$(libopensand_encap_la_h)

libopensand_encap_la_DEPENDENCIES = \
	$(libopensand_encap_la_cpp) \
	$(libopensand_encap_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb_rcs/core \
	-I$(top_srcdir)/src/dvb_rcs/ncc_interface \
	-I$(top_srcdir)/src/dvb_rcs/fmt \
	-I$(top_srcdir)/src/dvb_rcs/utils

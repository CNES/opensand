noinst_LTLIBRARIES = libopensand_encap.la

libopensand_encap_la_cpp = \
	bloc_encap.cpp \
	bloc_encap_sat.cpp


libopensand_encap_la_h = \
	bloc_encap.h \
	bloc_encap_sat.h


libopensand_encap_la_SOURCES = \
	$(libopensand_encap_la_cpp) \
	$(libopensand_encap_la_h)

libopensand_encap_la_DEPENDENCIES = \
	$(libopensand_encap_la_cpp) \
	$(libopensand_encap_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/dvb_rcs/utils

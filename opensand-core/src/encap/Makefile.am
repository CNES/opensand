noinst_LTLIBRARIES = libopensand_encap.la

libopensand_encap_la_cpp = \
	BlockEncap.cpp

libopensand_encap_la_h = \
	BlockEncap.h

libopensand_encap_la_SOURCES = \
	$(libopensand_encap_la_cpp) \
	$(libopensand_encap_la_h)

libopensand_encap_la_DEPENDENCIES = \
	$(libopensand_encap_la_cpp) \
	$(libopensand_encap_la_h)

libopensand_encap_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/lan_adaptation \
	-I$(top_srcdir)/src/dvb/utils

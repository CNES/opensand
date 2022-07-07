noinst_LTLIBRARIES = libopensand_conf_core.la

libopensand_conf_core_la_cpp = \
	OpenSandModelConf.cpp

libopensand_conf_core_la_h = \
	OpenSandModelConf.h

libopensand_conf_core_la_SOURCES = \
	$(libopensand_conf_core_la_cpp) \
	$(libopensand_conf_core_la_h)

libopensand_conf_core_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/dvb/fmt

libopensand_conf_core_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-rdynamic

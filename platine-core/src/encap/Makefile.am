noinst_LTLIBRARIES = libplatine_encap.la

libplatine_encap_la_cpp = \
	bloc_encap.cpp \
	bloc_encap_sat.cpp


libplatine_encap_la_h = \
	bloc_encap.h \
	bloc_encap_sat.h


libplatine_encap_la_SOURCES = \
	$(libplatine_encap_la_cpp) \
	$(libplatine_encap_la_h)

libplatine_encap_la_DEPENDENCIES = \
	$(libplatine_encap_la_cpp) \
	$(libplatine_encap_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/common

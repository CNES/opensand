SUBDIRS = .

noinst_LTLIBRARIES = libopensand_interconnect.la

libopensand_interconnect_la_cpp = \
	interconnect_channel.cpp

libopensand_interconnect_la_h = \
	BlockInterconnectUpward.h \
	BlockInterconnectDownward.h \
	interconnect_channel.h

libopensand_interconnect_la_SOURCES = \
	$(libopensand_interconnect_la_cpp) \
	$(libopensand_interconnect_la_h)

libopensand_interconnect_la_DEPENDENCIES = \
	$(libopensand_interconnect_la_cpp) \
	$(libopensand_interconnect_la_h)

libopensand_interconnect_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils

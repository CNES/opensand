################################################################################
#   Name       : Makefile
#   Author     : Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.com>
#   Description: create the constant satellite delay plugin for OpenSAND
################################################################################

SUBDIRS =

plugins_LTLIBRARIES = libopensand_constant_isldelay_plugin.la

libopensand_constant_isldelay_plugin_la_cpp = \
	ConstantIslDelay.cpp

libopensand_constant_isldelay_plugin_la_h = \
	ConstantIslDelay.h

libopensand_constant_isldelay_plugin_la_SOURCES = \
	$(libopensand_constant_isldelay_plugin_la_cpp) \
	$(libopensand_constant_isldelay_plugin_la_h)

libopensand_constant_isldelay_plugin_la_LIBADD = \
	$(top_builddir)/src/common/libopensand_plugin.la

pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

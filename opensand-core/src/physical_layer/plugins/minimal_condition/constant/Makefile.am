################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the constant minimal condition plugin for OpenSAND
################################################################################

SUBDIRS =

plugins_LTLIBRARIES = libopensand_constant_minimal_plugin.la

libopensand_constant_minimal_plugin_la_cpp = \
	Constant.cpp

libopensand_constant_minimal_plugin_la_h = \
	Constant.h

libopensand_constant_minimal_plugin_la_SOURCES = \
	$(libopensand_constant_minimal_plugin_la_cpp) \
	$(libopensand_constant_minimal_plugin_la_h)

libopensand_constant_minimal_plugin_la_LIBADD = \
	$(top_builddir)/src/conf/libopensand_conf_core.la

pluginsdir = $(libdir)/opensand/plugins

libopensand_constant_minimal_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

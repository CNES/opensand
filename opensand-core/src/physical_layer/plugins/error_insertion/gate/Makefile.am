################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the Gate error insertion plugin for OpenSAND
################################################################################

SUBDIRS =

plugins_LTLIBRARIES = libopensand_gate_error_plugin.la

libopensand_gate_error_plugin_la_cpp = \
	Gate.cpp

libopensand_gate_error_plugin_la_h = \
	Gate.h

libopensand_gate_error_plugin_la_SOURCES = \
	$(libopensand_gate_error_plugin_la_cpp) \
	$(libopensand_gate_error_plugin_la_h)

libopensand_gate_error_plugin_la_LIBADD = \
	$(top_builddir)/src/common/libopensand_plugin.la

pluginsdir = $(libdir)/opensand/plugins

libopensand_gate_error_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

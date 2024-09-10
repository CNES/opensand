################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the file attenuation plugin for OpenSAND
################################################################################

SUBDIRS =

plugins_LTLIBRARIES = libopensand_file_attenuation_plugin.la

libopensand_file_attenuation_plugin_la_cpp = \
	File.cpp

libopensand_file_attenuation_plugin_la_h = \
	File.h

libopensand_file_attenuation_plugin_la_SOURCES = \
	$(libopensand_file_attenuation_plugin_la_cpp) \
	$(libopensand_file_attenuation_plugin_la_h)

libopensand_file_attenuation_plugin_la_LIBADD = \
	$(top_builddir)/src/common/libopensand_plugin.la

pluginsdir = $(libdir)/opensand/plugins

libopensand_file_attenuation_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

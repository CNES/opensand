################################################################################
#   Name       : Makefile
#   Author     : Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.com>
#   Description: create the file sat delay plugin for OpenSAND
################################################################################

SUBDIRS =

plugins_LTLIBRARIES = libopensand_file_isldelay_plugin.la

libopensand_file_isldelay_plugin_la_cpp = \
	FileIslDelay.cpp

libopensand_file_isldelay_plugin_la_h = \
	FileIslDelay.h

libopensand_file_isldelay_plugin_la_SOURCES = \
	$(libopensand_file_isldelay_plugin_la_cpp) \
	$(libopensand_file_isldelay_plugin_la_h)

libopensand_file_isldelay_plugin_la_LIBADD = \
	$(top_builddir)/src/common/libopensand_plugin.la

pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

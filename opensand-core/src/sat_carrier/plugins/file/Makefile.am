################################################################################
#   Name       : Makefile
#   Author     : Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.com>
#   Description: create the file sat delay plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_file_satdelay_plugin.la

libopensand_file_satdelay_plugin_la_cpp = \
	FileDelay.cpp

libopensand_file_satdelay_plugin_la_h = \
	FileDelay.h

libopensand_file_satdelay_plugin_la_SOURCES = \
    $(libopensand_file_satdelay_plugin_la_cpp) \
    $(libopensand_file_satdelay_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/common

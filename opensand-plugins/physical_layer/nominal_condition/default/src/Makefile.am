################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the default nominal plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_default_nominal_plugin.la

EXTRA_DIST = \
	default.py

libopensand_default_nominal_plugin_la_cpp = \
	Default.cpp

libopensand_default_nominal_plugin_la_h = \
	Default.h

libopensand_default_nominal_plugin_la_SOURCES = \
    $(libopensand_default_nominal_plugin_la_cpp) \
    $(libopensand_default_nominal_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

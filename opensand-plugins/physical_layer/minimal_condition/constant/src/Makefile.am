################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the constant minimal condition plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_constant_minimal_plugin.la

EXTRA_DIST = \
	constant.py

libopensand_constant_minimal_plugin_la_cpp = \
	Constant.cpp

libopensand_constant_minimal_plugin_la_h = \
	Constant.h

libopensand_constant_minimal_plugin_la_SOURCES = \
    $(libopensand_constant_minimal_plugin_la_cpp) \
    $(libopensand_constant_minimal_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

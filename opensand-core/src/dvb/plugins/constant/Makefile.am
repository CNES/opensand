################################################################################
#   Name       : Makefile
#   Author     : Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.com>
#   Description: create the constant satellite delay plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_constant_satdelay_plugin.la

libopensand_constant_satdelay_plugin_la_cpp = \
	Constant.cpp

libopensand_constant_satdelay_plugin_la_h = \
	Constant.h

libopensand_constant_satdelay_plugin_la_SOURCES = \
    $(libopensand_constant_satdelay_plugin_la_cpp) \
    $(libopensand_constant_satdelay_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/common

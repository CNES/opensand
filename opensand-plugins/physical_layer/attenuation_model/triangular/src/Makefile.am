################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the triangular attenuation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_triangular_attenuation_plugin.la

EXTRA_DIST = \
	triangular.py

libopensand_triangular_attenuation_plugin_la_cpp = \
	Triangular.cpp

libopensand_triangular_attenuation_plugin_la_h = \
	Triangular.h

libopensand_triangular_attenuation_plugin_la_SOURCES = \
    $(libopensand_triangular_attenuation_plugin_la_cpp) \
    $(libopensand_triangular_attenuation_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

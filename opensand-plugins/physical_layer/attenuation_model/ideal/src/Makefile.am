################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ideal attenuation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_ideal_attenuation_plugin.la

EXTRA_DIST = \
	ideal.py

libopensand_ideal_attenuation_plugin_la_cpp = \
	Ideal.cpp

libopensand_ideal_attenuation_plugin_la_h = \
	Ideal.h

libopensand_ideal_attenuation_plugin_la_SOURCES = \
    $(libopensand_ideal_attenuation_plugin_la_cpp) \
    $(libopensand_ideal_attenuation_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

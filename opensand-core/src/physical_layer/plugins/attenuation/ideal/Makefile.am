################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ideal attenuation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_ideal_attenuation_plugin.la

libopensand_ideal_attenuation_plugin_la_cpp = \
	Ideal.cpp

libopensand_ideal_attenuation_plugin_la_h = \
	Ideal.h

libopensand_ideal_attenuation_plugin_la_SOURCES = \
    $(libopensand_ideal_attenuation_plugin_la_cpp) \
    $(libopensand_ideal_attenuation_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

libopensand_ideal_attenuation_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common

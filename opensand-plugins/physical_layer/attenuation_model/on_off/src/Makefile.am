################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the on_off attenuation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_on_off_attenuation_plugin.la

EXTRA_DIST = \
	on_off.py

libopensand_on_off_attenuation_plugin_la_cpp = \
	OnOff.cpp

libopensand_on_off_attenuation_plugin_la_h = \
	OnOff.h

libopensand_on_off_attenuation_plugin_la_SOURCES = \
    $(libopensand_on_off_attenuation_plugin_la_cpp) \
    $(libopensand_on_off_attenuation_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the on_off attenuation plugin for OpenSAND
################################################################################

SUBDIRS =

plugins_LTLIBRARIES = libopensand_on_off_attenuation_plugin.la

libopensand_on_off_attenuation_plugin_la_cpp = \
	OnOff.cpp

libopensand_on_off_attenuation_plugin_la_h = \
	OnOff.h

libopensand_on_off_attenuation_plugin_la_SOURCES = \
	$(libopensand_on_off_attenuation_plugin_la_cpp) \
	$(libopensand_on_off_attenuation_plugin_la_h)

libopensand_on_off_attenuation_plugin_la_LIBADD = \
	$(top_builddir)/src/common/libopensand_plugin.la

pluginsdir = $(libdir)/opensand/plugins

libopensand_on_off_attenuation_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

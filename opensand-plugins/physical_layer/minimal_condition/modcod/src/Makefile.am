################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the MODCOD minimal conditionplugin for OpenSAND
################################################################################

SUBDIRS = modcod .

plugins_LTLIBRARIES = libopensand_modcod_minimal_plugin.la

EXTRA_DIST = \
	modcod.py

libopensand_modcod_minimal_plugin_la_cpp = \
    Modcod.cpp

libopensand_modcod_minimal_plugin_la_h = \
    Modcod.h

libopensand_modcod_minimal_plugin_la_SOURCES = \
    $(libopensand_modcod_minimal_plugin_la_cpp) \
    $(libopensand_modcod_minimal_plugin_la_h)
    
libopensand_modcod_minimal_plugin_la_LIBADD= \
    $(top_builddir)/src/modcod/libopensand_modcod.la

INCLUDES = \
    -I$(top_srcdir)/src/modcod 

pluginsdir = $(prefix)/lib/opensand/plugins

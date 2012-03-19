################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the GSE plugin for Platine
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libplatine_gse_plugin.la

libplatine_gse_plugin_la_cpp = \
	GseIdentifier.cpp \
	GseEncapCtx.cpp \
	Gse.cpp

libplatine_gse_plugin_la_h = \
	GseIdentifier.h \
	GseEncapCtx.h \
	Gse.h

libplatine_gse_plugin_la_SOURCES = \
    $(libplatine_gse_plugin_la_cpp) \
    $(libplatine_gse_plugin_la_h)

pluginsdir = $(prefix)/lib/platine/plugins

################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the GSE plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_gse_plugin.la

libopensand_gse_plugin_la_cpp = \
	GseIdentifier.cpp \
	GseEncapCtx.cpp \
	Gse.cpp

libopensand_gse_plugin_la_h = \
	GseIdentifier.h \
	GseEncapCtx.h \
	Gse.h

libopensand_gse_plugin_la_SOURCES = \
    $(libopensand_gse_plugin_la_cpp) \
    $(libopensand_gse_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

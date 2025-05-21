################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the GSE encapsulation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_gse_encap_plugin.la

libopensand_gse_encap_plugin_la_cpp = \
	GseIdentifier.cpp \
	GseEncapCtx.cpp \
	Gse.cpp

libopensand_gse_encap_plugin_la_h = \
	GseIdentifier.h \
	GseEncapCtx.h \
	Gse.h

libopensand_gse_encap_plugin_la_SOURCES = \
    $(libopensand_gse_encap_plugin_la_cpp) \
    $(libopensand_gse_encap_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

EXTRA_DIST = \
	gse.py


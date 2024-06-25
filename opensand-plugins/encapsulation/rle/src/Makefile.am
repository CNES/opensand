###############################################################################
#   Name       : Makefile
#   Author     : Aurélien DELRIEU <adelrieu@toulouse.viveris.com>
#   Description: create the RLE encapsulation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_rle_encap_plugin.la

libopensand_rle_encap_plugin_la_cpp = \
	RleIdentifier.cpp \
	Rle.cpp 

libopensand_rle_encap_plugin_la_h = \
	RleIdentifier.h \
	Rle.h

libopensand_rle_encap_plugin_la_SOURCES = \
    $(libopensand_rle_encap_plugin_la_cpp) \
    $(libopensand_rle_encap_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

EXTRA_DIST = \
	rle.py


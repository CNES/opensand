################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ROHC plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_rohc_plugin.la

libopensand_rohc_plugin_la_cpp = \
	RohcPacket.cpp \
	Rohc.cpp

libopensand_rohc_plugin_la_h = \
	RohcPacket.h \
	Rohc.h

libopensand_rohc_plugin_la_SOURCES = \
    $(libopensand_rohc_plugin_la_cpp) \
    $(libopensand_rohc_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ROHC plugin for Platine
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libplatine_rohc_plugin.la

libplatine_rohc_plugin_la_cpp = \
	RohcPacket.cpp \
	Rohc.cpp

libplatine_rohc_plugin_la_h = \
	RohcPacket.h \
	Rohc.h

libplatine_rohc_plugin_la_SOURCES = \
    $(libplatine_rohc_plugin_la_cpp) \
    $(libplatine_rohc_plugin_la_h)

pluginsdir = $(prefix)/lib/platine/plugins

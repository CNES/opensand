################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ULE encapsulation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_ule_encap_plugin.la

libopensand_ule_encap_plugin_la_cpp = \
	UlePacket.cpp \
	UleExt.cpp \
	UleExtPadding.cpp \
	UleExtTest.cpp \
	Ule.cpp

#	UleExtSecurity.cpp 

libopensand_ule_encap_plugin_la_h = \
	UlePacket.h \
	UleExt.h \
	UleExtPadding.h \
	UleExtTest.h \
	Ule.h

#	UleExtSecurity.h

libopensand_ule_encap_plugin_la_SOURCES = \
    $(libopensand_ule_encap_plugin_la_cpp) \
    $(libopensand_ule_encap_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/common

EXTRA_DIST = \
	ule.py


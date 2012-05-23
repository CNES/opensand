################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ULE plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_ule_plugin.la

libopensand_ule_plugin_la_cpp = \
	UlePacket.cpp \
	UleExt.cpp \
	UleExtPadding.cpp \
	UleExtTest.cpp \
	Ule.cpp

#	UleExtSecurity.cpp 

libopensand_ule_plugin_la_h = \
	UlePacket.h \
	UleExt.h \
	UleExtPadding.h \
	UleExtTest.h \
	Ule.h

#	UleExtSecurity.h

libopensand_ule_plugin_la_SOURCES = \
    $(libopensand_ule_plugin_la_cpp) \
    $(libopensand_ule_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

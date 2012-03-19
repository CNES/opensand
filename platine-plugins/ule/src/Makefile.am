################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ULE plugin for Platine
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libplatine_ule_plugin.la

libplatine_ule_plugin_la_cpp = \
	UlePacket.cpp \
	UleExt.cpp \
	UleExtPadding.cpp \
	UleExtTest.cpp \
	Ule.cpp

#	UleExtSecurity.cpp 

libplatine_ule_plugin_la_h = \
	UlePacket.h \
	UleExt.h \
	UleExtPadding.h \
	UleExtTest.h \
	Ule.h

#	UleExtSecurity.h

libplatine_ule_plugin_la_SOURCES = \
    $(libplatine_ule_plugin_la_cpp) \
    $(libplatine_ule_plugin_la_h)

pluginsdir = $(prefix)/lib/platine/plugins

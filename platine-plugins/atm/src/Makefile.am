################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ATM/AAL5 plugin for Platine
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libplatine_atm_aal5_plugin.la

libplatine_atm_aal5_plugin_la_cpp = \
	AtmIdentifier.cpp \
	AtmCell.cpp \
	Aal5Packet.cpp \
	Atm.cpp

libplatine_atm_aal5_plugin_la_h = \
	AtmIdentifier.h \
	AtmCell.h \
	Aal5Packet.h \
	Atm.h

libplatine_atm_aal5_plugin_la_SOURCES = \
    $(libplatine_atm_aal5_plugin_la_cpp) \
    $(libplatine_atm_aal5_plugin_la_h)

pluginsdir = $(prefix)/lib/platine/plugins

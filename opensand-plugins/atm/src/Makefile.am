################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ATM/AAL5 plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_atm_aal5_plugin.la

libopensand_atm_aal5_plugin_la_cpp = \
	AtmIdentifier.cpp \
	AtmCell.cpp \
	Aal5Packet.cpp \
	Atm.cpp

libopensand_atm_aal5_plugin_la_h = \
	AtmIdentifier.h \
	AtmCell.h \
	Aal5Packet.h \
	Atm.h

libopensand_atm_aal5_plugin_la_SOURCES = \
    $(libopensand_atm_aal5_plugin_la_cpp) \
    $(libopensand_atm_aal5_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

EXTRA_DIST = \
	atm_aal5.py


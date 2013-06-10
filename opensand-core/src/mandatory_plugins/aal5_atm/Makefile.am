################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the AAL5/ATM encapsulation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_aal5_atm_encap_plugin.la

libopensand_aal5_atm_encap_plugin_la_cpp = \
	AtmIdentifier.cpp \
	AtmCell.cpp \
	Aal5Packet.cpp \
	Atm.cpp

libopensand_aal5_atm_encap_plugin_la_h = \
	AtmIdentifier.h \
	AtmCell.h \
	Aal5Packet.h \
	Atm.h

libopensand_aal5_atm_encap_plugin_la_SOURCES = \
    $(libopensand_aal5_atm_encap_plugin_la_cpp) \
    $(libopensand_aal5_atm_encap_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/common

EXTRA_DIST = \
	aal5_atm.py


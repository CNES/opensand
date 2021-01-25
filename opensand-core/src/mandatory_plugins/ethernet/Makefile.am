################################################################################
#   Name       : Makefile
#   Author     : Remy Pienne <rpienne@toulouse.viveris.com>
#   Description: create the Ethernet adaptation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_eth_lan_adapt_plugin.la

libopensand_eth_lan_adapt_plugin_la_cpp = \
	Evc.cpp \
	Ethernet.cpp

libopensand_eth_lan_adapt_plugin_la_h = \
	EthernetHeader.h \
	Evc.h \
	Ethernet.h

libopensand_eth_lan_adapt_plugin_la_SOURCES = \
	$(libopensand_eth_lan_adapt_plugin_la_cpp) \
	$(libopensand_eth_lan_adapt_plugin_la_h)

libopensand_eth_lan_adapt_plugin_la_LIBADD = \
	$(top_builddir)/src/conf/libopensand_conf_core.la \
	$(top_builddir)/src/lan_adaptation/libopensand_lan_adaptation.la

pluginsdir = $(prefix)/lib/opensand/plugins

libopensand_eth_lan_adapt_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/lan_adaptation


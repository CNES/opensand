################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the IP lan adaptation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_ip_lan_adapt_plugin.la

libopensand_ip_lan_adapt_plugin_la_cpp = \
	IpPacket.cpp \
	Ipv4Packet.cpp \
	Ipv6Packet.cpp \
	Ip.cpp

libopensand_ip_lan_adapt_plugin_la_h = \
	IpPacket.h \
	Ipv4Packet.h \
	Ipv6Packet.h \
	Ip.h

libopensand_ip_lan_adapt_plugin_la_SOURCES = \
    $(libopensand_ip_lan_adapt_plugin_la_cpp) \
    $(libopensand_ip_lan_adapt_plugin_la_h)

libopensand_ip_lan_adapt_plugin_la_LIBADD = \
	$(top_builddir)/src/lan_adaptation/libopensand_lan_adaptation.la
 
pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/lan_adaptation


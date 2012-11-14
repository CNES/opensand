SUBDIRS = . tests

noinst_LTLIBRARIES = libopensand_common.la
lib_LTLIBRARIES = libopensand_plugin.la

libopensand_common_la_cpp = \
	msg_dvb_rcs.cpp \
	IpPacket.cpp \
	Ipv4Packet.cpp \
	Ipv6Packet.cpp \
	IpAddress.cpp \
	Ipv4Address.cpp \
	Ipv6Address.cpp \
	PluginUtils.cpp

libopensand_common_la_h = \
	msg_dvb_rcs.h \
	IpPacket.h \
	Ipv4Packet.h \
	Ipv6Packet.h \
	IpAddress.h \
	Ipv4Address.h \
	Ipv6Address.h \
	IpPacketHandler.h \
	PluginUtils.h

libopensand_plugin_la_cpp = \
	Data.cpp \
	NetPacket.cpp \
	NetBurst.cpp

libopensand_plugin_la_h = \
	OpenSandPlugin.h \
	OpenSandCore.h \
	Data.h \
	NetPacket.h \
	NetBurst.h \
	EncapPlugin.h \
	PhysicalLayerPlugin.h
	
INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils/

libopensand_common_la_LIBADD = -ldl

libopensand_common_la_SOURCES = \
	$(libopensand_common_la_cpp) \
	$(libopensand_common_la_h)

libopensand_plugin_la_SOURCES = \
	$(libopensand_plugin_la_cpp) \
	$(libopensand_plugin_la_h)

libopensand_plugin_includedir = ${includedir}/opensand_plugin

libopensand_plugin_include_HEADERS = \
	$(libopensand_plugin_la_h) \
	$(top_srcdir)/src/dvb_rcs/utils/lib_dvb_rcs.h

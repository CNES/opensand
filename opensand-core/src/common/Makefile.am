SUBDIRS = . tests

noinst_LTLIBRARIES = libopensand_common.la
lib_LTLIBRARIES = libopensand_encap_plugin.la

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

libopensand_encap_plugin_la_cpp = \
	Data.cpp \
	NetPacket.cpp \
	NetBurst.cpp

libopensand_encap_plugin_la_h = \
	OpenSandCore.h \
	Data.h \
	NetPacket.h \
	NetBurst.h \
	EncapPlugin.h

libopensand_common_la_LIBADD = -ldl

libopensand_common_la_SOURCES = \
	$(libopensand_common_la_cpp) \
	$(libopensand_common_la_h)

libopensand_encap_plugin_la_SOURCES = \
	$(libopensand_encap_plugin_la_cpp) \
	$(libopensand_encap_plugin_la_h)

libopensand_encap_plugin_includedir = ${includedir}/opensand_encap_plugin

libopensand_encap_plugin_include_HEADERS = \
	$(libopensand_encap_plugin_la_h)

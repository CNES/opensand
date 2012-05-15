SUBDIRS = . tests

noinst_LTLIBRARIES = libplatine_common.la
lib_LTLIBRARIES = libplatine_encap_plugin.la

libplatine_common_la_cpp = \
	msg_dvb_rcs.cpp \
	IpPacket.cpp \
	Ipv4Packet.cpp \
	Ipv6Packet.cpp \
	IpAddress.cpp \
	Ipv4Address.cpp \
	Ipv6Address.cpp \
	PluginUtils.cpp

libplatine_common_la_h = \
	PlatineCore.h \
	msg_dvb_rcs.h \
	IpPacket.h \
	Ipv4Packet.h \
	Ipv6Packet.h \
	IpAddress.h \
	Ipv4Address.h \
	Ipv6Address.h \
	IpPacketHandler.h \
	PluginUtils.h

libplatine_encap_plugin_la_cpp = \
	Data.cpp \
	NetPacket.cpp \
	NetBurst.cpp

libplatine_encap_plugin_la_h = \
	Data.h \
	NetPacket.h \
	NetBurst.h \
	EncapPlugin.h

libplatine_common_la_SOURCES = \
	$(libplatine_common_la_cpp) \
	$(libplatine_common_la_h)

libplatine_encap_plugin_la_SOURCES = \
	$(libplatine_encap_plugin_la_cpp) \
	$(libplatine_encap_plugin_la_h)

libplatine_encap_plugin_includedir = ${includedir}/platine_encap_plugin

libplatine_encap_plugin_include_HEADERS = \
	$(libplatine_encap_plugin_la_h)

SUBDIRS = tests

noinst_LTLIBRARIES = libplatine_common.la

libplatine_common_la_cpp = \
	msg_dvb_rcs.cpp \
	Data.cpp \
	NetPacket.cpp \
	IpPacket.cpp \
	Ipv4Packet.cpp \
	Ipv6Packet.cpp \
	IpAddress.cpp \
	Ipv4Address.cpp \
	Ipv6Address.cpp \
	AtmCell.cpp \
	Aal5Packet.cpp \
	MpegPacket.cpp \
	UlePacket.cpp \
	GsePacket.cpp \
	NetBurst.cpp \
	AtmIdentifier.cpp \
	GseIdentifier.cpp

libplatine_common_la_rohc_cpp = \
	RohcPacket.cpp

libplatine_common_la_h = \
	msg_dvb_rcs.h \
	Data.h \
	NetPacket.h \
	IpPacket.h \
	Ipv4Packet.h \
	Ipv6Packet.h \
	IpAddress.h \
	Ipv4Address.h \
	Ipv6Address.h \
	AtmCell.h \
	Aal5Packet.h \
	MpegPacket.h \
	UlePacket.h \
	GsePacket.h \
	NetBurst.h \
	AtmIdentifier.h \
	GseIdentifier.h

libplatine_common_la_rohc_h = \
	RohcPacket.h

libplatine_common_la_SOURCES = \
	$(libplatine_common_la_cpp) \
	$(libplatine_common_la_rohc_cpp) \
	$(libplatine_common_la_h) \
	$(libplatine_common_la_rohc_h)

INCLUDES = \
	-I../dama \
	-I../dvb_rcs_base

#AM_CPPFLAGS= -g -Wall -Werror -DUTI_DEBUG_ON


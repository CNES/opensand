################################################################################
#	Name       : Makefile
#	Author     : Cyrille Gaillardet <cyrille.gaillardet@toulouse.viveris.com>
#	Description: create the OpenSAND rt library
################################################################################

SUBDIRS = . test

lib_LTLIBRARIES = libopensand_rt.la

libopensand_rt_la_cpp = \
	Rt.cpp \
	Block.cpp \
	RtCommunicate.cpp \
	RtChannelBase.cpp \
	RtChannelMux.cpp \
	RtChannel.cpp \
	RtMutex.cpp \
	BlockManager.cpp \
	RtEvent.cpp \
	MessageEvent.cpp \
	TimerEvent.cpp \
	TcpListenEvent.cpp \
	NetSocketEvent.cpp  \
	FileEvent.cpp  \
	SignalEvent.cpp \
	RtFifo.cpp

libopensand_rt_la_h = \
	Rt.h \
	RtCommunicate.h \
	RtChannelBase.h \
	RtChannelMux.h \
	RtChannelMuxDemux.h \
	RtChannelDemux.h \
	RtChannel.h \
	RtMutex.h \
	Data.h \
	Ptr.h \
	Types.h \
	Block.h \
	BlockManager.h \
	RtEvent.h \
	MessageEvent.h \
	TimerEvent.h \
	TcpListenEvent.h \
	NetSocketEvent.h \
	FileEvent.h \
	SignalEvent.h \
	RtFifo.h \
	TemplateHelper.h

libopensand_rt_la_SOURCES = $(libopensand_rt_la_cpp) $(libopensand_rt_la_h)
#libopensand_rt_la_LIBADD = -lrt -lpthread /usr/lib/libtcmalloc_minimal.so

libopensand_rt_includedir = ${includedir}/opensand_rt

libopensand_rt_include_HEADERS = \
	$(libopensand_rt_la_h)

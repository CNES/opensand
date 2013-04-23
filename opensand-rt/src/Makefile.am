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
	Channel.cpp \
	BlockManager.cpp \
	Event.cpp \
	MessageEvent.cpp \
	TimerEvent.cpp \
	NetSocketEvent.cpp  \
	SignalEvent.cpp \
	Fifo.cpp


libopensand_rt_la_h = \
	Rt.h \
	Channel.h \
	Types.h \
	Block.h \
	BlockManager.h \
	Event.h \
	MessageEvent.h \
	TimerEvent.h \
	NetSocketEvent.h \
	SignalEvent.h \
	Fifo.h

libopensand_rt_la_SOURCES = $(libopensand_rt_la_cpp) $(libopensand_rt_la_h)
#libopensand_rt_la_LIBADD = -lrt -lpthread /usr/lib/libtcmalloc_minimal.so

libopensand_rt_includedir = ${includedir}/opensand_rt

libopensand_rt_include_HEADERS = \
	Rt.h \
	Channel.h \
	Types.h \
	Block.h \
	TimerEvent.h \
	NetSocketEvent.h \
	SignalEvent.h
	
# TODO add event.h if custom event

################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ULE plugin for Platine
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libplatine_mpeg_plugin.la

libplatine_mpeg_plugin_la_cpp = \
	MpegEncapCtx.cpp \
	MpegDeencapCtx.cpp \
	MpegPacket.cpp \
	Mpeg.cpp

libplatine_mpeg_plugin_la_h = \
     	MpegEncapCtx.h \
	MpegDeencapCtx.h \
	MpegPacket.h \
	Mpeg.h

libplatine_mpeg_plugin_la_SOURCES = \
    $(libplatine_mpeg_plugin_la_cpp) \
    $(libplatine_mpeg_plugin_la_h)

pluginsdir = $(prefix)/lib/platine/plugins

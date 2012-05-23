################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ULE plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_mpeg_plugin.la

libopensand_mpeg_plugin_la_cpp = \
	MpegEncapCtx.cpp \
	MpegDeencapCtx.cpp \
	MpegPacket.cpp \
	Mpeg.cpp

libopensand_mpeg_plugin_la_h = \
     	MpegEncapCtx.h \
	MpegDeencapCtx.h \
	MpegPacket.h \
	Mpeg.h

libopensand_mpeg_plugin_la_SOURCES = \
    $(libopensand_mpeg_plugin_la_cpp) \
    $(libopensand_mpeg_plugin_la_h)

pluginsdir = $(prefix)/lib/opensand/plugins

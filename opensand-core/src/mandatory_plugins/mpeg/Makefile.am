################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the MPEG encapsulation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_mpeg_encap_plugin.la

libopensand_mpeg_encap_plugin_la_cpp = \
	MpegEncapCtx.cpp \
	MpegDeencapCtx.cpp \
	MpegPacket.cpp \
	Mpeg.cpp

libopensand_mpeg_encap_plugin_la_h = \
	MpegEncapCtx.h \
	MpegDeencapCtx.h \
	MpegPacket.h \
	Mpeg.h

libopensand_mpeg_encap_plugin_la_SOURCES = \
    $(libopensand_mpeg_encap_plugin_la_cpp) \
    $(libopensand_mpeg_encap_plugin_la_h)

pluginsdir = $(libdir)/opensand/plugins

INCLUDES = \
	-I$(top_srcdir)/src/common

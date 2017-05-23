################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the ACM loop minimal condition plugin for OpenSAND
################################################################################

SUBDIRS =

plugins_LTLIBRARIES = libopensand_acm_loop_minimal_plugin.la

libopensand_acm_loop_minimal_plugin_la_cpp = \
    AcmLoop.cpp

libopensand_acm_loop_minimal_plugin_la_h = \
    AcmLoop.h

libopensand_acm_loop_minimal_plugin_la_SOURCES = \
    $(libopensand_acm_loop_minimal_plugin_la_cpp) \
    $(libopensand_acm_loop_minimal_plugin_la_h)
    
libopensand_acm_loop_minimal_plugin_la_LIBADD= \
    $(top_builddir)/src/dvb/fmt/libopensand_dvb_fmt.la

libopensand_acm_loop_minimal_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/dvb/utils \
    -I$(top_srcdir)/src/dvb/fmt \
	-I$(top_srcdir)/src/common

pluginsdir = $(libdir)/opensand/plugins

################################################################################
#   Name       : Makefile
#   Author     : Aurélien DELRIEU <aureleien.delrieu@viveris.fr
#   Description: create the configuration library for OpenSAND tools
################################################################################

SUBDIRS = \
	. \
	tests

pyexecdir=$(prefix)/lib/python3/dist-packages
pyexec_LTLIBRARIES = py_opensand_conf.la

py_opensand_conf_la_SOURCES = \
	PyConfiguration.cpp

py_opensand_conf_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(LIBPYTHON_CFLAGS) \
	-I$(top_srcdir)/src/
#	-I$(top_srcdir)/include/

py_opensand_conf_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	$(LIBPYTHON_LIBS) \
	-module \
	-L$(top_builddir)/src/.libs \
	-lopensand_conf \
	-lboost_python38
#	-fno-implicit-templates 
#	-Wl,--no-undefined 

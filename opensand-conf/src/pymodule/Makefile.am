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
	$(PYTHON_CPPFLAGS) \
	-I$(top_srcdir)/src/
#	-I$(top_srcdir)/include/

py_opensand_conf_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	$(PYTHON_LIBS) \
	$(BOOST_PYTHON_LIB) \
	-module \
	-L$(top_builddir)/src/.libs \
	-lopensand_conf
#	-fno-implicit-templates 
#	-Wl,--no-undefined 

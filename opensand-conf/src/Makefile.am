################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the configuration library for OpenSAND tools
################################################################################

SUBDIRS = \
	test

lib_LTLIBRARIES = libopensand_conf.la

libopensand_conf_la_cpp = \
    ConfigurationList.cpp \
    ConfigurationFile.cpp

libopensand_conf_la_h = \
    ConfigurationList.h \
    ConfigurationFile.h \
    conf.h

libopensand_conf_la_SOURCES = \
    $(libopensand_conf_la_cpp) 

libopensand_conf_includedir = ${includedir}/opensand_conf

libopensand_conf_include_HEADERS = \
    ConfigurationList.h \
    ConfigurationFile.h \
    conf.h

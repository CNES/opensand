################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the configuration library for Platine tools
################################################################################

lib_LTLIBRARIES = libplatine_conf.la

libplatine_conf_la_cpp = \
    uti_debug.cpp \
    ConfigurationFile.cpp

libplatine_conf_la_h = \
    uti_debug.h \
    ConfigurationFile.h \
    conf.h

libplatine_conf_la_SOURCES = \
    $(libplatine_conf_la_cpp) \
    $(libplatine_conf_la_h)

libplatine_conf_includedir = ${includedir}/platine_conf

libplatine_conf_include_HEADERS = \
    uti_debug.h \
    ConfigurationFile.h \
    conf.h

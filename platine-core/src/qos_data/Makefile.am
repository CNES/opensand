noinst_LTLIBRARIES = libplatine_qosdata.la

libplatine_qosdata_la_cpp = \
	bloc_ip_qos.cpp \
	ipqos_init.cpp \
	SarpTable.cpp \
	ServiceClass.cpp \
	TrafficCategory.cpp

libplatine_qosdata_la_h = \
	bloc_ip_qos.h \
	SarpTable.h \
	ServiceClass.h \
	TrafficCategory.h

libplatine_qosdata_la_SOURCES = $(libplatine_qosdata_la_cpp) $(libplatine_qosdata_la_h)
 
INCLUDES = \
	-I$(top_srcdir)/src/common \
	-Ilinux

AM_LDFLAGS= -lm


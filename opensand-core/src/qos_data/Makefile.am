noinst_LTLIBRARIES = libopensand_qosdata.la

libopensand_qosdata_la_cpp = \
	bloc_ip_qos.cpp \
	ipqos_init.cpp \
	SarpTable.cpp \
	ServiceClass.cpp \
	TrafficCategory.cpp

libopensand_qosdata_la_h = \
	bloc_ip_qos.h \
	SarpTable.h \
	ServiceClass.h \
	TrafficCategory.h

libopensand_qosdata_la_SOURCES = $(libopensand_qosdata_la_cpp) $(libopensand_qosdata_la_h)
 
INCLUDES = \
	-I$(top_srcdir)/src/common \
	-Ilinux

AM_LDFLAGS= -lm


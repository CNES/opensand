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
	-I../dvb_rcs_base \
	-I../dvb_rcs \
	-I../dama \
	-I../common \
	-Ilinux

#AM_CPPFLAGS= -g -Werror -DUTI_DEBUG_ON -Wall 

AM_LDFLAGS= -lm


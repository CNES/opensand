
lib_LTLIBRARIES = libplatine_dvb_dra.la

libplatine_dvb_dra_la_cpp = \
	DraSchemeDefinition.cpp \
	DraSchemeDefinitionTable.cpp
       
libplatine_dvb_dra_la_h = \
	DraSchemeDefinition.h \
	DraSchemeDefinitionTable.h

libplatine_dvb_dra_la_SOURCES = \
	$(libplatine_dvb_dra_la_cpp) \
	$(libplatine_dvb_dra_la_h)

INCLUDES = \
	-I../utils

AM_CPPFLAGS = -g -Wall -Werror -DUTI_DEBUG_ON


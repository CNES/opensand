lib_LTLIBRARIES = libplatine_dama.la

libplatine_dama_la_cpp = \
	lib_circular_buffer.cpp \
	lib_dama_utils.cpp \
	lib_dama_agent.cpp \
	lib_dama_agent_esa.cpp \
	lib_dama_agent_uor.cpp \
	lib_dama_ctrl.cpp  \
	lib_dama_ctrl_stub.cpp  \
	lib_dama_ctrl_yes.cpp \
	lib_dama_ctrl_st.cpp \
	lib_dama_ctrl_esa.cpp \
  	lib_dama_ctrl_uor.cpp
       
libplatine_dama_la_h = \
	lib_circular_buffer.h \
	lib_dama_utils.h  \
	lib_dama_agent.h  \
	lib_dama_agent_esa.h \
 	lib_dama_agent_uor.h \
	lib_dama_ctrl.h \
	lib_dama_ctrl_stub.h \
	lib_dama_ctrl_yes.h \
	lib_dama_ctrl_st.h \
	lib_dama_ctrl_esa.h \
     	lib_dama_ctrl_uor.h 

libplatine_dama_la_SOURCES = \
	$(libplatine_dama_la_cpp) \
	$(libplatine_dama_la_h)

INCLUDES = \
	-I.. \
	-I../utils \
	-I../dra \
	-I../pep \
	-I../core \
	-I$(top_srcdir)/src/common

AM_CPPFLAGS = -g -Wall -Werror -DUTI_DEBUG_ON


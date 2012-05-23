lib_LTLIBRARIES = libopensand_dama.la

libopensand_dama_la_cpp = \
	lib_circular_buffer.cpp \
	lib_dama_utils.cpp \
	lib_dama_agent.cpp \
	lib_dama_agent_legacy.cpp \
	lib_dama_agent_uor.cpp \
	lib_dama_ctrl.cpp  \
	lib_dama_ctrl_stub.cpp  \
	lib_dama_ctrl_yes.cpp \
	lib_dama_ctrl_st.cpp \
	lib_dama_ctrl_legacy.cpp \
  	lib_dama_ctrl_uor.cpp
       
libopensand_dama_la_h = \
	lib_circular_buffer.h \
	lib_dama_utils.h  \
	lib_dama_agent.h  \
	lib_dama_agent_legacy.h \
 	lib_dama_agent_uor.h \
	lib_dama_ctrl.h \
	lib_dama_ctrl_stub.h \
	lib_dama_ctrl_yes.h \
	lib_dama_ctrl_st.h \
	lib_dama_ctrl_legacy.h \
	lib_dama_ctrl_uor.h 

libopensand_dama_la_SOURCES = \
	$(libopensand_dama_la_cpp) \
	$(libopensand_dama_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/dra \
	-I$(top_srcdir)/src/dvb_rcs/pep \
	-I$(top_srcdir)/src/dvb_rcs/core \
	-I$(top_srcdir)/src/common

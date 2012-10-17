lib_LTLIBRARIES = libopensand_dama.la

libopensand_dama_la_cpp = \
	CircularBuffer.cpp \
	UplinkSchedulingRcs.cpp \
	DamaUtils.cpp \
	DamaAgent.cpp \
	DamaAgentRcs.cpp \
	DamaAgentRcsLegacy.cpp \
	DamaAgentRcsUor.cpp \
	lib_dama_ctrl.cpp  \
	lib_dama_ctrl_yes.cpp \
	lib_dama_ctrl_st.cpp \
	lib_dama_ctrl_legacy.cpp \
  	lib_dama_ctrl_uor.cpp
       
libopensand_dama_la_h = \
	CircularBuffer.h \
	UplinkScheduling.h \
	UplinkSchedulingRcs.h \
	DamaUtils.h  \
	Ttp.h \
	CapacityRequest.h \
	DamaAgent.h  \
	DamaAgentRcs.h  \
	DamaAgentRcsLegacy.h \
 	DamaAgentRcsUor.h \
	lib_dama_ctrl.h \
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

noinst_LTLIBRARIES = libopensand_dvb_utils.la

libopensand_dvb_utils_la_cpp = \
	CapacityRequest.cpp \
	DvbFifo.cpp \
	MacFifoElement.cpp \
	sat_emulator_err.cpp \
	lib_dvb_rcs.cpp
     
libopensand_dvb_utils_la_h = \
	CapacityRequest.h \
	DvbFifo.h \
	ModulationType.h \
	MacFifoElement.h \
	sat_emulator_err.h \
	msg_dvb_rcs.h \
	lib_dvb_rcs.h

libopensand_dvb_utils_la_SOURCES = \
	$(libopensand_dvb_utils_la_cpp) \
	$(libopensand_dvb_utils_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/common

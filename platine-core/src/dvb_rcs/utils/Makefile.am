noinst_LTLIBRARIES = libplatine_dvb_utils.la

libplatine_dvb_utils_la_cpp = \
	dvb_fifo.cpp \
	MacFifoElement.cpp \
	sat_emulator_err.cpp \
	lib_dvb_rcs.cpp
     
libplatine_dvb_utils_la_h = \
	dvb_fifo.h \
	ModulationType.h \
	MacFifoElement.h \
	sat_emulator_err.h \
	lib_dvb_rcs.h

libplatine_dvb_utils_la_SOURCES = \
	$(libplatine_dvb_utils_la_cpp) \
	$(libplatine_dvb_utils_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/common

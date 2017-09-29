noinst_LTLIBRARIES = libopensand_dvb_ncc_interface.la

libopensand_dvb_ncc_interface_la_cpp = \
	NccInterface.cpp \
	PepRequest.cpp \
	NccPepInterface.cpp \
	SvnoRequest.cpp \
	NccSvnoInterface.cpp

libopensand_dvb_ncc_interface_la_h = \
	NccInterface.h \
	PepRequest.h \
	NccPepInterface.h \
	SvnoRequest.h \
	NccSvnoInterface.h

libopensand_dvb_ncc_interface_la_SOURCES = \
	$(libopensand_dvb_ncc_interface_la_cpp) \
	$(libopensand_dvb_ncc_interface_la_h)

libopensand_dvb_ncc_interface_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common

noinst_LTLIBRARIES = libplatine_encap.la

libplatine_encap_la_cpp = \
	EncapCtx.cpp \
	Aal5Ctx.cpp \
	AtmCtx.cpp \
	AtmAal5Ctx.cpp \
	MpegEncapCtx.cpp \
	MpegDesencapCtx.cpp \
	MpegCtx.cpp \
	UleExt.cpp \
	UleExtTest.cpp \
	UleExtPadding.cpp \
	UleCtx.cpp \
	GseEncapCtx.cpp \
	GseCtx.cpp \
	MpegUleCtx.cpp \
	MpegAtmAal5Ctx.cpp \
	GseAtmAal5Ctx.cpp \
	GseMpegUleCtx.cpp \
	bloc_encap.cpp \
	bloc_encap_sat.cpp

libplatine_encap_la_rohc_cpp = \
	RohcCtx.cpp \
	AtmAal5RohcCtx.cpp \
	MpegUleRohcCtx.cpp \
	MpegAtmAal5RohcCtx.cpp \
	GseRohcCtx.cpp \
	GseAtmAal5RohcCtx.cpp \
	GseMpegUleRohcCtx.cpp

if ule_security
libplatine_encap_la_cpp += \
	UleExtSecurity.cpp
endif

libplatine_encap_la_h = \
	EncapCtx.h \
	Aal5Ctx.h \
	AtmCtx.h \
	AtmAal5Ctx.h \
	MpegEncapCtx.h \
	MpegDesencapCtx.h \
	MpegCtx.h \
	UleExt.h \
	UleExtTest.h \
	UleExtPadding.h \
	UleCtx.h \
	MpegUleCtx.h \
	GseEncapCtx.h \
	GseCtx.h \
	MpegAtmAal5Ctx.h \
	GseAtmAal5Ctx.h \
	GseMpegUleCtx.h \
	bloc_encap.h \
	bloc_encap_sat.h

libplatine_encap_la_rohc_h = \
	RohcCtx.h \
	AtmAal5RohcCtx.h \
	MpegUleRohcCtx.h \
	MpegAtmAal5RohcCtx.h \
	GseRohcCtx.h \
	GseAtmAal5RohcCtx.h \
	GseMpegUleRohcCtx.h

if ule_security
libplatine_encap_la_h += \
	UleExtSecurity.h
endif

libplatine_encap_la_SOURCES = \
	$(libplatine_encap_la_cpp) \
	$(libplatine_encap_la_h)
#	$(libplatine_encap_la_rohc_cpp) \
#	$(libplatine_encap_la_rohc_h)

libplatine_encap_la_DEPENDENCIES = \
	$(libplatine_encap_la_cpp) \
	$(libplatine_encap_la_h)
#	$(libplatine_encap_la_rohc_cpp) \
#	$(libplatine_encap_la_rohc_h)

INCLUDES = \
	-I../dvb_rcs_base \
	-I../qos_data \
	-I../dama \
	-I../common

#AM_CPPFLAGS= -g -Wall -Werror -DUTI_DEBUG_ON

if ule_security
AM_LDFLAGS= -lcryptopp
endif

EXTRA_DIST=readme.txt


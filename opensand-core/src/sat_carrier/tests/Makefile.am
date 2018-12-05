noinst_PROGRAMS = test_sat_carrier

PACKED_COMMON_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/sat_carrier \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common 

PACKED_COMMON_LIBS= \
	$(top_builddir)/src/common/libopensand_utils.la \
	$(top_builddir)/src/common/libopensand_plugin.la \
	$(top_builddir)/src/sat_carrier/libopensand_satcarrier.la \
	$(top_builddir)/src/conf/libopensand_conf_core.la

allexec_LDADD = \
	-lpthread \
	-lrt

test_sat_carrier_SOURCES = \
	TestSatCarriers.cpp \
	TestSatCarriers.h \
	test.cpp

test_sat_carrier_CPPFLAGS = \
	$(PACKED_COMMON_CPPFLAGS)

test_sat_carrier_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)

test_sat_carrier_fifo_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)



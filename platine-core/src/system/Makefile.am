bin_PROGRAMS = st gw sat

INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/switch \
	-I$(top_srcdir)/src/dvb_rcs/pep \
	-I$(top_srcdir)/src/dvb_rcs/dra \
	-I$(top_srcdir)/src/dvb_rcs/modcod \
	-I$(top_srcdir)/src/dvb_rcs/dama \
	-I$(top_srcdir)/src/dvb_rcs/core \
	-I$(top_srcdir)/src/qos_data \
	-I$(top_srcdir)/src/sat_carrier \
	-I$(top_srcdir)/src/encap \
	-I$(top_srcdir)/src/common

PACKED_COMMON_LIBS= \
	$(top_builddir)/src/encap/libplatine_encap.la \
	$(top_builddir)/src/qos_data/libplatine_qosdata.la \
	$(top_builddir)/src/dvb_rcs/libplatine_dvb.la \
	$(top_builddir)/src/dvb_rcs/dama/libplatine_dama.la \
	$(top_builddir)/src/dvb_rcs/pep/libplatine_dvb_pep.la \
	$(top_builddir)/src/dvb_rcs/utils/libplatine_dvb_utils.la \
	$(top_builddir)/src/sat_carrier/libplatine_satcarrier.la \
	$(top_builddir)/src/common/libplatine_encap_plugin.la \
	$(top_builddir)/src/common/libplatine_common.la

allexec_LDADD = \
	-lpthread \
	-lrt

st_SOURCES = st.cpp
st_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD) 

gw_SOURCES = gw.cpp
gw_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD) 

sat_SOURCES = sat.cpp
sat_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)


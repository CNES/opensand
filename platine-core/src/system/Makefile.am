bin_PROGRAMS = st gw sat

INCLUDES = \
	-I../dvb_rcs/utils \
	-I../dvb_rcs/switch \
	-I../dvb_rcs/pep \
	-I../dvb_rcs/dra \
	-I../dvb_rcs/modcod \
	-I../dvb_rcs/dama \
	-I../dvb_rcs/core \
	-I../qos_data \
	-I../sat_carrier \
	-I../encap \
	-I../common

#AM_CPPFLAGS= -g -DUTI_DEBUG_ON -Wall -Werror

PACKED_COMMON_LIBS= \
	../encap/libplatine_encap.la \
	../qos_data/libplatine_qosdata.la \
	../dvb_rcs/libplatine_dvb.la \
	../dvb_rcs/dama/libplatine_dama.la \
	../dvb_rcs/pep/libplatine_dvb_pep.la \
	../dvb_rcs/utils/libplatine_dvb_utils.la \
	../sat_carrier/libplatine_satcarrier.la \
	../common/libplatine_encap_plugin.la \
	../common/libplatine_common.la

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


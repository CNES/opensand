bin_PROGRAMS = st gw sat

INCLUDES = \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/switch \
	-I$(top_srcdir)/src/dvb_rcs/pep \
	-I$(top_srcdir)/src/dvb_rcs/fmt \
	-I$(top_srcdir)/src/dvb_rcs/dama \
	-I$(top_srcdir)/src/dvb_rcs/saloha \
	-I$(top_srcdir)/src/dvb_rcs/core \
	-I$(top_srcdir)/src/lan_adaptation \
	-I$(top_srcdir)/src/sat_carrier \
	-I$(top_srcdir)/src/encap \
	-I$(top_srcdir)/src/physical_layer \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

PACKED_COMMON_LIBS= \
	$(top_builddir)/src/encap/libopensand_encap.la \
	$(top_builddir)/src/lan_adaptation/libopensand_lan_adaptation.la \
	$(top_builddir)/src/dvb_rcs/libopensand_dvb.la \
	$(top_builddir)/src/dvb_rcs/dama/libopensand_dama.la \
	$(top_builddir)/src/dvb_rcs/pep/libopensand_dvb_pep.la \
	$(top_builddir)/src/dvb_rcs/saloha/libopensand_dvb_saloha.la \
	$(top_builddir)/src/dvb_rcs/utils/libopensand_dvb_utils.la \
	$(top_builddir)/src/physical_layer/libopensand_physical_layer.la \
	$(top_builddir)/src/sat_carrier/libopensand_satcarrier.la \
	$(top_builddir)/src/conf/libopensand_conf_core.la \
	$(top_builddir)/src/common/libopensand_plugin.la \
	$(top_builddir)/src/common/libopensand_plugin_utils.la

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


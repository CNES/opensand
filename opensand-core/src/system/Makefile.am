bin_PROGRAMS = opensand-st opensand-sat opensand-gw opensand-gw-net-acc opensand-gw-phy

PACKED_COMMON_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/dvb/switch \
	-I$(top_srcdir)/src/dvb/ncc_interface \
	-I$(top_srcdir)/src/dvb/fmt \
	-I$(top_srcdir)/src/dvb/dama \
	-I$(top_srcdir)/src/dvb/saloha \
	-I$(top_srcdir)/src/dvb/core/transparent \
	-I$(top_srcdir)/src/dvb/core \
	-I$(top_srcdir)/src/lan_adaptation \
	-I$(top_srcdir)/src/interconnect \
	-I$(top_srcdir)/src/sat_carrier \
	-I$(top_srcdir)/src/encap \
	-I$(top_srcdir)/src/physical_layer \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common

PACKED_COMMON_LIBS= \
	$(top_builddir)/src/encap/libopensand_encap.la \
	$(top_builddir)/src/lan_adaptation/libopensand_lan_adaptation.la \
	$(top_builddir)/src/physical_layer/libopensand_physical_layer.la \
	$(top_builddir)/src/dvb/libopensand_dvb.la \
	$(top_builddir)/src/dvb/dama/libopensand_dama.la \
	$(top_builddir)/src/dvb/ncc_interface/libopensand_dvb_ncc_interface.la \
	$(top_builddir)/src/dvb/saloha/libopensand_dvb_saloha.la \
	$(top_builddir)/src/dvb/core/transparent/libopensand_dvb_core_transp.la \
	$(top_builddir)/src/dvb/utils/libopensand_dvb_utils.la \
	$(top_builddir)/src/sat_carrier/libopensand_satcarrier.la \
	$(top_builddir)/src/interconnect/libopensand_interconnect.la \
	$(top_builddir)/src/conf/libopensand_conf_core.la \
	$(top_builddir)/src/common/libopensand_plugin.la \
	$(top_builddir)/src/common/libopensand_plugin_utils.la \
	$(top_builddir)/src/common/libopensand_utils.la

allexec_LDADD = \
	-lpthread \
	-lrt

opensand_st_SOURCES = st.cpp
opensand_st_CPPFLAGS = $(PACKED_COMMON_CPPFLAGS)
opensand_st_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)

opensand_gw_SOURCES = gw.cpp
opensand_gw_CPPFLAGS = $(PACKED_COMMON_CPPFLAGS)
opensand_gw_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)

opensand_gw_net_acc_SOURCES = gw_net_acc.cpp
opensand_gw_net_acc_CPPFLAGS = $(PACKED_COMMON_CPPFLAGS)
opensand_gw_net_acc_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)

opensand_gw_phy_SOURCES = gw_phy.cpp
opensand_gw_phy_CPPFLAGS = $(PACKED_COMMON_CPPFLAGS)
opensand_gw_phy_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)

opensand_sat_SOURCES = sat.cpp
opensand_sat_CPPFLAGS = $(PACKED_COMMON_CPPFLAGS)
opensand_sat_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)

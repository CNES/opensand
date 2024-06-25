# Be very careful, as Plugin define a static instance of
# PluginUtils, you MUST NOT link with libopensand_plugin_utils.la !!
noinst_LTLIBRARIES = libopensand_plugin_utils.la libopensand_utils.la
lib_LTLIBRARIES = libopensand_plugin.la

libopensand_plugin_utils_la_cpp = \
	PluginLibdir.cpp \
	PluginUtils.cpp \
	Plugin.cpp

libopensand_plugin_utils_la_h = \
	PluginUtils.h \
	Plugin.h

libopensand_plugin_la_cpp = \
	OpenSandCore.cpp \
	Except.cpp \
	CarrierType.cpp \
	FifoElement.cpp \
	NetContainer.cpp \
	NetPacket.cpp \
	NetBurst.cpp \
	MacAddress.cpp \
	TrafficCategory.cpp \
	SarpTable.cpp \
	StackPlugin.cpp \
	SimpleEncapPlugin.cpp \
	LanAdaptationPlugin.cpp \
	PhysicalLayerPlugin.cpp \
	IslPlugin.cpp \
	SpotComponentPair.cpp \
	DelayFifo.cpp

libopensand_plugin_la_h = \
	OpenSandPlugin.h \
	StackPlugin.h \
	OpenSandCore.h \
	Except.h \
	CarrierType.h \
	FifoElement.h \
	NetContainer.h \
	NetPacket.h \
	NetBurst.h \
	MacAddress.h \
	TrafficCategory.h \
	SarpTable.h \
	SimpleEncapPlugin.h \
	LanAdaptationPlugin.h \
	PhysicalLayerPlugin.h \
	IslPlugin.h \
	SpotComponentPair.h \
	DelayFifo.h

libopensand_utils_la_cpp = \
	UdpChannel.cpp

libopensand_utils_la_h = \
	UdpChannel.h

libopensand_utils_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

libopensand_utils_la_SOURCES = \
	$(libopensand_utils_la_cpp) \
	$(libopensand_utils_la_h)

libopensand_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/dvb/utils/

libopensand_plugin_la_SOURCES = \
	$(libopensand_plugin_la_cpp) \
	$(libopensand_plugin_la_h)

libopensand_plugin_la_LIBADD = \
	$(top_builddir)/src/conf/libopensand_conf_core.la

libopensand_plugin_utils_la_LIBADD = \
	-ldl

libopensand_plugin_utils_la_SOURCES = \
	$(libopensand_plugin_utils_la_cpp) \
	$(libopensand_plugin_utils_la_h)

libopensand_plugin_utils_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf/

libopensand_plugin_includedir = ${includedir}/opensand_plugin

libopensand_plugin_include_HEADERS = \
	$(top_srcdir)/src/conf/OpenSandModelConf.h \
	$(libopensand_plugin_la_h)

CLEANFILES = PluginLibdir.cpp


PluginLibdir.cpp:
	echo '#include <string>' > $@; echo 'extern const std::string PLUGIN_LIBDIR{"'$(libdir)'"};' >> $@

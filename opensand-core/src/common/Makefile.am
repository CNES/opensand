SUBDIRS = tests

# Be very careful, as Plugin define a static instance of
# PluginUtils, you MUST NOT link with libopensand_plugin_utils.la !!
noinst_LTLIBRARIES = libopensand_plugin_utils.la libopensand_utils.la
lib_LTLIBRARIES = libopensand_plugin.la

libopensand_plugin_utils_la_cpp = \
	PluginUtils.cpp \
	Plugin.cpp

libopensand_plugin_utils_la_h = \
	PluginUtils.h \
	Plugin.h
	
# TODO move NetPacket ?

libopensand_plugin_la_cpp = \
	Data.cpp \
	NetContainer.cpp \
	NetPacket.cpp \
	NetBurst.cpp \
	MacAddress.cpp \
	TrafficCategory.cpp \
	SarpTable.cpp \
	EncapPlugin.cpp

libopensand_plugin_la_h = \
	OpenSandPlugin.h \
	StackPlugin.h \
	OpenSandCore.h \
	Data.h \
	NetContainer.h \
	NetPacket.h \
	NetBurst.h \
	MacAddress.h \
	TrafficCategory.h \
	SarpTable.h \
	EncapPlugin.h \
	LanAdaptationPlugin.h \
	PhysicalLayerPlugin.h

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

libopensand_plugin_utils_la_LIBADD = -ldl

libopensand_plugin_utils_la_SOURCES = \
	$(libopensand_plugin_utils_la_cpp) \
	$(libopensand_plugin_utils_la_h)

libopensand_plugin_utils_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf/

libopensand_plugin_includedir = ${includedir}/opensand_plugin

libopensand_plugin_include_HEADERS = \
	$(libopensand_plugin_la_h)

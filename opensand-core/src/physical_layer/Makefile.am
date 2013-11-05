SUBDIRS = plugins

noinst_LTLIBRARIES = libopensand_physical_layer.la

libopensand_physical_layer_la_cpp = \
	BlockPhysicalLayer.cpp \
	BlockPhysicalLayerSat.cpp \
	PhyChannel.cpp

libopensand_physical_layer_la_h = \
	BlockPhysicalLayer.h \
	BlockPhysicalLayerSat.h \
	PhyChannel.h

libopensand_physical_layer_la_SOURCES = \
	$(libopensand_physical_layer_la_cpp) \
	$(libopensand_physical_layer_la_h)

libopensand_physical_layer_la_DEPENDENCIES = \
	$(libopensand_physical_layer_la_cpp) \
	$(libopensand_physical_layer_la_h)

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/dvb_rcs/utils \
	-I$(top_srcdir)/src/dvb_rcs/fmt




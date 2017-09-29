SUBDIRS = plugins . tests

noinst_LTLIBRARIES = libopensand_physical_layer.la

libopensand_physical_layer_la_cpp = \
	DelayFifo.cpp \
	DelayFifoElement.cpp \
	BlockPhysicalLayer.cpp \
	PhyChannel.cpp

libopensand_physical_layer_la_h = \
	DelayFifo.h \
	DelayFifoElement.h \
	BlockPhysicalLayer.h \
	PhyChannel.h

libopensand_physical_layer_la_SOURCES = \
	$(libopensand_physical_layer_la_cpp) \
	$(libopensand_physical_layer_la_h)

libopensand_physical_layer_la_DEPENDENCIES = \
	$(libopensand_physical_layer_la_cpp) \
	$(libopensand_physical_layer_la_h)

libopensand_physical_layer_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/dvb/fmt




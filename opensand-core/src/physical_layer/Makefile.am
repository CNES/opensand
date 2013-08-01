noinst_LTLIBRARIES = libopensand_physical_layer.la

libopensand_physical_layer_la_cpp = \
	BlockPhysicalLayer.cpp \
	AttenuationModel.cpp \
	NominalCondition.cpp \
	MinimalCondition.cpp \
    ErrorInsertion.cpp \
	PhyChannel.cpp

libopensand_physical_layer_la_h = \
	BlockPhysicalLayer.h \
	AttenuationModel.h \
	NominalCondition.h \
	MinimalCondition.h \
	ErrorInsertion.h \
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




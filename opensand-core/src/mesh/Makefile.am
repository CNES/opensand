noinst_LTLIBRARIES = libopensand_mesh.la

libopensand_mesh_la_cpp = \
	BlockTransp.cpp \
	BlockMesh.cpp \
	SatDemuxKey.cpp \
	NetPacketSerializer.cpp

libopensand_mesh_la_h = \
	BlockTransp.h \
	BlockMesh.h \
	SatDemuxKey.h \
	NetPacketSerializer.h

libopensand_mesh_la_SOURCES = \
	$(libopensand_mesh_la_cpp) \
	$(libopensand_mesh_la_h)

libopensand_mesh_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils \
	-I$(top_srcdir)/src/common


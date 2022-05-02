noinst_LTLIBRARIES = libopensand_mesh.la

libopensand_mesh_la_cpp = \
	BlockMesh.cpp \
	NetPacketSerializer.cpp

libopensand_mesh_la_h = \
	BlockMesh.h \
	NetPacketSerializer.h

libopensand_mesh_la_SOURCES = \
	$(libopensand_mesh_la_cpp) \
	$(libopensand_mesh_la_h)

libopensand_mesh_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common


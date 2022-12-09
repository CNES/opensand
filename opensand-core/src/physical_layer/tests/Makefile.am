noinst_PROGRAMS = test_delay_fifo

INCLUDES = \
	-I$(top_srcdir)/src/physical_layer \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/common 

PACKED_COMMON_LIBS= \
	$(top_builddir)/src/common/libopensand_plugin.la \
	$(top_builddir)/src/physical_layer/libopensand_physical_layer.la \
	$(top_builddir)/src/conf/libopensand_conf_core.la

allexec_LDADD = \
	-lpthread \
	-lrt

test_delay_fifo_SOURCES = \
	test_fifo.cpp

test_delay_fifo_LDADD = \
	$(PACKED_COMMON_LIBS) \
	$(allexec_LDADD)

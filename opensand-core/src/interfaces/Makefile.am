SUBDIRS = tests

bin_PROGRAMS = opensand_interfaces


INCLUDES = \
	-I$(top_srcdir)/src/lan_adaptation

opensand_interfaces_SOURCES = opensand_interfaces.c

opensand_interfaces_LDADD = \
	$(top_builddir)/src/lan_adaptation/libopensand_bridge.la

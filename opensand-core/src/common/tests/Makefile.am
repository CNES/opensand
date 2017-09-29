CPPFLAGS_COMMON = -I$(top_srcdir)/src/common -g -Wall

check_PROGRAMS = \
	test_plugins

TESTS_ICMP = \
	test_plugins_icmp_28.sh \
	test_plugins_icmp_64.sh

EXTRA_DIST = \
	test_plugins.sh \
	$(TESTS_ICMP)

############## test for encap plugins ##############

test_plugins_CPPFLAGS = \
  $(AM_CPPFLAGS) \
  -I$(top_srcdir)/src/lan_adaptation/ \
  -I$(top_srcdir)/src/common/ \
  -I$(top_srcdir)/src/dvb/utils/

test_plugins_SOURCES = \
  test_plugins.cpp

test_plugins_CXXFLAGS = $(CPPFLAGS_COMMON)
test_plugins_LDFLAGS =
test_plugins_LDADD = \
  $(top_builddir)/src/lan_adaptation/libopensand_lan_adaptation.la \
  $(top_builddir)/src/common/libopensand_plugin_utils.la \
  $(top_builddir)/src/common/libopensand_plugin.la \
  -lpcap


# Target to test plugin architecture
check-plugins: test_plugins$(EXEEXT)	
	./test_plugins_icmp_28.sh
	./test_plugins_icmp_64.sh

CPPFLAGS_COMMON = -I$(top_srcdir)/src/common -g -Wall

# test programs to build
check_PROGRAMS = \
  ip_addr

noinst_PROGRAMS = \
	test_plugins

# test programs to run
TESTS = \
  ip_addr

TESTS_ICMP = \
	test_plugins_icmp_28.sh \
	test_plugins_icmp_64.sh

EXTRA_DIST = \
	test_plugins.sh \
	$(TESTS_ICMP)

############## test for IP addresses ##############

ip_addr_SOURCES = \
  $(top_srcdir)/src/common/IpAddress.cpp \
  $(top_srcdir)/src/common/Ipv4Address.cpp \
  $(top_srcdir)/src/common/Ipv6Address.cpp \
  ip_addr.cpp

ip_addr_CXXFLAGS = $(CPPFLAGS_COMMON)
ip_addr_LDFLAGS =
ip_addr_LDADD = 


############## test for encap plugins ##############

INCLUDES = \
  -I$(top_srcdir)/src/common/

test_plugins_SOURCES = \
  test_plugins.cpp

test_plugins_CXXFLAGS = $(CPPFLAGS_COMMON)
test_plugins_LDFLAGS =
test_plugins_LDADD = \
  $(top_builddir)/src/common/libplatine_common.la \
  $(top_builddir)/src/common/libplatine_encap_plugin.la \
  -lpcap


# Target to test plugin architecture
check-plugins: test_plugins$(EXEEXT)	
	./test_plugins_icmp_28.sh
	./test_plugins_icmp_64.sh

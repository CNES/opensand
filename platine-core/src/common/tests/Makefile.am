CPPFLAGS_COMMON = -I.. -g -Wall

# test programs to build
check_PROGRAMS = \
  ip_addr \
  test_plugins

# test programs to run
TESTS = \
  ip_addr \
  test_plugins.sh

############## test for IP addresses ##############

ip_addr_SOURCES = \
  ../IpAddress.cpp \
  ../Ipv4Address.cpp \
  ../Ipv6Address.cpp \
  ip_addr.cpp

ip_addr_CXXFLAGS = $(CPPFLAGS_COMMON)
ip_addr_LDFLAGS =
ip_addr_LDADD = 


############## test for encap plugins ##############

INCLUDES = \
  -I..

test_plugins_SOURCES = \
  test_plugins.cpp

test_plugins_CXXFLAGS = $(CPPFLAGS_COMMON)
test_plugins_LDFLAGS =
test_plugins_LDADD = \
  ../libplatine_common.la \
  ../libplatine_encap_plugin.la \
  -lpcap



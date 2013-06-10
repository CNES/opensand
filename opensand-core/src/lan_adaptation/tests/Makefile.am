# test programs to build
check_PROGRAMS = \
  ip_addr

# test programs to run
TESTS = \
  ip_addr

############## test for IP addresses ##############

ip_addr_SOURCES = \
  $(top_srcdir)/src/common/IpAddress.cpp \
  $(top_srcdir)/src/lan_adaptation/Ipv4Address.cpp \
  $(top_srcdir)/src/lan_adaptation/Ipv6Address.cpp \
  ip_addr.cpp
  
INCLUDES = \
	-I$(top_srcdir)/src/lan_adaptation \
	-I$(top_srcdir)/src/common


ip_addr_CXXFLAGS = $(CPPFLAGS_COMMON)
ip_addr_LDFLAGS =
ip_addr_LDADD = 



CPPFLAGS_COMMON = -I.. -g -Wall
LDFLAGS_NETPACKET = -lpthread

# test programs to build
noinst_PROGRAMS = \
  ip_addr \
  net_packet

# test programs to run
TESTS = \
  ip_addr \
  net_packet

############## test for IP addresses ##############

ip_addr_SOURCES = \
  ../IpAddress.cpp \
  ../Ipv4Address.cpp \
  ../Ipv6Address.cpp \
  ip_addr.cpp

ip_addr_CXXFLAGS = $(CPPFLAGS_COMMON)
ip_addr_LDFLAGS =
ip_addr_LDADD = 

############## test for Network Packets ##############

net_packet_SOURCES =                      \
  ../Data.cpp \
  ../NetPacket.cpp \
  ../AtmCell.cpp \
  net_packet.cpp

net_packet_CXXFLAGS = $(CPPFLAGS_COMMON) $(CPPFLAGS_NETPACKET)
net_packet_LDFLAGS = $(LDFLAGS_NETPACKET)
net_packet_LDADD =


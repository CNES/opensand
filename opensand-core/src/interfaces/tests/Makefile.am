#SUBDIRS = libbridge 
# test programs to build
check_PROGRAMS = \
  test_tap

# test programs to run
TEST_TAP = \
  test_tap
  
EXTRA_DIST = \
	source.pcap 

test_tap_SOURCES = \
  test_tap_write.c

INCLUDES = \
	-I$(top_srcdir)/src/lan_adaptation
  
test_tap_LDFLAGS =
test_tap_LDADD = \
	$(top_builddir)/src/lan_adaptation/libopensand_lan_adaptation.la \
	-lpcap

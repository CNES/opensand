bin_PROGRAMS = test_no_interconnect \
			   test_interconnect_bottom \
			   test_interconnect_top

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/conf \
	-I$(top_srcdir)/src/dvb/utils

#test_interconnect_LDFLAGS = \
#	-lpthread \
#	-lrt

test_no_interconnect_SOURCES = \
	TestInterconnectBlock.cpp \
	TestInterconnectBlock.h \
	Config.h \
	test_no_interconnect.cpp

test_interconnect_bottom_SOURCES = \
	interconnect_channel_test.h \
	interconnect_channel_test.cpp \
    TestBlockInterconnectUpward.cpp \
	TestBlockInterconnectUpward.h \
	TestBlockInterconnectDownward.cpp \
	TestBlockInterconnectDownward.h \
	TestInterconnectBlock.cpp \
	TestInterconnectBlock.h \
	Config.h \
	test_interconnect_bottom.cpp

test_interconnect_top_SOURCES = \
	interconnect_channel_test.h \
	interconnect_channel_test.cpp \
    TestBlockInterconnectUpward.cpp \
	TestBlockInterconnectUpward.h \
	TestBlockInterconnectDownward.cpp \
	TestBlockInterconnectDownward.h \
	TestInterconnectBlock.cpp \
	TestInterconnectBlock.h \
	Config.h \
	test_interconnect_top.cpp
#test_interconnect_LDADD = \
#	$(top_builddir)/src/interconnect/libopensand_interconnect.la

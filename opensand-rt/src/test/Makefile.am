# test programs to build
check_PROGRAMS = \
  test_block \
  test_multi_blocks

# test programs to run
TESTS = \
  test.sh

LIBS_COMMON = \
	$(top_builddir)/src/libopensand_rt.la

test_block_CPPFLAGS = ${AM_CPPFLAGS} -I$(top_srcdir)/src/
test_block_SOURCES = \
	TestBlock.h \
	TestBlock.cpp
test_block_LDADD = $(LIBS_COMMON)

test_multi_blocks_CPPFLAGS = ${AM_CPPFLAGS} -I$(top_srcdir)/src/
test_multi_blocks_SOURCES = \
	TestMultiBlocks.h \
	TestMultiBlocks.cpp
test_multi_blocks_LDADD = $(LIBS_COMMON)

# we need .h here beacause it is opened in test
EXTRA_DIST = \
	TestMultiBlocks.h \
	test.sh

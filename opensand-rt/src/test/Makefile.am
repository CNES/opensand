# test programs to build
check_PROGRAMS = \
  test_block \
  test_multi_blocks

# test programs to run
TESTS = \
  test.sh

INCLUDES = -I$(top_srcdir)/src/
LIBS_COMMON = \
	$(top_builddir)/src/libopensand_rt.la

test_block_SOURCES = \
	TestBlock.h \
	TestBlock.cpp
test_block_LDADD = $(LIBS_COMMON)

test_multi_blocks_SOURCES = \
	TestMultiBlocks.h \
	TestMultiBlocks.cpp
test_multi_blocks_LDADD = $(LIBS_COMMON)

EXTRA_DIST = \
	test.sh

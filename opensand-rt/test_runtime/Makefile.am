################################################################################
#   Name       : Makefile
#   Author     : Cyrille Gaillardet <cyrille.gaillardet@toulouse.viveris.com>
#   Description: create the OpenSAND rt library
################################################################################

noinst_PROGRAMS = test_single_block test_3_blocks

INCLUDES = -I$(top_srcdir)/src

test_single_block_SOURCES = test_channels.cpp
test_single_block_LDADD = $(top_builddir)/src/libopensand_rt.la -lpthread /usr/lib/libtcmalloc_minimal.so

test_3_blocks_SOURCES = test_3_blocks.cpp
test_3_blocks_LDADD = $(top_builddir)/src/libopensand_rt.la -lpthread /usr/lib/libtcmalloc_minimal.so


EXTRA_DIST=readme.txt test_config_file.cfg


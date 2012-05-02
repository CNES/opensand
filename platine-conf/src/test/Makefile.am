################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the configuration library for Platine tools
################################################################################

TEST_SCRIPT = \
	test_configuration.sh
	
TESTS = \
	$(TEST_SCRIPT)

check_PROGRAMS = \
	test_configuration

AM_LDFLAGS = \
	-L$(top_builddir)/src/.libs 	

test_configuration_SOURCES = \
	test_configuration.cpp
test_configuration_LDADD = \
	-lplatine_conf

INCLUDES = \
	-I$(top_srcdir)/src/

EXTRA_DIST = \
	$(TEST_SCRIPT) \
	input	

################################################################################
#   Name       : Makefile
#   Author     : Julien BERNARD <julien.bernard@toulouse.viveris.com>
#   Description: create the configuration library for OpenSAND tools
################################################################################

TEST_SCRIPT = \
	test_configuration.sh
	
TESTS = \
	$(TEST_SCRIPT)

check_PROGRAMS = \
	test_configuration

test_configuration_SOURCES = \
	test_configuration.cpp

test_configuration_LDADD = \
	$(AM_LDADD) \
	-lopensand_old_conf 

test_configuration_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/

test_configuration_LDFLAGS = \
	$(AM_LDFLAGS) \
	-L$(top_builddir)/src/.libs 	


EXTRA_DIST = \
	$(TEST_SCRIPT) \
	input	

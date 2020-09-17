################################################################################
#   Name       : Makefile
#   Author     : Aurélien DELRIEU <aureleien.delrieu@viveris.fr
#   Description: create the configuration library for OpenSAND tools
################################################################################

TESTS = \
	test_configuration

check_PROGRAMS = \
	test_configuration

test_configuration_SOURCES = \
	test_configuration.cpp

test_configuration_LDADD = \
	$(AM_LDADD) \
	-lopensand_conf 

test_configuration_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/
#	-I$(top_srcdir)/include/

test_configuration_LDFLAGS = \
	$(AM_LDFLAGS) \
	-L$(top_builddir)/src/.libs

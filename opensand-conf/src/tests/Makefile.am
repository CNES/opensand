################################################################################
#   Name       : Makefile
#   Author     : Aurélien DELRIEU <aureleien.delrieu@viveris.fr
#   Description: create the configuration library for OpenSAND tools
################################################################################

TESTS = \
	test_configuration \
	test_configuration_ref \
	test_configuration_io

check_PROGRAMS = \
	test_configuration \
	test_configuration_ref \
	test_configuration_io

test_configuration_SOURCES = \
	test_configuration.cpp \
	catch.hpp

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

test_configuration_ref_SOURCES = \
	test_configuration_ref.cpp \
	catch.hpp

test_configuration_ref_LDADD = \
	$(AM_LDADD) \
	-lopensand_conf 

test_configuration_ref_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/
#	-I$(top_srcdir)/include/

test_configuration_ref_LDFLAGS = \
	$(AM_LDFLAGS) \
	-L$(top_builddir)/src/.libs

test_configuration_io_SOURCES = \
	test_configuration_io.cpp \
	catch.hpp

test_configuration_io_LDADD = \
	$(AM_LDADD) \
	-lopensand_conf 

test_configuration_io_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/
#	-I$(top_srcdir)/include/

test_configuration_io_LDFLAGS = \
	$(AM_LDFLAGS) \
	-L$(top_builddir)/src/.libs

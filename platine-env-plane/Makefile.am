################################################################################
#	Name       : Makefile
#	Author     : Didier Barvaux <didier.barvaux@toulouse.viveris.com>
#	Description: create the Platine environment plane library
################################################################################

SUBDIRS = \
	src

configure_files = \
	aclocal.m4 \
	config.h.in \
	configure \
	Makefile.am \
	configure.in \
	install-sh \
	mkinstalldirs \
	missing \
	depcomp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = platine_env_plane.pc

sysconf_DATA = conf/exec/* conf/stat/*

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

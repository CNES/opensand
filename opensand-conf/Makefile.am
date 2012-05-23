################################################################################
#	Name       : Makefile
#	Author     : Julien BERNARD <jbernard@toulouse.viveris.com>
#	Description: create the OpenSAND configuration library
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
pkgconfig_DATA = opensand_conf.pc

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

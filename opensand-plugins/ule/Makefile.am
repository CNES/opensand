################################################################################
#	Name       : Makefile
#	Author     : Julien BERNARD <jbernard@toulouse.viveris.com>
#	Description: create the OpenSAND ATM/AAL5 plugin library
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

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

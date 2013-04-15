################################################################################
#	Name       : Makefile
#	Author     : Cyrille Gaillardet <cyrille.gaillardet@toulouse.viveris.com>
#	Description: create the OpenSAND rt library
################################################################################

SUBDIRS = \
	src \
	test_runtime

dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	COPYING \
	COPYING.LESSER \
	INSTALL \
	NEWS \
	README
	
EXTRA_DIST = \
	autogen.sh \
	doc/doxygen


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_rt.pc

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

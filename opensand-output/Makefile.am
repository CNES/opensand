################################################################################
#	Name       : Makefile
#	Author     : Didier Barvaux <didier.barvaux@toulouse.viveris.com>
#	Description: create the OpenSAND output library
################################################################################

SUBDIRS = \
	src

dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	COPYING \
	INSTALL \
	NEWS \
	README
	
EXTRA_DIST = \
	autogen.sh \
	doc/doxygen


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_output.pc

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

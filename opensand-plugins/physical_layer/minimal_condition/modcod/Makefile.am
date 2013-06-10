################################################################################
#	Name       : Makefile
#	Author     : Julien BERNARD <jbernard@toulouse.viveris.com>
#	Description: create the OpenSAND modcod plugin library
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

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

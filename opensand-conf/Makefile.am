################################################################################
#	Name       : Makefile
#	Author     : Aurélien DELRIEU <aurelien.delrieu@viveris.fr>
#	Description: create the OpenSAND configuration library
################################################################################

ACLOCAL_AMFLAGS = -I m4

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
pkgconfig_DATA = opensand_conf.pc

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html
	$(RM) -r doc/latex

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

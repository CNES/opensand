################################################################################
#	Name       : Makefile
#	Author     : Julien BERNARD <jbernard@toulouse.viveris.com>
#	Description: create the OpenSAND GSE plugin library
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
	
DEBIAN = \
	debian/changelog \
	debian/control \
	debian/docs \
	debian/libopensand-gse-plugin.install \
	debian/libopensand-gse-plugin.postinst \
	debian/postinst \
	debian/preinst \
	debian/rules \
	debian/compat \
	debian/copyright \
	debian/libopensand-gse-plugin.dirs \
	debian/libopensand-gse-plugin-manager.dirs \
	debian/libopensand-gse-plugin.prerm \
	debian/postrm \
	debian/prerm

EXTRA_DIST = \
	doc/doxygen \
	conf/gse.conf \
	conf/gse.xsd \
	$(DEBIAN)

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

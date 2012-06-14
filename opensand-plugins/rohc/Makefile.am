################################################################################
#	Name       : Makefile
#	Author     : Julien BERNARD <jbernard@toulouse.viveris.com>
#	Description: create the OpenSAND ROHC plugin library
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
	debian/libopensand-rohc-plugin.install \
	debian/libopensand-rohc-plugin.postinst \
	debian/postinst \
	debian/preinst \
	debian/rules \
	debian/compat \
	debian/copyright \
	debian/libopensand-rohc-plugin.dirs \
	debian/libopensand-rohc-plugin-manager.dirs \
	debian/libopensand-rohc-plugin.prerm \
	debian/postrm \
	debian/prerm

EXTRA_DIST = \
	doc/doxygen \
	conf/rohc.conf \
	conf/rohc.xsd \
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

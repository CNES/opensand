################################################################################
#	Name       : Makefile
#	Author     : Julien BERNARD <jbernard@toulouse.viveris.com>
#	Description: create the OpenSAND ATM/AAL5 plugin library
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
	debian/libopensand-mpeg-plugin.install \
	debian/libopensand-mpeg-plugin.postinst \
	debian/postinst \
	debian/preinst \
	debian/rules \
	debian/compat \
	debian/copyright \
	debian/libopensand-mpeg-plugin.dirs \
	debian/libopensand-mpeg-plugin-manager.dirs \
	debian/libopensand-mpeg-plugin.prerm \
	debian/postrm \
	debian/prerm
	
EXTRA_DIST = \
	doc/doxygen \
	conf/mpeg.conf \
	conf/mpeg.xsd \
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

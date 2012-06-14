################################################################################
#	Name       : Makefile
#	Author     : Julien BERNARD <jbernard@toulouse.viveris.com>
#	Description: create the OpenSAND configuration library
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
	debian/compat \
	debian/control \
	debian/copyright \
	debian/docs \
	debian/libopensand-conf-dev.dirs \
	debian/libopensand-conf-dev.install \
	debian/libopensand-conf.dirs \
	debian/libopensand-conf.install \
	debian/postinst \
	debian/postrm \
	debian/preinst \
	debian/prerm \
	debian/rules
	
EXTRA_DIST = \
	doc/doxygen \
	$(DEBIAN)

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

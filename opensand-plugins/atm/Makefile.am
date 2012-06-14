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
	debian/libopensand-atm-aal5-plugin.install \
	debian/libopensand-atm-aal5-plugin.postinst \
	debian/postinst \
	debian/preinst \
	debian/rules \
	debian/compat \
	debian/copyright \
	debian/libopensand-atm-aal5-plugin.dirs \
	debian/libopensand-atm-aal5-plugin-manager.dirs \
	debian/libopensand-atm-aal5-plugin.prerm \
	debian/postrm \
	debian/prerm

EXTRA_DIST = \
	doc/doxygen \
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

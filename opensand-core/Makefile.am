################################################################################
#	Name       : Makefile
#	Author     : Julien Bernard <julien.bernard@toulouse.viveris.fr>
#	Description: build the OpenSAND platform
################################################################################

SUBDIRS= \
	src \
	conf

dist_doc_DATA = \
	README \
	INSTALL \
	COPYING \
	COPYING.LESSER \
	AUTHORS \
	CHANGES \
	ChangeLog

DEBIAN = \
	debian/changelog \
	debian/copyright \
	debian/libopensand-encap-plugin-dev.install \
	debian/opensand-core-all.dirs \
	debian/opensand-core-all.postrm \
	debian/opensand-core-bin.dirs \
	debian/opensand-core-bin.postrm \
	debian/rules \
	debian/compat \
	debian/docs \
	debian/libopensand-encap-plugin.dirs \
	debian/opensand-core-all.install \
	debian/opensand-core-all.preinst \
	debian/opensand-core-bin.install \
	debian/opensand-core-bin.preinst \
	debian/control \
	debian/libopensand-encap-plugin-dev.dirs \
	debian/libopensand-encap-plugin.install \
	debian/opensand-core-all.postinst \
	debian/opensand-core-all.prerm \
	debian/opensand-core-bin.postinst \
	debian/opensand-core-bin.prerm

EXTRA_DIST = \
	$(DEBIAN)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_encap_plugin.pc

.PHONY: doc

doc:
	doxygen $(opensand_doc_dir)/doxygen

clean-doc:
	$(RM) -r $(opensand_doc_dir)/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

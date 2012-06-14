SUBDIRS=src test_runtime doc_src


dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	COPYING \
	INSTALL \
	LICENSE.LGPL \
	NEWS \
	README \
	readme.txt
	
DEBIAN = \
	debian/changelog \
	debian/control \
	debian/docs \
	debian/libopensand-margouilla-dev.install \
	debian/libopensand-margouilla.install \
	debian/postrm \
	debian/prerm \
	debian/compat \
	debian/copyright \
	debian/libopensand-margouilla-dev.dirs \
	debian/libopensand-margouilla.dirs \
	debian/postinst \
	debian/preinst \
	debian/rules

EXTRA_DIST = \
	doc/readme.txt \
	$(DEBIAN)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_margouilla.pc

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

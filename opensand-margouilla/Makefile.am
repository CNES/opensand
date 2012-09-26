SUBDIRS=src test_runtime doc_src


dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	COPYING.LGPL \
	INSTALL \
	NEWS \
	README
	
EXTRA_DIST = \
	autogen.sh \
	doc/readme.txt

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_margouilla.pc

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

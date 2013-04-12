#SUBDIRS=src test_runtime doc_src
SUBDIRS=src test_runtime

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
pkgconfig_DATA = opensand_rt.pc

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

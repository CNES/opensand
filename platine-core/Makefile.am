SUBDIRS= \
	src \
	conf

readme_files = readme.txt 

configure_files = \
	aclocal.m4 \
	config.h.in \
	configure \
	Makefile.am \
	configure.in \
	install-sh \
	mkinstalldirs \
	missing \
	depcomp

EXTRA_DIST= $(readme_files) $(configure_files)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = platine_encap_plugin.pc

.PHONY: doc

doc:
	doxygen $(platine_doc_dir)/doxygen

clean-doc: 
	rm -rf $(platine_doc_dir)/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

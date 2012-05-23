SUBDIRS=src test_runtime doc_src

readme_files = \
	readme.txt \
	doc/readme.txt

EXTRA_DIST= $(readme_files)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_margouilla.pc

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

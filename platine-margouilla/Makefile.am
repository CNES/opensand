SUBDIRS=src test_runtime


readme_files = \
readme.txt \
doc/readme.txt


configure_files = \
aclocal.m4 \
config.h.in \
configure \
makefile.am \
configure.in \
install-sh \
mkinstalldirs \
missing \
depcomp


EXTRA_DIST= $(readme_files) $(configure_files)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = platine_margouilla.pc

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

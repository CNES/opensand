################################################################################
#	Name       : Makefile
#	Author     : Julien Bernard <julien.bernard@toulouse.viveris.fr>
#	Description: build the OpenSAND platform
################################################################################

SUBDIRS= \
	src

dist_doc_DATA = \
	README \
	INSTALL \
	COPYING \
	COPYING.LESSER \
	AUTHORS \
	CHANGES \
	ChangeLog

EXTRA_DIST = \
	autogen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_plugin.pc

.PHONY: doc

doc:
	doxygen $(opensand_doc_dir)/doxygen

clean-doc:
	$(RM) -r $(opensand_doc_dir)/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

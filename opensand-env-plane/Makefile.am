################################################################################
#	Name       : Makefile
#	Author     : Didier Barvaux <didier.barvaux@toulouse.viveris.com>
#	Description: create the OpenSAND environment plane library
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

EXTRA_DIST = \
	doc/doxygen \
	TODO

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opensand_env_plane.pc

exec_data = \
	conf/exec/com_parameters.conf \
	conf/exec/error_def.conf \
	conf/exec/event_def.conf \
	conf/exec/stat_def_gw.conf \
	conf/exec/stat_def_sat.conf \
	conf/exec/stat_def_st.conf

stat_data = \
	conf/stat/event_conf.conf \
	conf/stat/stat_conf_gw.conf \
	conf/stat/stat_conf_sat.conf \
	conf/stat/stat_conf_st.conf \
	conf/stat/trace_param.conf

dist_sysconf_DATA = $(exec_data) $(stat_data)

.PHONY: doc

doc:
	doxygen doc/doxygen

clean-doc:
	$(RM) -r doc/html

deb:
	dpkg-buildpackage

clean-deb:
	dh_clean

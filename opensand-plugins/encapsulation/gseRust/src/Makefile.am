################################################################################
#   Name       : Makefile
#   Author     : Axel PINEL <axel.pinel@viveris.fr>
#   Description: create the GSE RUST encapsulation plugin for OpenSAND
################################################################################

SUBDIRS = 

plugins_LTLIBRARIES = libopensand_gseRust_encap_plugin.la

libopensand_gseRust_encap_plugin_la_cpp = \
	GseIdentifier.cpp \
	GseRust.cpp \
	SimpleGseRust.cpp \
	Memory.cpp \
	GseRustCApi.cpp

libopensand_gseRust_encap_plugin_la_h = \
	GseRust.h \
	GseIdentifier.h \
	SimpleGseRust.h \
	Memory.h \
	GseRustCApi.h

libopensand_gseRust_encap_plugin_la_SOURCES = \
	$(libopensand_gseRust_encap_plugin_la_cpp) \
	$(libopensand_gseRust_encap_plugin_la_h)

libopensand_gseRust_encap_plugin_la_LIBADD = libgse_rust_c_api.a

pluginsdir = $(libdir)/opensand/plugins

libgse_rust_c_api.a:
	cd $(srcdir)/gse_c_rust_api; cargo build --release; cp target/release/$@ $(abs_builddir)

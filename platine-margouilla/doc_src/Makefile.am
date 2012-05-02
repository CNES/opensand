#bin_SCRIPTS = generate_html generate_api_html

api_dox_src = \
	api_doc.dox \
	api_header.htm

web_html_src = \
	content_header_page.htm \
	content_about.htm \
	content_components.htm \
	content_documentation.htm \
	content_design_runtime.htm \
	content_tutorial_new_bloc.htm \
	content_exemple_msg_bloc.htm \
	content_download.htm \
	content_mailing_list.htm \
	content_contact.htm \
	content_license.htm \
	content_bas_page.htm \
	index.php \
	toc.php \
	generic_page.php \
	generic_page \
	core.php \
	content_template.htm \
	generate_htm_api_from_src_marg.bat \
	generate_htm_from_php_marg.bat \
	schemas.sxw

icon_html_menu = \
	icon/small_tri_right.gif  \
	icon/tube-home-over.gif \
	icon/tube-subbutton-marker.gif \
	icon/tube-mail.gif \
	icon/tube-subbutton-over.gif \
	icon/tube-mail-over.gif \
	icon/tube-subsubbutton-marker.gif \
	icon/pad.gif \
	icon/tube-button.gif \
	icon/tube-next.gif \
	icon/tube-subsubbutton-over.gif \
	icon/tube-button-marker.gif \
	icon/tube-next-over.gif \
	icon/tube-top.gif \
	icon/tube-button-over.gif \
	icon/tube-prev.gif \
	icon/tube-up.gif \
	icon/tube-elem.gif \
	icon/tube-prev-over.gif \
	icon/tube-up-over.gif \
	icon/tube-home.gif \
	icon/tube-subbutton.gif

icon_html_menu_text = \
	icon/titre_margouilla.png \
	icon/logo_bleusimple_small2.jpg \
	icon/about_margouilla_text.png \
	icon/mailing_list_text.png \
	icon/components_text.png \
	icon/the_gimp_text.gif \
	icon/contact_text.png \
	icon/margouilla_text.png \
	icon/documentation_text.png \
	icon/download_text.png \
	icon/runtime_design_text.png \
	icon/exemple_msg_bloc_text.png \
	icon/license_text.png \
	icon/tutorial_new_bloc_text.png

icon_html_draw = \
	icon/schemas_ditributed.png \
	icon/schemas_remote_debug.png \
	icon/schemas_runtime.png

icon_html = $(icon_html_menu) $(icon_html_menu_text) $(icon_html_draw)


EXTRA_DIST = $(web_html_src) $(icon_html) $(api_dox_src)

DOCDIR = $(top_srcdir)/doc


generate_html : $(web_html_src)
	@if [ ! -d $(DOCDIR) ] ; then mkdir -p $(DOCDIR) ; fi
	./generic_page content_about.htm >$(DOCDIR)/about.htm
	./generic_page content_components.htm >$(DOCDIR)/components.htm
	./generic_page content_documentation.htm >$(DOCDIR)/documentation.htm
	./generic_page content_design_runtime.htm >$(DOCDIR)/design_runtime.htm
	./generic_page content_tutorial_new_bloc.htm >$(DOCDIR)/tutorial_new_bloc.htm
	./generic_page content_exemple_msg_bloc.htm >$(DOCDIR)/exemple_msg_bloc.htm
	./generic_page content_download.htm >$(DOCDIR)/download.htm
	./generic_page content_mailing_list.htm >$(DOCDIR)/mailing_list.htm
	./generic_page content_license.htm >$(DOCDIR)/license.htm
	./generic_page content_contact.htm >$(DOCDIR)/contact.htm
	cp $(DOCDIR)/about.htm $(DOCDIR)/index.htm
	cp $(DOCDIR)/about.htm $(DOCDIR)/index.html
	cp index.php $(DOCDIR)/index.php
	@if [ ! -d $(DOCDIR)/icon ] ; then mkdir -p $(DOCDIR)/icon ; fi
	cp icon/* $(DOCDIR)/icon 

 
generate_api_html : $(api_dox_src)
	@if [ ! -d $(DOCDIR) ] ; then mkdir -p $(DOCDIR) ; fi
	@if [ ! -d $(DOCDIR)/api ] ; then mkdir -p $(DOCDIR)/api ; fi
	@if [ ! -d $(DOCDIR)/api/htm ] ; then mkdir -p $(DOCDIR)/api/htm ; fi
	@if [ ! -d $(DOCDIR)/api/htm/icon ] ; then mkdir -p $(DOCDIR)/api/htm/icon ; fi
	doxygen api_doc.dox 
	cp icon/titre_margouilla.png $(DOCDIR)/api/htm/icon



#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 TAS
#
#
# This file is part of the OpenSAND testbed.
#
#
# OpenSAND is free software : you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see http://www.gnu.org/licenses/.
# 
#


"""
st_assignment_dialog.py - ST assignmentconfiguration dialog
"""

import gtk
import gobject

from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_core.utils import get_conf_xpath, CATEGORY, \
        ST, TAL_ID, TAL_DEF_AFF, TAL_AFFECTATIONS, CARRIERS_DISTRIB
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup


class AssignmentDialog(WindowView):
    """ an modcod configuration window """
    def __init__(self, model, spot, gw, manager_log, update_cb, link):
        
        WindowView.__init__(self, None, 'edit_dialog')
        self._model = model
        self._spot = spot
        self._gw = gw 
        self._update_cb = update_cb
        self._link = link

        self._dlg = self._ui.get_widget('edit_dialog')
        self._dlg.set_keep_above(False)
        
        self._vbox = self._ui.get_widget('edit_dialog_vbox')
        self.edit_text_win = self._ui.get_widget('edit_text_win')
        
        #Add vbox_assignement Box
        self.vbox_assignement = gtk.VBox()
        self.edit_text_win.add_with_viewport(self.vbox_assignement)
        
        #Add hbox_assignment_title in vbox_assignement
        self.hbox_assignment_title=gtk.HBox()
        self.vbox_assignement.pack_start(self.hbox_assignment_title, expand=False)
        #Add scroll_st_allocation in vbox_assignement
        self.scroll_st_allocation=gtk.ScrolledWindow()
        self.vbox_assignement.pack_start(self.scroll_st_allocation)
        
        #Add vbox_st_allocation in scroll_st_allocation
        self.vbox_st_allocation = gtk.VBox()
        self.scroll_st_allocation.add_with_viewport(self.vbox_st_allocation)
        
        #Add title inn hbox_assignment_title
        self.st_title=gtk.Label(str="ST")
        self.group_title=gtk.Label(str="Group")
        
        self.hbox_assignment_title.pack_start(self.st_title)
        self.hbox_assignment_title.pack_start(self.group_title)
        
        #Size and display
        self._dlg.set_default_size(350, 300)
        
        
    ##################################################
        
    def go(self):
        """ run the window """
        
        self._vbox.show_all()
        
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
        self._dlg.run()
    
    ##################################################
        
    def load(self):
        """ load the hosts assignment """
        def select_group(combo_box, group):
            i = 0
            for gp in combo_box.get_model():
                if gp[1] == group:
                    combo_box.set_active(i)
                    return
                i = i + 1
                
        #Load the configuration file
        config=self._model.get_conf().get_configuration()
        #Get ST list from opensand
        host_list = self._model.get_hosts_list()  
        st_list = []

        #load the st assignment
        xpath = get_conf_xpath(TAL_AFFECTATIONS, self._link, 
                               self._spot, self._gw)
        defaulf_grp_path = get_conf_xpath(TAL_DEF_AFF, self._link,
                                          self._spot, self._gw)
        defaulf_grp = config.get_value(config.get(defaulf_grp_path))
        for st in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(st)
            st_list.append([content[TAL_ID],content[CATEGORY]])
            
        #Create combo box for group
        group = gtk.ListStore(int, str)
        group_list = []
        xpath = get_conf_xpath(CARRIERS_DISTRIB, self._link, self._spot, self._gw)
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            if ([self.get_group_value(content[CATEGORY]),                             
                 content[CATEGORY]]) not in group_list:
                group_list.append([self.get_group_value(content[CATEGORY]),
                                   content[CATEGORY]])
                group.append([self.get_group_value(content[CATEGORY]), 
                              content[CATEGORY]])
        for host in host_list:
            if not host.get_name().lower().startswith(ST):
                continue
            if host.get_spot_id() != self._spot:
                continue
            if host.get_gw_id() != self._gw:
                continue
            #Create box for st
            hbox_st_allocation = gtk.HBox()
            #Label for st
            label_st_name = gtk.Label(str=host.get_name().upper())
            #Create combo box for st
            combo_box_group = gtk.ComboBox(group)
            renderer_text = gtk.CellRendererText()
            combo_box_group.pack_start(renderer_text, True)
            combo_box_group.add_attribute(renderer_text, "text", 1)
            combo_box_group.set_active(0)
            select_group(combo_box_group, defaulf_grp)
            for st in st_list:
                if host.get_name().lower() == ST + str(st[0]):
                    select_group(combo_box_group, st[1])
            #Add all in the window
            hbox_st_allocation.pack_start(label_st_name)
            hbox_st_allocation.pack_start(combo_box_group)
            self.vbox_st_allocation.pack_start(hbox_st_allocation, fill=False, expand=False)
        self.edit_text_win.show_all()

    ##################################################
            
    def get_group_value(self, name):
        """Convert old group name to value"""
        config = self._model.get_conf().get_configuration()
        category_type = config.get_simple_type("Category")
        if category_type is None or category_type["enum"] is None:
            return 0

        i = 0
        for cat in category_type["enum"]:
            if name.startswith(cat):
                return i
            i = i + 1
        return 0

    ##################################################

    def get_group_str(self, value):
        """Convert value to group name """
        config = self._model.get_conf().get_configuration()
        category_type = config.get_simple_type("Category")
        if category_type is None or category_type["enum"] is None:
            return ""
        
        if len(category_type["enum"]) <= value:
            value = 0
        return category_type["enum"][value]
            
    ##################################################
    
    def get_id_st(self, st):
        """Return the id of ST """
        ret = st.upper().split("T")
        return ret[1]
    
    ##################################################
        
    def on_save_edit_clicked(self, source=None):
        """Save the ST assignment"""
        st_line = []
        st_list = self.get_st_list()
        
        #Get the xml to save
        config=self._model.get_conf().get_configuration()
        xpath = get_conf_xpath(TAL_AFFECTATIONS, self._link, 
                               self._spot, self._gw)
        table = config.get(xpath)
        
        #Get the number of ST in xml
        for element in config.get_table_elements(table):
            st_line.append('ST' + element.get(TAL_ID))
       
        #Place the number of st in xml
        for st in st_list:
            if st[0] not in st_line:
                config.add_line(config.get_path(table))
                st_line.append(st[0])
        table = config.get(xpath)
        
        #Save all st assigment
        for st in st_list:
            if st[0] in st_line:
                line_id = st_line.index(st[0])
                config.set_value(self.get_id_st(st[0]),
                        config.get_path(config.get_table_elements(table)[line_id]),
                        TAL_ID)
                config.set_value(self.get_group_str(st[1][0]),
                        config.get_path(config.get_table_elements(table)[line_id]),
                        CATEGORY)

        config.write()
        gobject.idle_add(self._update_cb)
        self._dlg.destroy()
            
    ##################################################
    
    def get_st_list(self):
        """retourn all the config to save """
        st_list = []
        child = self.vbox_st_allocation.get_children()
        for st in child:
            box = st.get_children()
            couple = []
            for element in box:
                try:
                    couple.append(element.get_text())
                except:
                    couple.append(element.get_model()[element.get_active()])
            st_list.append(couple)
        return st_list
                
    ##################################################
    
    def on_edit_dialog_delete_event(self, source=None, event=None):
        """ close and delete the window """
        self._dlg.destroy()
    
    ##################################################
        
    def on_cancel_edit_clicked(self, source=None):
        """ close and delete the window """
        gobject.idle_add(self._update_cb)
        self._dlg.destroy()
        

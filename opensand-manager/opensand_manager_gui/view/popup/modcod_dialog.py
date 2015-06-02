#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
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

# Author : Maxime POMPA 


"""
modcod_dialog.py - MODCOD configuration dialog
"""

import gtk

from matplotlib.figure import Figure
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigureCanvas


from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_core.utils import FORWARD_DOWN, RETURN_UP
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup



class ModcodParameter(WindowView):
    """ an modcod configuration window """
    def __init__(self, model, manager_log, link, carrier_id, parent):
        
        WindowView.__init__(self, None, 'edit_dialog')

        self._dlg = self._ui.get_widget('edit_dialog')
        self._dlg.set_keep_above(True)
        
        self._vbox = self._ui.get_widget('dialog-vbox7')
        self._edit_text_win = self._ui.get_widget('edit_text_win')
        self._carrier_id = carrier_id
        self._parent = parent
        self._link = link
        
        #Add vbox_conf Box
        self._vbox_conf=gtk.VBox()
        self._edit_text_win.add_with_viewport(self._vbox_conf)
        
        #Add hbox_mod in vbox_conf
        self._hbox_modcod=gtk.HBox()
        self._vbox_conf.pack_start(self._hbox_modcod)
        #Add vbox_vcm_option in vbox_conf
        self._vbox_vcm_option=gtk.VBox()
        self._vbox_conf.pack_start(self._vbox_vcm_option, expand=False)
        
        #Add frame_access_type in hbox_modcod
        self._frame_access_type=gtk.Frame(label="Access Type")
        self._frame_access_type.set_shadow_type(gtk.SHADOW_OUT)
        self._hbox_modcod.pack_start(self._frame_access_type,padding = 10)
        #Add frame_modcod in hbox_modcod
        self._frame_modcod=gtk.Frame(label="MODCOD")
        self._frame_modcod.set_shadow_type(gtk.SHADOW_OUT)
        self._hbox_modcod.pack_start(self._frame_modcod, padding = 25)
        
        #Add vbox_access_type in frame_access_type
        self._vbox_access_type=gtk.VBox()
        self._frame_access_type.add(self._vbox_access_type)
        #Add modcod_scroll_window in frame_modcod
        self._modcod_scroll_window=gtk.ScrolledWindow()
        self._modcod_scroll_window.set_size_request(100, 200)
        self._frame_modcod.add(self._modcod_scroll_window)
        #Add vbox_modcod in modcod_scroll_window
        self._vbox_modcod=gtk.VBox()
        self._modcod_scroll_window.add_with_viewport(self._vbox_modcod)
        
        #Create Frame_temporal_graphe
        self._frame_temporal_graphe=gtk.Frame(label="Temporal Representation")
        
        self._figure = Figure()
        self._ax = self._figure.add_subplot(111)
        canvas = FigureCanvas(self._figure)
        canvas.set_size_request(150,150)
        self._frame_temporal_graphe.add(canvas)
        
        #Create scroll_ratio
        self._scroll_ratio = gtk.ScrolledWindow()
        self._scroll_ratio.set_size_request(150,120)
        
        #Add vbox_ratio in scroll_ratio
        self._vbox_ratio = gtk.VBox()
        self._scroll_ratio.add_with_viewport(self._vbox_ratio)
        
        
        #Size and display
        self._dlg.set_default_size(500, 300)
        
        #MODCOD list from file definition.txt
        if self._link == FORWARD_DOWN:
            path="/usr/share/opensand/modcod/forward/definition.txt"
        elif self._link == RETURN_UP:
            encap = model.get_conf().get_return_up_encap()
            if encap['0'] == 'AAL5/ATM':
                path="/usr/share/opensand/modcod/return/definition_ATM.txt"
            else:
                path = "/usr/share/opensand/modcod/return/definition_MPEG.txt"
        self._modcod_list = self.load_modcod(path)
        #Item List to simply access to toogle button
        self._item_list = []
        self._list_modcod_ratio = {}
        self._dico_modcod = {}

        self._vcm_radio = None
        
    ##################################################
        
    def go(self):
        """ run the window """
        
        self._vbox.show_all()
        
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
        self._dlg.set_title("ModCod parameters - OpenSAND Manager")
        self._dlg.run()
        
    ##################################################
        
    def load(self):
        
        #Add the access type button for forward or return
        if self._link == FORWARD_DOWN:
            #Add radio button
            self._ccm_radio = gtk.RadioButton(group=None, label="CCM", 
                                            use_underline=True)
            self._acm_radio = gtk.RadioButton(group=self._ccm_radio, 
                                            label="ACM", 
                                            use_underline=True)
            self._vcm_radio = gtk.RadioButton(group=self._ccm_radio, 
                                            label="VCM", 
                                            use_underline=True)
            #Connect signal to button
            self._ccm_radio.connect("toggled", self.on_ccm_toggled)
            self._acm_radio.connect("toggled", self.on_acm_toggled)
            self._vcm_radio.connect("toggled", self.on_vcm_toggled)
            #Add to box
            self._vbox_access_type.pack_start(self._ccm_radio, 
                                              expand=True, fill=True)
            self._vbox_access_type.pack_start(self._acm_radio, 
                                              expand=True, fill=True)
            self._vbox_access_type.pack_start(self._vcm_radio, 
                                              expand=True, fill=True)
        elif self._link == RETURN_UP:
            self._dama_radio=gtk.RadioButton(group=None, label="DAMA", 
                                             use_underline=True)
            self._aloha_radio=gtk.RadioButton(group=self._dama_radio,
                                    label="ALOHA", use_underline=True)
            self._scpc_radio=gtk.RadioButton(group=self._dama_radio,
                                    label="SCPC", use_underline=True)
            self._dama_radio.connect("toggled", self.on_toggled)
            self._aloha_radio.connect("toggled", self.on_toggled)
            self._scpc_radio.connect("toggled", self.on_toggled)
            self._vbox_access_type.pack_start(self._dama_radio, 
                                              expand=True, fill=True)
            self._vbox_access_type.pack_start(self._aloha_radio, 
                                              expand=True, fill=True)
            self._vbox_access_type.pack_start(self._scpc_radio, 
                                              expand=True, fill=True)
            
        #Load access type from the carrier
        access_type = self._parent.get_list_carrier()[self._carrier_id-1].getAccessType()
        if access_type == "CCM":
            self._ccm_radio.toggled()
        elif access_type == "ACM":
            self._acm_radio.set_active(True)
        elif access_type == "VCM":
            self._vcm_radio.set_active(True)
        elif access_type == "DAMA":
            self._dama_radio.toggled()
        elif access_type == "ALOHA":
            self._aloha_radio.set_active(True)
        elif access_type == "SCPC":
            self._scpc_radio.set_active(True)
        
        #Load modcod from carrier
        modcod_list = self._parent.get_list_carrier()[self._carrier_id-1].getModCod()
        list_ratio = self._parent.get_list_carrier()[self._carrier_id-1].getRatio()
        index = 0
        for modcod in modcod_list:
            self._list_modcod_ratio[modcod] = list_ratio[index]
            if len(modcod_list) == len(list_ratio):
                index += 1
            self._item_list[modcod-1].set_active(True)
        
        self._vbox_conf.show_all()
    ##################################################

    def close(self):
        """ close the window """
        self._dlg.destroy()
        
    ##################################################
    
    def on_edit_dialog_delete_event(self, source=None, event=None):
        """ close and delete the window """
        self.close()
    
    ##################################################

    def add_modcod_item(self, source=None):
        """display all the modcod button"""
        
        #remove the old list
        for child in self._vbox_modcod.get_children():
            self._vbox_modcod.remove(child)
        #Add the new list in the vbox
        for element in self._item_list:
            self._vbox_modcod.pack_start(element, expand=False, fill=False)
            
        self._vbox.show_all()
        
    ##################################################
    
    def on_ccm_toggled(self, source=None):
        """Signal when ccm button change"""
        """ With the list of modcod it creates a list of widget"""
        self._item_list=[]
        #Create tooltips for button
        tooltip = gtk.Tooltips()
        #If button become enabled
        if source.get_active():
            for modcod in self._modcod_list:
                #In radio button the first radio have no group
                if modcod == self._modcod_list[0]:
                    check_modcod = gtk.RadioButton(group = None,
                                                   label = modcod[1] + " " + modcod[2],
                                                   use_underline=True)
                    check_modcod.connect("toggled", self.on_check_modcod, modcod)
                    #Add tooltip
                    tooltip.set_tip(check_modcod, "Spectral_efficiency : "
                                    + modcod[3]+"\nRequired Es/N0 : " + modcod[4])
                    #Default value for ratio is 10
                    if modcod in self._list_modcod_ratio.keys():
                        self._dico_modcod[modcod[1] + " " + modcod[2]] = \
                            self._list_modcod_ratio[modcod]            
                    else:
                        self._dico_modcod[modcod[1] + " " + modcod[2]] = 10            
                else:
                    check_modcod = gtk.RadioButton(group = self._item_list[0],
                                                   label = modcod[1] + " " + modcod[2],
                                                   use_underline = True)
                    tooltip.set_tip(check_modcod, "Spectral_efficiency : "
                                    + modcod[3] + "\nRequired Es/N0 : " + modcod[4])
                    check_modcod.connect("toggled", self.on_check_modcod, modcod)
                self._item_list.append(check_modcod)
            #Add all the widget in the window
            self.add_modcod_item()
        #If button become disable
        else:
            self._dico_modcod.clear()
        
    ##################################################
    
    def on_acm_toggled(self, source=None):
        """Signal when acm button change"""
        """ With the list of modcod it creates a list of widget"""
        self._item_list = []
        tooltip = gtk.Tooltips()
        if source.get_active():
            for modcod in self._modcod_list:
                check_modcod = gtk.CheckButton(label=modcod[1] + " " + modcod[2],
                                               use_underline = True)
                check_modcod.connect("toggled", self.on_check_modcod, modcod)
                tooltip.set_tip(check_modcod, "Spectral_efficiency : "
                                + modcod[3] + "\nRequired Es/N0 : " + modcod[4])
                self._item_list.append(check_modcod)
            self.add_modcod_item()
        else:
            self._dico_modcod.clear()
    
    ##################################################
    
    def on_vcm_toggled(self, source=None):
        """Signal when vcm button change"""
        """ With the list of modcod it creates a list of widget"""
        tooltip = gtk.Tooltips()
        self._item_list = []
        #Check if the toogle button is active
        if source.get_active():
            #Create check button for modcod
            for modcod in self._modcod_list:
                check_modcod = gtk.CheckButton(label = modcod[1] + " " + modcod[2],
                                               use_underline = True)
                check_modcod.connect("toggled", self.on_check_modcod, modcod)
                tooltip.set_tip(check_modcod, "Spectral_efficiency : "
                                + modcod[3] + "\nRequired Es/N0 : " + modcod[4])
                self._item_list.append(check_modcod)
            
            #Add the temporal representation graphe to the window
            self.trace_temporal_representation()
            #Refresh the ratio selection interface
            self.create_menu_ratio()
            #Extend the window size
            self._dlg.resize(500, 565)
            self.add_modcod_item()
        #Remove graphe and ratio part if we leave vcm mode
        else:
            self._dlg.resize(500, 300)
            self._dico_modcod.clear()
            for child in self._vbox_vcm_option:
                self._vbox_vcm_option.remove(child)
            
    ##################################################
    
    def on_toggled(self, source=None):
        self._item_list=[]
        tooltip = gtk.Tooltips()
        #If button become enabled
        if source.get_active():
            for modcod in self._modcod_list:
                #In radio button the first radio have no group
                if modcod == self._modcod_list[0]:
                    check_modcod = gtk.RadioButton(group = None, 
                                                   label = modcod[1] + " " + modcod[2],
                                                   use_underline = True)
                    tooltip.set_tip(check_modcod, "Spectral_efficiency : "
                                    + modcod[3] + "\nRequired Es/N0 : " + modcod[4])
                    check_modcod.connect("toggled", self.on_check_modcod, modcod)
                    #Default value for ratio is 10
                    if modcod in self._list_modcod_ratio.keys():
                        self._dico_modcod[modcod[1] + " " + modcod[2]] = \
                                self._list_modcod_ratio[modcod]            
                    else:
                        self._dico_modcod[modcod[1] + " " + modcod[2]] = 10            
                else:
                    check_modcod = gtk.RadioButton(group=self._item_list[0],
                                                   label = modcod[1] + " " + modcod[2], 
                                                   use_underline=True)
                    check_modcod.connect("toggled", self.on_check_modcod, modcod)
                    tooltip.set_tip(check_modcod, "Spectral_efficiency : "
                                    + modcod[3]+"\nRequired Es/N0 : " + modcod[4])
                self._item_list.append(check_modcod)
            #Add all the widget in the window
            self.add_modcod_item()
        #If button become disable
        else:
            self._dico_modcod.clear()
            
    ##################################################
        
    def on_check_modcod(self, source=None, modcod=0):
        """Signal if it is selected"""
        #If modcod become enable
        if source.get_active():
            #Add this modcod in the dictionary with the default ration value 10
            if int(modcod[0]) in self._list_modcod_ratio.keys():
                self._dico_modcod[source.get_label()] = \
                        self._list_modcod_ratio[int(modcod[0])]            
            else:
                self._dico_modcod[source.get_label()] = 10
        #if modcod become disable
        else:
            del self._dico_modcod[source.get_label()]
        #refresh the ratio selection menu 
        self.create_menu_ratio()
        #Trace the graphic
        self.trace_arrow()
        
    ##################################################
    
    def on_update_ratio(self, source=None, modcod=None):
        """Signal when a ratio change"""
        self._dico_modcod[modcod] = int(source.get_value())
        self.trace_arrow()
        
    ##################################################
        
    def load_modcod(self, path):
        """Read in the file the available modcod"""
        mc_list = []
        with  open(path, 'r') as modcod_def:
            for line in modcod_def:
                if (line.startswith("/*") or 
                    line.isspace() or
                    line.startswith('nb_fmt')):
                    continue
                elts = line.split()
                if len(elts) != 5:
                    continue
                if not elts[0].isdigit:
                    continue
                # id, modulation, coding_rate, spectral_efficiency, required Es/N0
                mc_list.append([elts[0], elts[1], elts[2], elts[3], elts[4]])
        
        return mc_list
        
    ##################################################
    
    def trace_temporal_representation(self):
        """Add graphe and ration selection in the window """
        
        self._vbox_vcm_option.pack_start(self._frame_temporal_graphe)
        self._vbox_vcm_option.pack_start(self._scroll_ratio)
        #Clear the graph
        self._ax.cla()
        self._figure.canvas.draw()
        #Display all
        self._vbox.show_all()    
        
    ##################################################
        
    def trace_arrow(self):
        """Calculate and display the representation with dico_modcod """
        ratio = 0                        
        d = 0
        
        #Clear graph
        self._ax.cla()
        self._figure.canvas.draw()
        
        #Calcul total ratio
        for value in self._dico_modcod.values():
            ratio = ratio + value
        
        for modcod, value in self._dico_modcod.items():
            t = float(value) / ratio
            self._ax.annotate('', 
                              xy = (d, 0.50), 
                              xycoords = 'data', 
                              xytext = (d+t, 0.50),
                              textcoords = 'data', 
                              arrowprops = {'arrowstyle':'<->'})
            self._ax.annotate(modcod, 
                              xy = (d, 0.25), 
                              xycoords = 'data', 
                              xytext = (d+t, 0.25),
                              textcoords = 'offset points')
            self._ax.annotate(str("{:.2f}".format(t*100)) + "%", 
                              xy = (d, 0.10),
                              xycoords = 'data', 
                              xytext = (d+t, 0.10), 
                              textcoords = 'offset points')
            d += t
            
        self._figure.canvas.draw()
        self._vbox.show_all()
        
    ##################################################
        
    def create_menu_ratio(self):
        """Create and add all the widget to configure ratio"""
        
        #Remove the all widget
        for child in self._vbox_ratio.get_children():
            self._vbox_ratio.remove(child)
        
        for modcod,value in self._dico_modcod.items():
            hbox_ratio=gtk.HBox()
            
            modcod_name = gtk.Label(str = modcod)
            ratio = gtk.Label(str = "Ratio")
            ajustement = gtk.Adjustment(int(value), 1, 10000, 1, 8)
            modcod_ratio = gtk.SpinButton(ajustement, digits = 0)
            modcod_ratio.connect("value-changed", self.on_update_ratio, modcod)
            
            hbox_ratio.pack_start(modcod_name, fill = False)
            hbox_ratio.pack_start(ratio, fill = False)
            hbox_ratio.pack_start(modcod_ratio, fill = False)
            
            self._vbox_ratio.pack_start(hbox_ratio, expand = False, fill = False)
        self._vbox.show_all()
        
    ##################################################
        
    def get_active_access_type(self):
        #Get the toggle access_type 
        all_access_type = self._vbox_access_type.get_children()
        for button in all_access_type:
            if button.get_active():
                access_type = button.get_label()
        return access_type
    ##################################################
        
    def get_active_modcod(self):
        #Get all toggle modcod
        all_modcod = self._vbox_modcod.get_children()
        fmt_id = 1
        modcod = []
        ratio = []
        modcods = {}
        if self._vcm_radio is not None and self._vcm_radio.get_active():
            for button in all_modcod:
                if button.get_active():
                    modcod.append(fmt_id)
                    ratio.append(self._dico_modcod[button.get_label()])
                    modcods[';'.join(str(e) for e in modcod)] = ';'.join(str(e) for e in ratio)
                modcod = []
                ratio = []
                fmt_id += 1

        else:
            for button in all_modcod:
                if button.get_active():
                    modcod.append(fmt_id)
                    ratio.append(self._dico_modcod[button.get_label()])
                fmt_id += 1
        
            modcods[';'.join(str(e) for e in modcod)] = ratio[0]
        return modcods
    ##################################################
    
    def on_save_edit_clicked(self, source=None):
        """Save the modcod configuration """
        modcods = self.get_active_modcod()
        self._parent.get_list_carrier()[self._carrier_id-1].setAccessType(
            self.get_active_access_type())
        self._parent.get_list_carrier()[self._carrier_id-1].setModcod(
            modcods.keys())
        ratio = ';'.join(str(e) for e in modcods.values())
        self._parent.get_list_carrier()[self._carrier_id-1].setRatio(
            ratio)
        self._parent.get_list_carrier()[self._carrier_id-1].setRatio(
            ratio)
        self._dlg.destroy()
    
    ##################################################
    
    def on_edit_dialog_event(self, source=None):
        """Quit by red cross"""
        self._dlg.destroy()
    
    ##################################################
        
    def on_cancel_edit_clicked(self, source=None):
        """Quit by cancel button"""
        self._dlg.destroy()
        
##################################################
if __name__ == '__main__':
    
    app = ModcodParameter()
    gtk.main()


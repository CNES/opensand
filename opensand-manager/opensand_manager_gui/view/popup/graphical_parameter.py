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
graphical_parameter.py - Some graphical parameters for band configuration
"""

import gtk
import gobject

from matplotlib.figure import Figure
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigureCanvas

from opensand_manager_core.carrier import Carrier
from opensand_manager_core.carriers_band import CarriersBand
from opensand_manager_core.utils import get_conf_xpath, ROLL_OFF, CARRIERS_DISTRIB, \
        FMT_GROUPS, ID, BANDWIDTH, FMT_ID, FMT_GROUP, RATIO, SYMBOL_RATE, \
        CATEGORY, ACCESS_TYPE, CCM, DAMA, FORWARD_DOWN, RETURN_UP 
from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_gui.view.popup.modcod_dialog import ModcodParameter
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup

class GraphicalParameter(WindowView):
    """ an band configuration window """
    def __init__(self, model, spot, gw,  manager_log, update_cb, link):     
    
        WindowView.__init__(self, None, 'band_configuration_dialog')

        self._dlg = self._ui.get_widget('band_configuration_dialog')
        self._dlg.set_keep_above(False)
        
        self._model = model
        self._spot = spot
        self._gw = gw
        self._bandwidth_total = 1 
        self._log = manager_log
        self._enabled_button = []
        self._removed = []
        self._new_fmt_grps = {}
        self._link = link
        self._update_cb = update_cb
        self._description = {}
        
        title = self._ui.get_widget('label_title_parameter')
        
        if self._link == FORWARD_DOWN:
            title.set_text(    "<b>Forward Band Configuration</b>")
        elif self._link == RETURN_UP:
            title.set_text(    "<b>Return Band Configuration</b>")
        title.set_use_markup(True)
            
        def ok(source=None, event=None):
            source.hide_all()
        
        #Create graphe in the window
        self._graphe = self._ui.get_widget('scrolled_graph_parameter')
        self._figure = Figure()
        self._ax = self._figure.add_subplot(111)
        canvas = FigureCanvas(self._figure)
        canvas.set_size_request(400,400)
        self._graphe.add_with_viewport(canvas)
        
        self._list_carrier = []
        self._fmt_group = {}
        self._current_fmt = {}
        self._nb_carrier = 0
        self._color = {1:'b-', 
                       2:'g-', 
                       3:'c-', 
                       4:'m-', 
                       5:'y-', 
                       6:'k-', 
                       7:'r-'}
        
        self._vbox = self._ui.get_widget('vbox_band_parameter')
        self._vbox.show_all()
            
        
    def go(self):
        """ run the window """
        button_rolloff = self._ui.get_widget('spinbutton_rollof_parameter')
        button_rolloff.connect("value_changed", self.on_update_roll_off)
        
        button_add = self._ui.get_widget('button_add_carriers')
        button_add.get_image().show()
        button_add.connect("clicked", self.on_add_to_listCarrier)
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
        self._dlg.set_title("Resources configuration - OpenSAND Manager")
        self._dlg.run()
        
    
    def load(self):
        """ load the hosts configuration """
        config = self._model.get_conf().get_configuration()
        
        # fmt groups
        xpath = get_conf_xpath(FMT_GROUPS, self._link, 
                               self._spot, self._gw)
        for group in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(group)
            self._fmt_group[content[ID]] = content[FMT_ID]
        self._current_fmt = self._fmt_group 

        xpath = get_conf_xpath(BANDWIDTH, self._link, self._spot, self._gw)
        # bandwidth in Hz
        bandwidth = float(config.get_value(config.get(xpath))) * 1000000

        xpath = get_conf_xpath(ROLL_OFF, self._link)
        roll_off = float(config.get_value(config.get(xpath)))

        #Load carrier from xml file
        xpath = get_conf_xpath(CARRIERS_DISTRIB, self._link, 
                               self._spot, self._gw)
        total_ratio_rs = 0
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            ratios = map(lambda x: float(x), content[RATIO].split(';'))
            total_ratio_rs += float(content[SYMBOL_RATE]) * sum(ratios)
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            list_modcod = []
            for fmt_grp_id in content[FMT_GROUP].split(";"):
                list_modcod.append(self._fmt_group[fmt_grp_id])
                ratios = map(lambda x: float(x), content[RATIO].split(';'))
                nb_carrier = int(round(sum(ratios) / \
                                 total_ratio_rs * bandwidth / (1 + roll_off)))
                if nb_carrier <= 0:
                    nb_carrier = 1
            new_carrier = Carrier(float(content[SYMBOL_RATE]),
                                  nb_carrier, content[CATEGORY], 
                                  content[ACCESS_TYPE], content[FMT_GROUP], 
                                  list_modcod, content[RATIO])
            self._list_carrier.append(new_carrier)
            self._nb_carrier = len(self._list_carrier)
            self.create_carrier_interface(self._list_carrier[-1],
                                          self._nb_carrier)
            
        self.trace()
        
        xpath = get_conf_xpath(ROLL_OFF, self._link)
        self._ui.get_widget('spinbutton_rollof_parameter').set_value(
                            float(config.get_value(config.get(xpath))))
        
    def clear_graph(self):
        """
        Clear the graphical representation
        """
        self._ax.cla()
        self._figure.canvas.draw()
        
    
    def clear_carrier_interface(self):
        """Clear the carrier menu """
        self._enabled_button = []
        table = self._ui.get_widget('vbox_carriers_list')
        for element in table.get_children():
            table.remove(element)
     
    def create_carrier_interface(self, carrier, carrier_id):
        """
        Add a new carrier
        One carrier is define by (symbol_rate, fmt, group, 
        accesse type and delete button)
        """
        
        hbox_carrier = gtk.HBox(homogeneous=True, spacing=0)
        hbox_name_carrier = gtk.HBox(homogeneous=True, spacing=0)
        name_carrier = gtk.Label("Carrier"+str(carrier_id))
        
        img = gtk.Image()
        img.set_from_stock(gtk.STOCK_DIALOG_INFO,
                           gtk.ICON_SIZE_MENU)
        
        hbox_name_carrier.pack_start(name_carrier, expand=False, fill=False)
        hbox_name_carrier.pack_start(img, expand=True, fill=False, padding=1)

        self._description[carrier_id] = img
        
        #Create Synbol rate
        ajustement1 = gtk.Adjustment(float(carrier.getSymbolRate()) / 1E6, 
                                     0, 10000, 1, 8)
        new_sr = gtk.SpinButton(ajustement1, digits=2)
        new_sr.set_numeric(True)
        new_sr.set_name("sr"+str(carrier_id))
        new_sr.connect("value-changed", self.on_update_sr, carrier_id)
        
        ajustement2 = gtk.Adjustment(int(carrier.getNbCarrier()),
                                     1, 5, 1, 8)
        new_nb_carrier = gtk.SpinButton(ajustement2, digits=0)
        new_nb_carrier.set_numeric(True)
        new_nb_carrier.set_name("nb_carrier"+str(carrier_id))
        new_nb_carrier.connect("value-changed", self.on_update_nb_carrier, carrier_id)

        #Create Group spin button
        comboBox = gtk.combo_box_new_text()
        comboBox.append_text('Standard')
        comboBox.append_text('Premium')
        comboBox.append_text('Pro')
        comboBox.connect("changed", self.on_update_group, carrier_id)
        comboBox.set_active(carrier.getCategory()-1)

        #Create MODCOD button
        button_modcod = gtk.Button(label="Configure")
        button_modcod.connect("clicked", 
                              self.on_modcod_configuration_clicked,
                              carrier_id)
        
        hbox_button = gtk.HBox(homogeneous=True, spacing=0)
        #Create tooltips for button
        tooltip = gtk.Tooltips()
        #Copy button
        image_copy = gtk.Image()
        image_copy.set_from_stock(gtk.STOCK_COPY, gtk.ICON_SIZE_MENU)
        button_copy = gtk.Button()
        button_copy.set_image(image_copy)
        button_copy.get_image().show()
        button_copy.connect("clicked", self.on_copy_carrier, carrier_id)
        tooltip.set_tip(button_copy, "Copy")
        
        #Delete button
        image_del = gtk.Image()
        image_del.set_from_stock(gtk.STOCK_CANCEL, gtk.ICON_SIZE_MENU)
        button_del = gtk.Button()
        button_del.set_image(image_del)
        button_del.get_image().show()
        button_del.connect("clicked", self.on_remove_carrier, carrier_id)
        tooltip.set_tip(button_del, "Delete")
        self._enabled_button.append(button_del)
        if len(self._enabled_button) > 1:
            for button in self._enabled_button:
                button.set_sensitive(True)
        else:
            button_del.set_sensitive(False)
        
        hbox_button.pack_start(button_copy, expand=False, fill=False)
        hbox_button.pack_start(button_del, expand=False, fill=False)
        
        
        hbox_carrier.pack_start(hbox_name_carrier, expand=False, fill=False)
        hbox_carrier.pack_start(new_sr, expand=False, fill=False)
        hbox_carrier.pack_start(new_nb_carrier, expand=False, fill=False)
        hbox_carrier.pack_start(comboBox, expand=False, fill=False, padding=0)
        hbox_carrier.pack_start(button_modcod, expand=False, fill=False)
        hbox_carrier.pack_start(hbox_button, expand=False, fill=False)
        
        table = self._ui.get_widget('vbox_carriers_list')
        table.pack_start(hbox_carrier, expand=False, fill=False)
        
        self._vbox.show_all()
            
    
    def on_add_to_listCarrier(self, source=None, event=None):
        """Create a new carrier with default value """
        self._nb_carrier += 1
        fmt_grp = self._fmt_group.keys()[0]
        
        if self._link == FORWARD_DOWN:
            self._list_carrier.append(Carrier(symbol_rate=4E6,
                                              category=1,
                                              fmt_groups=fmt_grp,
                                              access_type=CCM))
        else:
            self._list_carrier.append(Carrier(symbol_rate=4E6,
                                              category=1,
                                              fmt_groups=fmt_grp,
                                              access_type=DAMA))
        self.create_carrier_interface(self._list_carrier[-1], 
                                      self._nb_carrier)
        self.trace()
        
    def on_copy_carrier(self, source=None, id_carrier=None):
        """Copy a carrier identify by his ID"""    
        self._nb_carrier += 1

        sr = self._list_carrier[id_carrier-1].getSymbolRate()
        nb = self._list_carrier[id_carrier-1].getNbCarrier()
        g = self._list_carrier[id_carrier-1].getCategory()
        ac = self._list_carrier[id_carrier-1].getAccessType()
        fmt_grp = self._list_carrier[id_carrier-1].get_str_fmt_grp()
        md = self._list_carrier[id_carrier-1].get_str_modcod()
        ra = self._list_carrier[id_carrier-1].getStrRatio()
        
        self._list_carrier.append(Carrier(sr, nb, g, ac, fmt_grp, md, ra))
        self.clear_carrier_interface()
        
        carrier_id = 1
        for element in self._list_carrier:
            self.create_carrier_interface(element, carrier_id)
            carrier_id += 1

        self.trace()
    
    
    def on_remove_carrier(self, source=None, id_carrier=None):
        """ Remove carrier in the list identify by his ID"""
        del self._list_carrier[id_carrier-1]
        self._removed.append(id_carrier-1)
        self._nb_carrier = self._nb_carrier - 1
        self.clear_carrier_interface()
        
        carrier_id = 1
        for carrier in self._list_carrier:
            self.create_carrier_interface(carrier, carrier_id)
            carrier_id += 1
        self.trace()

    def trace(self, source=None):
        """
        Draw in the graph area the graphical representation of the 
        content in the carrier list
        """
        self.update_ratio()
        
        roll_off = float(self._ui.get_widget('spinbutton_rollof_parameter').get_value())

        config = self._model.get_conf().get_configuration()
        carriers_band = CarriersBand() 
        carriers_band.modcod_def(self._model.get_scenario(), 
                                 self._link, config, False)
        for carrier in self._list_carrier:
            carriers_band.add_carrier(carrier)
        for fmt_id in self._fmt_group: 
            carriers_band.add_fmt_group(int(fmt_id),
                                        self._fmt_group[fmt_id])

        off_set = 0
        carrier_id = 1
        self.clear_graph()
        for element in self._list_carrier :
            description = ''
            for nb_carrier in range(1, element.getNbCarrier()+1):
                element.calculateXY(roll_off, off_set)
                self._ax.plot(element.getX(), element.getY(), 
                              self._color[element.getCategory()], label = 'Carrier')
                # bandwidth in MHz
                off_set = off_set + element.getBandwidth(roll_off) \
                        / (1E6 * element.getNbCarrier())

            for (min_rate, max_rate) in carriers_band.get_carrier_bitrates(element):
                description += "Rate         [%d, %d] kb/s\n" % (min_rate / 1000,
                                                      max_rate / 1000)
           
            description += "Total rate [%d, %d] kb/s" % \
                    (carriers_band.get_min_bitrate(element.get_old_category(), 
                                                   element.getAccessType()) / 1000,
                     carriers_band.get_max_bitrate(element.get_old_category(),
                                                   element.getAccessType()) / 1000)
            self._description[carrier_id].set_tooltip_text(description)
            carrier_id += 1
            
        if off_set != 0:
            self._ax.axis([float(-off_set)/6, 
                           off_set + float(off_set)/6, 
                           0, 1.5])
        bp, = self._ax.plot([0, 0, off_set, off_set], 
                            [0, 1, 1, 0], 'r-', 
                            label = BANDWIDTH, 
                            linewidth = 3.0)
        self._ax.legend([bp],[BANDWIDTH])
        self._ax.grid(True)
        self._figure.canvas.draw()
        self._ui.get_widget('bandwith_total').set_text(str(off_set) + " MHz")
        self._bandwidth_total = off_set

    def on_update_roll_off(self, source=None):
        # disable method calling twice at the fisrt time
        gobject.idle_add(self.trace)
        
    def on_update_sr(self, source = None, carrier_id = None):
        """R
        group_list = []efresh the graph when the symbole rate change"""
        # disable method calling twice at the fisrt time
        gobject.idle_add(self.on_update_sr_callback, 
                         source, carrier_id)

    def on_update_sr_callback(self, source=None, carrier_id=None):
        self._list_carrier[carrier_id-1].setSymbolRate(source.get_value() * 1E6)
        self.trace()
    
    def on_update_nb_carrier(self, source = None, carrier_id = None):
        """Refresh the graph when the symbole rate change"""
        # disable method calling twice at the fisrt time
        gobject.idle_add(self.on_update_nb_carrier_callback,
                         source, carrier_id)
        
    def on_update_nb_carrier_callback(self, source=None, carrier_id=None):
        self._list_carrier[carrier_id-1].setNbCarrier(int(source.get_value()))
        self.trace()
    
    def update_ratio(self):
        total_ratio_rs = 0
        total_ratio = 0
        roll_off = float(self._ui.get_widget('spinbutton_rollof_parameter').get_value())
        for carrier in self._list_carrier:
            total_ratio_rs += sum(carrier.getRatio()) * \
                    carrier.getSymbolRate() / 1E6
        for carrier in self._list_carrier:
            total_ratio += int(round(carrier.getNbCarrier() * (1 + roll_off) /\
                                     self._bandwidth_total * total_ratio_rs ))

        for carrier in self._list_carrier:
            # bandwidth and bandwidth_total in Mhz
            ratio = int(round(carrier.getNbCarrier() * (1 + roll_off) /\
                    self._bandwidth_total * total_ratio_rs / total_ratio *  100))
            ratios = carrier.getRatio()
            old_ratios = list(ratios)
            ratio_str = ""
            index = 1
            for r in ratios:
                new_ratio = ratio * r / sum(old_ratios)
                if index != len(ratios):
                    ratio_str += str(new_ratio) + ";"
                else:
                    ratio_str += str(new_ratio)
                index += 1

            carrier.setRatio(ratio_str)
            

    def on_update_group(self, source=None, id_carrier=None):
        """Refresh the graph with new color when the group change"""
        tree_iter = source.get_active_iter()
        if tree_iter != None:
            model = source.get_model()
            self._list_carrier[id_carrier-1].setCategory(model[tree_iter].path[0]+1)    
            self.trace()
    
    
    def on_modcod_configuration_clicked(self, widget, id_carrier=None, event=None):
        """Open the modcod configuration window """
        window = ModcodParameter(self._model, 
                                 self._log, 
                                 self._link, 
                                 id_carrier, 
                                 self)
        
        window.go()

        new_fmt_id = int(self._fmt_group.keys()[-1]) + 1
        for carrier in self._list_carrier:
            fmt_groups = []
            for carrier_fmt_group in carrier.get_str_modcod():
                found = False
                for grp_id in self._fmt_group:
                    if carrier_fmt_group == self._fmt_group[grp_id]:
                        fmt_groups.append(grp_id)
                        found = True
                        break
                if not found:
                    fmt_groups.append(new_fmt_id)
                    self._new_fmt_grps[new_fmt_id] = carrier_fmt_group
                    self._fmt_group[str(new_fmt_id)] = carrier_fmt_group
                    new_fmt_id += 1
            
            carrier.setFmtGroups(';'.join(str(fmt_grp_id) for fmt_grp_id in
                                      fmt_groups))



    def on_band_configuration_dialog_save(self, source=None, event=None):
        #get the file xml
        config = self._model.get_conf().get_configuration()
        
        #save bandwidth
        xpath = get_conf_xpath(BANDWIDTH, self._link, self._spot, self._gw)
        bandwidth = self._ui.get_widget('bandwith_total').get_text().split(' ')
        config.set_value(bandwidth[0], xpath)
        
        #save roll_off
        xpath = get_conf_xpath(ROLL_OFF, self._link)
        roll_off = float(self._ui.get_widget('spinbutton_rollof_parameter').get_value())
        config.set_value(roll_off, xpath)
        
        #save carriers_distribution
        xpath = get_conf_xpath(CARRIERS_DISTRIB, self._link, 
                               self._spot, self._gw)
        table = config.get(xpath)
        
        #Get the carrier number in xml
        carrier_line = []
        for element in config.get_table_elements(table):
            carrier_line.append(config.get_element_content(element))
        i = len(carrier_line)
        #Place the number of carrier in xml
        while i < len(self._list_carrier):
            config.add_line(config.get_path(table))
            i += 1
        table = config.get(xpath)
        
        # remove 
        i = len(carrier_line)
        while i > len(self._list_carrier):
            config.remove_line(config.get_path(table), 0)
            table = config.get(xpath)
            i -= 1

        used_fmt_grps = []
        new_fmt_id = int(self._fmt_group.keys()[-1]) + 1
        #Save all carrier element
        carrier_id = 0
        access_type_cat = {}
        for carrier in self._list_carrier:
            config.set_value(carrier.get_old_access_type(), 
                             config.get_path(config.get_table_elements(table)[carrier_id]),
                             ACCESS_TYPE)
            config.set_value(carrier.get_old_category(), 
                             config.get_path(config.get_table_elements(table)[carrier_id]),
                             CATEGORY)
            config.set_value(carrier.getStrRatio(), 
                             config.get_path(config.get_table_elements(table)[carrier_id]),
                             RATIO)
            config.set_value(carrier.getSymbolRate(), 
                             config.get_path(config.get_table_elements(table)[carrier_id]),
                             SYMBOL_RATE)

            if carrier.get_old_category() not in access_type_cat.keys():
                access_type_cat[carrier.get_old_category()] = []
            access_type_cat[carrier.get_old_category()].append(carrier.get_old_access_type())
            
            fmt_groups = []
            for carrier_fmt_group in carrier.get_str_modcod():
                found = False
                for grp_id in self._fmt_group:
                    if carrier_fmt_group == self._fmt_group[grp_id]:
                        used_fmt_grps.append(grp_id)
                        fmt_groups.append(grp_id)
                        found = True
                        break
                if not found:
                    fmt_groups.append(new_fmt_id)
                    self._new_fmt_grps[new_fmt_id] = carrier_fmt_group
                    self._fmt_group[str(new_fmt_id)] = carrier_fmt_group
                    new_fmt_id += 1
            config.set_value(';'.join(str(fmt_grp_id) for fmt_grp_id in
                                      fmt_groups), 
                             config.get_path(config.get_table_elements(table)[carrier_id]),
                             FMT_GROUP)
            carrier.setFmtGroups(';'.join(str(fmt_grp_id) for fmt_grp_id in
                                      fmt_groups))
        
            carrier_id += 1

        for category in access_type_cat:
            if "SCPC" in access_type_cat[category]:
                for access_type in access_type_cat[category]:
                    if access_type != "SCPC":
                        error_popup(str("For category %s access type can only "\
                                        "be SCPC, please check all carriers" % category))
                        return None

       
        
        #save fmt
        xpath = get_conf_xpath(FMT_GROUPS, self._link, self._spot, self._gw)
        table = config.get(xpath)
        
        #Create or delete to have the good number
        for i in range(0, len(self._new_fmt_grps)):
            config.add_line(config.get_path(table))
            table = config.get(xpath)
        
        #remove unused fmt_group
        copy_fmt_group = self._fmt_group.copy()
        for fmt_id in copy_fmt_group:
            elmts = config.get_table_elements(table)
            if fmt_id not in used_fmt_grps:
                for elm in elmts:
                    if elm.get(ID) == fmt_id:
                        del self._fmt_group[fmt_id]
                        config.del_element(config.get_path(elm));
                table = config.get(xpath)
        
        #Save all fmt element
        i = len(self._fmt_group) - len(self._new_fmt_grps)
        for grp_id in self._new_fmt_grps:
            config.set_value(grp_id, 
                             config.get_path(config.get_table_elements(table)[i]),
                             ID)
            config.set_value(self._new_fmt_grps[grp_id], 
                             config.get_path(config.get_table_elements(table)[i]),
                             FMT_ID)
            i += 1
            
        config.write()
        self._removed = []
        self._new_fmt_grps = {}
        gobject.idle_add(self._update_cb)
        self._dlg.destroy()
        

    def on_band_configuration_dialog_destroy(self, source=None, event=None):
        """Close the window """
        self._fmt_group.clear()
        gobject.idle_add(self._update_cb)
        self._dlg.destroy()

    def on_band_conf_delete_event(self, source=None, event=None):
        """Close the window """
        self._fmt_group.clear()
        self._dlg.destroy()

    def get_list_carrier(self):
        return self._list_carrier
    

#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2016 TAS
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
import gobject

from matplotlib.figure import Figure
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigureCanvas


from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_core.utils import FORWARD_DOWN, RETURN_UP, CCM, ACM, VCM, \
                                        DAMA, ALOHA, SCPC, DVB_RCS, DVB_RCS2
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup


MODCOD_DEF_S2="modcod_def_s2"
MODCOD_DEF_RCS="modcod_def_rcs"
MODCOD_DEF_RCS2="modcod_def_rcs2"

class ModcodParameter(WindowView):
    """ an modcod configuration window """
    def __init__(self, model, manager_log, link, carrier_id, list_carrier, update_cb):

        WindowView.__init__(self, None, 'edit_dialog')

        self._update_cb = update_cb

        self._dlg = self._ui.get_widget('edit_dialog')
        self._dlg.set_keep_above(True)
        self._dlg.set_modal(True)

        self._vbox = self._ui.get_widget('edit_dialog_vbox')
        self._edit_text_win = self._ui.get_widget('edit_text_win')
        self._carrier_id = carrier_id
        self._list_carrier = list_carrier
        self._link = link
        self._model = model

        #Add vbox_conf Box
        self._vbox_conf = gtk.VBox()
        self._edit_text_win.add_with_viewport(self._vbox_conf)

        #Add hbox_mod in vbox_conf
        self._hbox_modcod = gtk.HBox()
        self._vbox_conf.pack_start(self._hbox_modcod)
        #Add vbox_vcm_option in vbox_conf
        self._vbox_vcm_option = gtk.VBox()
        self._vbox_conf.pack_start(self._vbox_vcm_option, expand=False)

        #Add frame_access_type in hbox_modcod
        self._frame_access_type = gtk.Frame(label="Access Type")
        self._frame_access_type.set_shadow_type(gtk.SHADOW_OUT)
        self._hbox_modcod.pack_start(self._frame_access_type,padding = 10)
        #Add frame_modcod in hbox_modcod
        self._frame_modcod = gtk.Frame(label="Wave Form")
        self._frame_modcod.set_shadow_type(gtk.SHADOW_OUT)
        self._hbox_modcod.pack_start(self._frame_modcod, padding = 25)

        #Add vbox_access_type in frame_access_type
        self._vbox_access_type = gtk.VBox()
        self._frame_access_type.add(self._vbox_access_type)
        #Add modcod_scroll_window in frame_modcod
        self._vbox_modcods = gtk.VBox()
        self._check_modcod = gtk.CheckButton(label="select all")
        self._check_modcod.connect("toggled", self.on_all_modcod_toggled)
        self._modcod_scroll_window = gtk.ScrolledWindow()
        self._modcod_scroll_window.set_size_request(100, 200)
        self._vbox_modcods.pack_start(self._check_modcod, fill=False , expand=False)
        self._vbox_modcods.pack_start(self._modcod_scroll_window, fill=False , expand=False)
        self._frame_modcod.add(self._vbox_modcods)
        #Add vbox_modcod in modcod_scroll_window
        self._vbox_modcod = gtk.VBox()
        self._modcod_scroll_window.add_with_viewport(self._vbox_modcod)

        #Create Frame_temporal_graph
        self._frame_temporal_graph = gtk.Frame(label="Temporal Representation")

        self._figure = Figure()
        self._ax = self._figure.add_subplot(111)
        canvas = FigureCanvas(self._figure)
        canvas.set_size_request(150, 150)
        self._frame_temporal_graph.add(canvas)

        #Create scroll_ratio
        self._scroll_ratio = gtk.ScrolledWindow()
        self._scroll_ratio.set_size_request(150, 120)

        #Add vbox_ratio in scroll_ratio
        self._vbox_ratio = gtk.VBox()
        self._scroll_ratio.add_with_viewport(self._vbox_ratio)

        #Size and display
        self._dlg.set_default_size(500, 300)

        #Item List to simply access to toogle button
        self._item_list = []
        self._list_modcod_ratio = {}
        self._dico_modcod = {}

        self._vcm_radio = None


    def go(self):
        """ run the window """
        self._vbox.show_all()

        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
        self._dlg.set_title("MODCOD parameters - OpenSAND Manager")
        self._dlg.run()


    def load(self):
        #Get the access type
        access_type = self._list_carrier[self._carrier_id - 1].get_access_type()
        ret_lnk_std = self._model.get_conf().get_return_link_standard()

        #Add the access type button for forward or return
        if self._link == FORWARD_DOWN:
            #Add radio button
            self._ccm_radio = gtk.RadioButton(group=None, label=CCM,
                                              use_underline=True)
            self._acm_radio = gtk.RadioButton(group=self._ccm_radio,
                                              label=ACM,
                                              use_underline=True)
            self._vcm_radio = gtk.RadioButton(group=self._ccm_radio,
                                              label=VCM,
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
            self._dama_radio = gtk.RadioButton(group=None, label=DAMA,
                                               use_underline=True)
            self._aloha_radio = gtk.RadioButton(group=self._dama_radio,
                                                label=ALOHA,
                                                use_underline=True)
            self._scpc_radio = gtk.RadioButton(group=self._dama_radio,
                                               label=SCPC, use_underline=True)
            if ret_lnk_std != DVB_RCS2: 
                self._dama_radio.connect("toggled", self.on_toggled)
            else:
                self._dama_radio.connect("toggled", self.on_dama_rcs2_toggled)
            self._aloha_radio.connect("toggled", self.on_toggled)
            self._scpc_radio.connect("toggled", self.on_scpc_toggled)
            self._vbox_access_type.pack_start(self._dama_radio,
                                              expand=True, fill=True)
            self._vbox_access_type.pack_start(self._aloha_radio,
                                              expand=True, fill=True)
            self._vbox_access_type.pack_start(self._scpc_radio,
                                              expand=True, fill=True)

            if self._model.get_conf().get_payload_type() == 'regenerative':
                self._aloha_radio.set_sensitive(False)
                self._scpc_radio.set_sensitive(False)

        #Load access type from the carrier
        if access_type == CCM:
            self._ccm_radio.toggled()
        elif access_type == ACM:
            self._acm_radio.set_active(True)
        elif access_type == VCM:
            self._vcm_radio.set_active(True)
        elif access_type == DAMA:
            self._dama_radio.toggled()
        elif access_type == ALOHA:
            self._aloha_radio.set_active(True)
        elif access_type == SCPC:
            self._scpc_radio.set_active(True)

        #Load modcod from carrier
        modcod_list = self._list_carrier[self._carrier_id - 1].get_modcod()
        list_ratio = self._list_carrier[self._carrier_id - 1].get_ratio()
        index = 0
        nb_active = 0
        for modcod in modcod_list:
            self._list_modcod_ratio[modcod] = list_ratio[index]
            if len(modcod_list) == len(list_ratio):
                index += 1
            for item in self._item_list:
                modcod_id = ModcodParameter.get_modcod_index(item.get_label())
                if int(modcod) == modcod_id:
                    item.set_active(True)
                    nb_active += 1
        if nb_active == len(self._item_list):
            self._check_modcod.set_active(True)

        self._vbox_conf.show_all()


    def close(self):
        """ close the window """
        self._dlg.destroy()


    def on_edit_dialog_delete_event(self, source=None, event=None):
        """ close and delete the window """
        self.close()


    def add_modcod_item(self, source=None):
        """display all the modcod button"""

        #remove the old list
        for child in self._vbox_modcod.get_children():
            self._vbox_modcod.remove(child)
        #Add the new list in the vbox
        for element in self._item_list:
            self._vbox_modcod.pack_start(element, expand=False, fill=False)

        self._vbox.show_all()

    @staticmethod
    def get_modcod_label(entry):
        if len(entry) < 5:
            return ""
        label = "{} - {} {}".format(entry[0], entry[1], entry[2])
        return label

    @staticmethod
    def get_modcod_index(label):
        elts = label.split()
        return int(elts[0])

    def set_modcod_widgets(self, source, is_radio, option=None):
        """ Create a list of widget with list of modcods """
        #MODCOD list from file definition.txt
        global_conf = self._model.get_conf()
        ret_lnk_std = global_conf.get_return_link_standard()
        has_burst_length = False
        self._check_modcod.set_active(False)
        if self._link == FORWARD_DOWN:
            path = global_conf.get_param(MODCOD_DEF_S2)
            if option == CCM:
                if self._check_modcod in self._vbox_modcods.get_children():
                    self._vbox_modcods.remove(self._check_modcod)
            else:
                if self._check_modcod not in self._vbox_modcods.get_children():
                    self._vbox_modcods.pack_start(self._check_modcod, fill=False , expand=False)
                    self._vbox_modcods.reorder_child(self._check_modcod, 0)
        elif self._link == RETURN_UP:
            if option == SCPC:
                if self._check_modcod not in self._vbox_modcods.get_children():
                    self._vbox_modcods.pack_start(self._check_modcod, fill=False , expand=False)
                    self._vbox_modcods.reorder_child(self._check_modcod, 0)
                path = global_conf.get_param(MODCOD_DEF_S2)
            elif option == DAMA and ret_lnk_std == DVB_RCS2:
                if self._check_modcod not in self._vbox_modcods.get_children():
                    self._vbox_modcods.pack_start(self._check_modcod, fill=False , expand=False)
                    self._vbox_modcods.reorder_child(self._check_modcod, 0)
                path = global_conf.get_param(MODCOD_DEF_RCS2)
                has_burst_length = True
            else:
                if self._check_modcod in self._vbox_modcods.get_children():
                    self._vbox_modcods.remove(self._check_modcod)
                if ret_lnk_std == DVB_RCS2:
                    path = global_conf.get_param(MODCOD_DEF_RCS2)
                    has_burst_length = True
                else:
                    path = global_conf.get_param(MODCOD_DEF_RCS)

        modcod_list = self.load_modcod(path, has_burst_length)

        del self._item_list[:]
        #Create tooltips for button
        tooltip = gtk.Tooltips()
        #If button become enabled
        if source.get_active():
            check_modcod = None
            for modcod in modcod_list:
                #In radio button the first radio have no group
                modcod_label = ModcodParameter.get_modcod_label(modcod)
                if is_radio:
                    radio_group = None
                    if modcod == modcod_list[0]:
                        #Default value for ratio is 10
                        if modcod in self._list_modcod_ratio.keys():
                            self._dico_modcod[modcod_label] = \
                                self._list_modcod_ratio[modcod]
                        else:
                            self._dico_modcod[modcod_label] = 10
                    else:
                        radio_group = self._item_list[0]
                    check_modcod = gtk.RadioButton(group = radio_group,
                                                   label = modcod_label,
                                                   use_underline = True)
                else:
                    check_modcod = gtk.CheckButton(label = modcod_label,
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

    def on_all_modcod_toggled(self, source=None):
        if source.get_active():
            for check_modcod in self._item_list:
                check_modcod.set_active(True)
        else:
            for check_modcod in self._item_list:
                check_modcod.set_active(False)


    def on_ccm_toggled(self, source=None):
        """Signal when ccm button change"""
        self.set_modcod_widgets(source, True, CCM)


    def on_acm_toggled(self, source=None):
        """Signal when acm button change"""
        self.set_modcod_widgets(source, False, ACM)

    def on_vcm_toggled(self, source=None):
        """Signal when vcm button change"""
        """ With the list of modcod it creates a list of widget"""
        self.set_modcod_widgets(source, False, VCM)
        if source.get_active():
            #Add the temporal representation graphe to the window
            self.trace_temporal_representation()
            #Refresh the ratio selection interface
            self.create_menu_ratio()
            #Extend the window size
            self._dlg.resize(500, 565)
            self.add_modcod_item()
        #Remove graph and ratio part if we leave vcm mode
        else:
            self._dlg.resize(500, 300)
            for child in self._vbox_vcm_option:
                self._vbox_vcm_option.remove(child)

    def on_scpc_toggled(self, source=None):
        """Signal when acm button change"""
        self.set_modcod_widgets(source, False, SCPC)

    def on_dama_rcs2_toggled(self, source=None):
        """Signal when acm button change"""
        self.set_modcod_widgets(source, False, DAMA)


    def on_toggled(self, source=None):
        self.set_modcod_widgets(source, True)


    def on_check_modcod(self, source=None, modcod=0):
        """Signal if it is selected"""
        #If modcod become enable
        if source.get_active():
            #Add this modcod in the dictionary with the default ratio value 10
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


    def on_update_ratio(self, source=None, modcod=None):
        """Signal when a ratio change"""
        self._dico_modcod[modcod] = int(source.get_value())
        self.trace_arrow()


    def load_modcod(self, path, has_burst_length=False):
        """Read in the file the available modcod"""
        global_conf = self._model.get_conf()
        mc_list = []
        burst_length = global_conf.get_rcs2_burst_length()
        with  open(path, 'r') as modcod_def:
            for line in modcod_def:
                if (line.startswith("/*") or
                    line.isspace() or
                    line.startswith('nb_fmt')):
                    continue
                elts = line.split()
                if not has_burst_length and len(elts) != 5:
                    continue
                elif has_burst_length and (len(elts) != 6
                     or elts[5] != burst_length):
                    continue
                if not elts[0].isdigit:
                    continue
                # id, modulation, coding_rate, spectral_efficiency, required Es/N0
                mc_list.append(elts)
        
        return mc_list

    def trace_temporal_representation(self):
        """Add graph and ratio selection in the window """

        self._vbox_vcm_option.pack_start(self._frame_temporal_graph)
        self._vbox_vcm_option.pack_start(self._scroll_ratio)
        #Clear the graph
        self._ax.cla()
        self._figure.canvas.draw()
        #Display all
        self._vbox.show_all()


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


    def get_active_access_type(self):
        #Get the toggle access_type
        all_access_type = self._vbox_access_type.get_children()
        for button in all_access_type:
            if button.get_active():
                access_type = button.get_label()
        return access_type


    def get_active_modcod(self):
        #Get all toggle modcod
        all_modcod = self._vbox_modcod.get_children()
        modcod = []
        ratio = []
        modcods = {}
        if self._vcm_radio is not None and self._vcm_radio.get_active():
            for button in all_modcod:
                if button.get_active():
                    modcod.append(ModcodParameter.get_modcod_index(button.get_label()))
                    ratio.append(self._dico_modcod[button.get_label()])
        else:
            for button in all_modcod:
                if button.get_active():
                    modcod.append(ModcodParameter.get_modcod_index(button.get_label()))
                    ratio.append(self._dico_modcod[button.get_label()])

        modcod_update = []
        if len(modcod) > 1 and (self._vcm_radio is None or not
                                self._vcm_radio.get_active()):
            first = modcod[0]
            last = modcod[0]
            row = False
            for i in range(1, len(modcod)):
                if int(modcod[i]) == int(modcod[i - 1]) + 1:
                    last = modcod[i]
                    row = True
                else:
                    row = False

                if not row or i == (len(modcod) - 1):
                    if first != last :
                        modcod_update.append(str(first) + "-" + str(last))
                    else:
                        if first not in modcod_update:
                            modcod_update.append(first)
                    first = modcod[i]
                    last = modcod[i]
                if not row and i == (len(modcod) - 1):
                    modcod_update.append(modcod[i])
        else:
            modcod_update = modcod

        return (modcod_update, ratio)



    def on_save_edit_clicked(self, source=None):
        """Save the modcod configuration """
        (modcods, ratios) = self.get_active_modcod()
        if len(modcods) == 0:
            error_popup("At least one modcod should be selected")
            return
        if self.get_active_access_type() == VCM and \
           len(modcods) != len(ratios):
            error_popup("There should be one MODCOD per ratio with VCM carriers")
            return
        if self.get_active_access_type() != VCM and len(ratios) != 1:
            ratios = [sum(ratios[:])]

        self._list_carrier[self._carrier_id - 1].set_access_type(
            self.get_active_access_type())

        modcod = ';'.join(str(e) for e in modcods)
        self._list_carrier[self._carrier_id - 1].set_modcod(modcod)
        ratio = ';'.join(str(e) for e in ratios)
        self._list_carrier[self._carrier_id - 1].set_ratio(ratio)

        gobject.idle_add(self._update_cb)

        self._dlg.destroy()


    def on_cancel_edit_clicked(self, source=None):
        """Quit by cancel button"""
        self.close()


##################################################
if __name__ == '__main__':
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model
    from opensand_manager_core.carrier import Carrier

    def nothing():
        pass

    LOGGER = ManagerLog(7, True, True, True)
    MODEL = Model(LOGGER)
    CARRIER = Carrier(12, 1, 1, 'VCM', "1;2", "20;18", "4;6")
    CARRIER_LIST = [CARRIER]
    WindowView(None, 'none', 'opensand.glade')
    app = ModcodParameter(MODEL, LOGGER, FORWARD_DOWN, 1, CARRIER_LIST, nothing)
    app.go()
    try:
        gtk.main()
    except KeyboardInterrupt:
        pass
    finally:
        for carrier in CARRIER_LIST:
            print "Carrier access type %s, ratio %s, MODCOD %s, groups %s" % (
                        carrier.get_access_type(), carrier.get_ratio(),
                        carrier.get_modcod(), carrier.get_fmt_groups())


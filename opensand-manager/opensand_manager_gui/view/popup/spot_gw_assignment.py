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


"""
spot_gw_assignment_dialog.py - ST assignmentconfiguration dialog
"""

import gtk

from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_core.utils import get_conf_xpath, ST, \
    ID, PATH_GW, PATH_SPOT, PATH_DEFAULT_SPOT, PATH_DEFAULT_GW,RETURN_UP_BAND
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.utils.config_elements import ManageSpot

MAX_SPOT_ID=10

class SpotGwAssignmentDialog(WindowView):
    """ an modcod configuration window """
    def __init__(self, model, manager_log):

        WindowView.__init__(self, None, 'edit_dialog')
        self._model = model
        self._default_spot = 1
        self._spot_list = []
        self._gw_list = []
        self._new_list = []
        self._default_gw = 0
        self.read_conf_spot()

        self._dlg = self._ui.get_widget('edit_dialog')
        self._dlg.set_keep_above(False)

        self._vbox = self._ui.get_widget('edit_dialog_vbox')
        self.edit_text_win = self._ui.get_widget('edit_text_win')

        #Add vbox_assignment Box
        self.vbox_assignment = gtk.VBox()
        self.edit_text_win.add_with_viewport(self.vbox_assignment)

        #Add hbox_assignment_title in vbox_assignment
        self.hbox_assignment_title=gtk.HBox()
        self.vbox_assignment.pack_start(self.hbox_assignment_title, expand=False)
        #Add scroll_st_allocation in vbox_assignment
        self.scroll_st_allocation=gtk.ScrolledWindow()
        self.vbox_assignment.pack_start(self.scroll_st_allocation)

        #Add vbox_st_allocation in scroll_st_allocation
        self.vbox_st_allocation = gtk.VBox()
        self.scroll_st_allocation.add_with_viewport(self.vbox_st_allocation)

        #Add title inn hbox_assignment_title
        self.st_title=gtk.Label(str="ST")
        self.spot_title=gtk.Label(str="SPOT")
        self.gw_title=gtk.Label(str="GW")

        self.hbox_assignment_title.pack_start(self.st_title)
        self.hbox_assignment_title.pack_start(self.spot_title)
        self.hbox_assignment_title.pack_start(self.gw_title)


        self._hbox_add_spot = gtk.HBox()
        label_spot = gtk.Label(" spot id ")
        adj = gtk.Adjustment(value=self._default_spot + 1, lower=1,
                             upper=MAX_SPOT_ID, step_incr=1)
        self._entry_spot_id = gtk.SpinButton(adjustment=adj, climb_rate=1)
        self._button_add_spot = gtk.Button()
        self._button_add_spot.set_label("Add spot")
        self._button_add_spot.connect('clicked', self.on_button_add_spot_clicked)
        self._hbox_add_spot.pack_start(label_spot)
        self._hbox_add_spot.pack_start(self._entry_spot_id)
        self._hbox_add_spot.pack_start(self._button_add_spot)
        self.vbox_assignment.pack_start(self._hbox_add_spot, expand=False,
                                        fill=False)

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

        for child in self.vbox_st_allocation.get_children():
            self.vbox_st_allocation.remove(child)
        
        self.get_default_spot_assignment()
        self.get_default_gw_assignment()

        #Load the configuration file
        config = self._model.get_topology().get_configuration()
        #Get ST list from opensand
        host_list = self._model.get_hosts_list()
        st_spot_list = []
        st_gw_list = []

        #Create combo box for group
        spots = gtk.ListStore(int, str)
        gws = gtk.ListStore(int, str)
        spot_set = set()
        self._gw_list = []

        for spot_id in self._spot_list:
            spot_value = "SPOT" + spot_id
            if spot_id not in spot_set:
                spot_set.add(int(spot_id))
                spots.insert(int(spot_id)-1,
                            [int(spot_id), spot_value])

                terminal_path = "%s/spot[@id='%s']/terminals" % (PATH_SPOT,
                                                                 spot_id)
                terminal = config.get(terminal_path)
                if terminal is not None:
                    for tal in config.get_table_elements(terminal):
                        tal_content = config.get_element_content(tal)
                        st_spot_list.append([tal_content[ID],
                                             int(spot_id)])

        for gw in config.get_table_elements(config.get(PATH_GW)):
            if gw.tag == "gw" :
                content = config.get_element_content(gw)
                gw_value =  "GW" + content[ID]
                if (int(content[ID])) not in self._gw_list:
                    self._gw_list.insert(int(content[ID]), int(content[ID]))
                    gws.insert(int(content[ID]),
                                 [int(content[ID]), gw_value])

                    terminal_path = "%s/gw[@id='%s']/terminals" % (PATH_GW,
                                                                   content[ID])
                    for tal in config.get_table_elements(config.get(terminal_path)):
                        tal_content = config.get_element_content(tal)
                        st_gw_list.append([tal_content[ID],
                                         int(content[ID])])

        for host in host_list:
            if not host.get_name().lower().startswith(ST):
                continue
            #Create box for st
            hbox_spot_allocation = gtk.HBox()
            hbox_gw_allocation = gtk.HBox()
            #Label for st
            label_st_name = gtk.Label(str=host.get_name().upper())
            #Create combo box for st
            combo_box_spot = gtk.ComboBox(spots)
            combo_box_gw = gtk.ComboBox(gws)
            renderer_text = gtk.CellRendererText()
            combo_box_spot.pack_start(renderer_text, True)
            combo_box_spot.add_attribute(renderer_text, "text", 1)
            def_spot = self.get_spot_value(spot_set,
                                           self._default_spot)
            combo_box_spot.set_active(def_spot)
            combo_box_gw.pack_start(renderer_text, True)
            combo_box_gw.add_attribute(renderer_text, "text", 1)
            def_gw = self.get_gw_value(self._gw_list,
                                       self._default_gw)
            combo_box_gw.set_active(def_gw)
            for st in st_spot_list:
                if host.get_name().lower() == ST + str(st[0]):
                    combo_box_spot.set_active(self.get_spot_value(spot_set,
                                                                  st[1]))
            for st in st_gw_list:
                if host.get_name().lower() == ST + str(st[0]):
                    combo_box_gw.set_active(self.get_gw_value(self._gw_list,
                                                              st[1]))
            #Add all in the window
            hbox_spot_allocation.pack_start(label_st_name)
            hbox_spot_allocation.pack_start(combo_box_spot)
            hbox_spot_allocation.pack_start(combo_box_gw)
            self.vbox_st_allocation.pack_start(hbox_spot_allocation, fill=False,
                                               expand=False)
            self.vbox_st_allocation.pack_start(hbox_gw_allocation, fill=False,
                                               expand=False)
        self.edit_text_win.show_all()


    def read_conf_spot(self):
        config = self._model.get_conf().get_configuration()
        xpath = "//"+RETURN_UP_BAND
        spot_set = set(self._spot_list)
        for key in config.get_keys(config.get(xpath)):
            if key.get(ID) is not None:
                spot_set.add(key.get(ID))
        self._spot_list = list(spot_set)


    def on_button_add_spot_clicked(self, source=None, event=None):
        spot_id_str = self._entry_spot_id.get_text()
        self._entry_spot_id.set_text(str(int(spot_id_str)+1))
        spot_id = int(spot_id_str)
        if spot_id_str not in self._spot_list and \
           spot_id >= 1 and \
           spot_id <= MAX_SPOT_ID:
            self._spot_list.append(spot_id_str)
            self._spot_list.sort()
            self._new_list.append(spot_id_str)
            self.load()
        else:
            error_popup("Spot id should not exist and stay between "
                        "1 and " + str(MAX_SPOT_ID))


    ##################################################

    def get_default_spot_assignment(self):
        """Return the default st assignment"""
        config = self._model.get_topology().get_configuration()
        defaulf_spot_path = get_conf_xpath(PATH_DEFAULT_SPOT)
        self._default_spot = int(config.get_value(config.get(defaulf_spot_path)))

    def get_default_gw_assignment(self):
        """Return the default st assignment"""
        config = self._model.get_topology().get_configuration()
        defaulf_gw_path = get_conf_xpath(PATH_DEFAULT_GW)
        self._default_gw = int(config.get_value(config.get(defaulf_gw_path)))

    ##################################################

    def get_spot_value(self, spots, spot_id):
        """Convert old group name to value"""
        i = 0
        for spot in spots:
            if spot == spot_id:
                return i
            i += 1
        return self._default_spot

    def get_gw_value(self, list_gw, gw_id):
        """Convert old group name to value"""
        i = 0
        for gw in list_gw:
            if gw == gw_id:
                return i
            i += 1
        return self._default_gw

    ##################################################

    def get_spot_str(self, value):
        """Convert value to group name """
        return "SPOT" + str(value)

    def get_gw_str(self, value):
        """Convert value to group name """
        return "GW" + str(value)

    ##################################################

    def get_id_st(self, st):
        """Return the id of ST """
        return st[-1]

    ##################################################
    def on_save_edit_clicked(self, source=None):
        """Save the ST assignment"""
        st_line = []
        st_list = self.get_st_list()

        # number a terminal by spot and gw
        nb_line_spot = {}
        nb_line_gw = {}
        # compter number of terminal by spot and gw
        cmp_line_spot = {}
        cmp_line_gw = {}
        spot_set = set()
        for st_status in st_list:
            pos_in_spot = st_status[1]
            pos_in_gw = st_status[2]
            if pos_in_spot not in nb_line_spot:
                nb_line_spot[pos_in_spot] = 1
            else:
                nb_line_spot[pos_in_spot] += 1

            if pos_in_gw not in nb_line_gw:
                nb_line_gw[pos_in_gw] = 1
            else:
                nb_line_gw[pos_in_gw] += 1

            cmp_line_spot[pos_in_spot] = 0
            cmp_line_gw[pos_in_gw] = 0

        #Get the xml to save
        topo = self._model.get_topology().get_configuration()

        # spot manager
        manager = ManageSpot(self._model)

        # TODO do that directly in TopologyConfig
        for st_status in st_list:
            st_name = st_status[0]
            pos_in_spot = st_status[1]
            pos_in_gw = st_status[2]
            #-------------#
            #     SPOT    #
            # ------------#
            spot_path = "%s/spot[@id='%s']" % (PATH_SPOT,
                                             self._spot_list[pos_in_spot])
            spot = topo.get(spot_path)

            # add not existing spot
            if spot is None and self._spot_list[pos_in_spot] not in spot_set:
                manager.add_spot(self._spot_list[pos_in_spot])

            xpath = spot_path + "/terminals"
            table = topo.get_table_elements(topo.get(xpath))

            # remove terminal's line
            if len(table) > nb_line_spot[pos_in_spot]:
                nb_remove = len(table) - nb_line_spot[pos_in_spot]
                topo.remove_line(xpath, nb_remove)
            # add terminal's line
            elif len(table) < nb_line_spot[pos_in_spot]:
                nb_add = nb_line_spot[pos_in_spot] - len(table)
                for i in range(nb_add):
                    topo.add_line(xpath)

            # update terminal value
            table = topo.get(xpath)
            topo.set_value(self.get_id_st(st_name),
                           topo.get_path(topo.get_table_elements(table)[
                               cmp_line_spot[pos_in_spot]]),
                           ID)
            cmp_line_spot[pos_in_spot] += 1
            spot_set.add(self._spot_list[pos_in_spot])

            #-------------#
            #     GW      #
            # ------------#
            terminal_path = "%s/gw[@id='%s']/terminals" % (PATH_GW,
                                             str(self._gw_list[pos_in_gw]))
            table = topo.get(terminal_path)

            # remove terminal's line
            if len(table) > nb_line_gw[pos_in_gw]:
                nb_remove = len(table) - nb_line_gw[pos_in_gw]
                topo.remove_line(terminal_path , nb_remove)
            # add terminal's line
            elif len(table) < nb_line_gw[pos_in_gw]:
                nb_add = nb_line_gw[pos_in_gw] - len(table)
                for i in range(nb_add):
                    topo.add_line(terminal_path)

            attribs = topo.get_table_elements(table)
            topo.set_value(self.get_id_st(st_name),
                           topo.get_path(attribs[cmp_line_gw[pos_in_gw]]), ID)
            cmp_line_gw[pos_in_gw] += 1

        # remove unused spots
        for spot in self._spot_list:
            if spot != str(self._default_spot) and \
               spot not in spot_set:
                manager.remove_spot(spot)

        self._model.update_spot_gw()
        topo.write()

        self._dlg.destroy()

    ##################################################

    def get_st_list(self):
        """retourn all the config to save """
        st_list = []
        child = self.vbox_st_allocation.get_children()
        for st in child:
            box = st.get_children()
            # state: (ST_ID, ACTIVE_SPOT, ACTIVE_GW)
            state = ()
            for element in box:
                try:
                    state += (element.get_text(),)
                except:
                    state += (element.get_active(),)
            if len(state) > 0:
                st_list.append(state)
        return st_list

    ##################################################

    def on_edit_dialog_delete_event(self, source=None, event=None):
        """ close and delete the window """
        self._dlg.destroy()

    ##################################################

    def on_cancel_edit_clicked(self, source=None):
        """ close and delete the window """
        self._dlg.destroy()



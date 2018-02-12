#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 TAS
# Copyright © 2018 CNES
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

# Author: Bénédicte Motto / <bmotto@toulouse.viveris.com>
# Author: Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>

"""
resource_view.py - the configuration tab view
"""

import gtk
import gobject
import re

from matplotlib.figure import Figure
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigureCanvas

from opensand_manager_core.carrier import Carrier, find_category

from opensand_manager_core.utils import get_conf_xpath, FORWARD_DOWN, RETURN_UP, \
        ROLL_OFF, CARRIERS_DISTRIB, BANDWIDTH, TAL_AFFECTATIONS, TAL_DEF_AFF, \
        TAL_ID, SYMBOL_RATE, RATIO, ACCESS_TYPE, CATEGORY, ST, SPOT, ID, GW, \
        RETURN_UP_BAND, FMT_GROUP, VCM, SCPC, FMT_GROUPS, FMT_ID, DVB_RCS2, DAMA
from opensand_manager_gui.view.utils.config_elements import SpotTree
from opensand_manager_gui.view.utils.carrier_arithmetic import CarrierArithmetic
from opensand_manager_gui.view.window_view import WindowView


class ResourceView(WindowView):
    """ Element for the resouces configuration tab """

    def __init__(self, parent, model, manager_log):
        WindowView.__init__(self, parent)

        self._init = True
        self._log = manager_log
        self._model = model
        self._spot = None
        self._gw = None
        self._tree_element = []
        self._list_carrier = {FORWARD_DOWN : [], RETURN_UP : []}
        self._desc_war = {}
        self._desc_err = {}
        self._fmt_group = {FORWARD_DOWN : {}, RETURN_UP : {}}
        self._update_spot = False
        self._return_link_std = self._model.get_conf().get_return_link_standard()
        self._rcs2_burst_length = self._model.get_conf().get_rcs2_burst_length()

        #Add graph forward
        self._graph_forward = self._ui.get_widget('scrolledwindow_forward_graph')
        self._figure_forward = Figure()
        self._ax_forward = self._figure_forward.add_subplot(111)
        canvas = FigureCanvas(self._figure_forward)
        canvas.set_size_request(200,200)
        self._graph_forward.add_with_viewport(canvas)
        self._forward_carrier_arithmetic = CarrierArithmetic(self._list_carrier[FORWARD_DOWN],
                                                             self._model, FORWARD_DOWN);
        #Add graph forward
        self._graph_return = self._ui.get_widget('scrolledwindow_return_graph')
        self._figure_return = Figure()
        self._ax_return = self._figure_return.add_subplot(111)
        canvas = FigureCanvas(self._figure_return)
        canvas.set_size_request(200,200)
        self._graph_return.add_with_viewport(canvas)
        self._return_carrier_arithmetic = CarrierArithmetic(self._list_carrier[RETURN_UP],
                                                            self._model, RETURN_UP);
        #Update graph

        #St allocation
        self._st_forward = self._ui.get_widget('vbox_forward_st_assignment')
        self._st_return = self._ui.get_widget('vbox_return_st_assignment')

        treeview = self._ui.get_widget('resource_tree')
        self._tree = SpotTree(treeview, "Spot", self.on_selection)

        # update view from model
        # do not load it in gobject.idle_add because we won't be able to catch
        # the exception
        # show first child
        self.update_view(True)


    def on_selection(self, path):
        """ on tree element selected """
        (tree, iterator) = path.get_selected()
        self._spot = None
        self._gw = None

        if iterator is not None:
            # select first child
            if len(path.get_selected_rows()[1][0]) < 2 :
                iterator = tree.iter_children(iterator)
                self._tree.select_path(tree.get_path(iterator))

            if tree.get_value(iterator, 0).lower().startswith(GW):
                self._gw = tree.get_value(iterator, 0).lower().split(GW)[1]
                if tree.get_value(tree.iter_parent(iterator),\
                                  0).lower().startswith(SPOT):
                    self._spot = tree.get_value(tree.iter_parent(iterator),\
                                                0).lower().split(SPOT)[1]
        self.update_view(True)
        pass

    def update_tree(self):
        """Update the tools tree """
        config = self._model.get_conf().get_configuration()
        xpath = "//" + RETURN_UP_BAND
        new_element = []

        # add element
        for element in config.get(xpath) :
            if element.tag == SPOT:
                spot =  SPOT + element.get(ID)
                gw =  GW + element.get(GW)
                new_element.append(spot)
                new_element.append(spot+gw)
                if spot not in self._tree_element:
                    gobject.idle_add(self._tree.add_spot, spot)
                    self._tree_element.append(spot)

                if spot + gw not in self._tree_element:
                    list_parent = []
                    list_parent.append(spot)
                    gobject.idle_add(self._tree.add_child, GW + element.get(GW),
                                     list_parent, True)
                    self._tree_element.append(spot + gw)
        # remove element
        for elm in self._tree_element:
            if elm not in new_element:
                gobject.idle_add(self._tree.del_elem, elm)
                self._tree_element.remove(elm)

        if self._init:
            first_child_path = (0,0)
            gobject.idle_add(self._tree.select_path, first_child_path)
            self._init = False
        # continue to refresh
        return True


    def update_view(self, load = False):
        """Update view """
        # Check if update required to set modcods to default
        default_modcod = False
        if self._return_link_std and self._return_link_std != self._model.get_conf().get_return_link_standard():
            self._return_link_std = self._model.get_conf().get_return_link_standard()
            default_modcod = True
        if self._return_link_std and self._return_link_std == DVB_RCS2 and \
           self._rcs2_burst_length != self._model.get_conf().get_rcs2_burst_length():
            self._rcs2_burst_length = self._model.get_conf().get_rcs2_burst_length()
            default_modcod = True

        if default_modcod:
            self.set_default_modcods()

        self.update_tree()
        if self._spot is not None and self._gw is not None:
            if load:
                self.update_carrier(FORWARD_DOWN)
            self.update_graph(FORWARD_DOWN)
            if load:
                self.update_carrier(RETURN_UP)
            self.update_graph(RETURN_UP)
            self.update_st_assignment(FORWARD_DOWN)
            self.update_st_assignment(RETURN_UP)
            if self._model.get_conf().get_payload_type() == 'regenerative':
                label = self._ui.get_widget('label_forward_title')
                label.set_text("<b>Downlink Band</b>")
                label.set_use_markup(True)
                label = self._ui.get_widget('label_return_title')
                label.set_text("<b>Uplink Band</b>")
                label.set_use_markup(True)

            else:
                label = self._ui.get_widget('label_forward_title')
                label.set_text("<b>Forward Band</b>")
                label.set_use_markup(True)
                label = self._ui.get_widget('label_return_title')
                label.set_text("<b>Return Band</b>")
                label.set_use_markup(True)
            self._ui.get_widget('vbox_resources').show_all()
        else:
            self._ui.get_widget('vbox_return').hide()
            self._ui.get_widget('vbox_forward').hide()

    def set_default_modcods(self):
        '''
        Set default modcods to all carriers of each spot and gateway
        '''
        config = self._model.get_conf().get_configuration()
        xpath = "{}_band".format(RETURN_UP)
        for elt in config.get_keys(config.get(xpath)):
            if elt.tag != SPOT:
                continue
            spot = config.get_element_content(elt)
            carrier_xpath = get_conf_xpath(CARRIERS_DISTRIB, RETURN_UP,
                                           spot[ID], spot[GW])
            fmt_gp_xpath = get_conf_xpath(FMT_GROUPS, RETURN_UP, spot[ID],
                                          spot[GW])
            for carrier in config.get_table_elements(config.get(carrier_xpath)):
                # Check return/up link carrier is DAMA
                content = config.get_element_content(carrier)
                if content[ACCESS_TYPE] != DAMA:
                    continue

                # Get FMT group id of the carrier
                fmt_group_id = content[FMT_GROUP]

                # Get default return/up link modcods
                modcods = self._model.get_conf().get_return_link_default_modcods()
                modcods_str = ';'.join(str(e[0]) for e in modcods)

                for fmt_group in config.get_table_elements(config.get(fmt_gp_xpath)):
                    # Check fmt group is this of carrier
                    fmt_content = config.get_element_content(fmt_group)
                    if fmt_content[ID] != fmt_group_id:
                        continue

                    # Set modcods to FMT group of the carrier
                    config.set_value(modcods_str, config.get_path(fmt_group), FMT_ID)
                    break
        config.write()


    def update_carrier(self, link):
        """Update carrier"""
        #clear the list and not reinstanciate it
        del self._list_carrier[link][:]
        config = self._model.get_conf()._configuration

        xpath = get_conf_xpath(BANDWIDTH, link, self._spot, self._gw)
        # bandwidth in MHz
        bandwidth = float(config.get_value(config.get(xpath))) * 1000000

        xpath = get_conf_xpath(ROLL_OFF, link)
        roll_off = float(config.get_value(config.get(xpath)))

        # fmt groups
        xpath = get_conf_xpath(FMT_GROUPS, link, self._spot, self._gw)
        for group in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(group)
            self._fmt_group[link][int(content[ID])] = content[FMT_ID]

        xpath = get_conf_xpath(CARRIERS_DISTRIB, link, self._spot, self._gw)
        total_ratio_rs = 0
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            ratios = map(lambda x: float(x),
                         re.findall(r"[\w']+", content[RATIO]))
            total_ratio_rs += float(content[SYMBOL_RATE]) * sum(ratios)
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            ratios = map(lambda x: float(x),
                         re.findall(r"[\w']+", content[RATIO]))
            nb_carrier = int(round(sum(ratios) / total_ratio_rs *\
                    bandwidth / (1 + roll_off)))
            if nb_carrier <= 0:
                nb_carrier = 1

            category = find_category(config, content[CATEGORY])
            carrier = Carrier(float(content[SYMBOL_RATE]),
                              nb_carrier, category,
                              content[ACCESS_TYPE],
                              content[FMT_GROUP],
                              ratio=content[RATIO])
            modcod = []
            for fmt_id in carrier.get_fmt_groups():
                modcod.append(self._fmt_group[link][fmt_id])

            modcods = ';'.join(str(e) for e in modcod)
            carrier.set_modcod(modcods)

            self._list_carrier[link].append(carrier)

    def update_graph(self, link):
        """Display on the graph the carrier representation"""
        #get the xml config
        config = self._model.get_conf().get_configuration()

        #get the roll off
        xpath = get_conf_xpath(ROLL_OFF, link)
        roll_off = float(config.get_value(config.get(xpath)))
        #display roll off
        if link == FORWARD_DOWN:
            self._ui.get_widget('label_forward_rolloff').set_text(
                                        'Roll_off : ' + str(roll_off))
        elif link == RETURN_UP:
            self._ui.get_widget('label_return_rolloff').set_text(
                                        'Roll_off : ' + str(roll_off))
        bandwidth = 0
        self.clear_graph(link)
        #Trace the graph
        if link == FORWARD_DOWN:
            self._forward_carrier_arithmetic.update_graph(self._ax_forward,
                                                           roll_off);
            bandwidth = self._forward_carrier_arithmetic.get_bandwidth()
            self._figure_forward.canvas.draw()
        elif link == RETURN_UP:
            self._return_carrier_arithmetic.update_graph(self._ax_return,
                                                          roll_off);
            bandwidth = self._return_carrier_arithmetic.get_bandwidth()
            self._figure_return.canvas.draw()

        #Display Bandwidth
        if link == FORWARD_DOWN:
            self._ui.get_widget('label_forward_bandwidth').set_text(
                                'Bandwidth : ' + str(bandwidth) + ' MHz')
        elif link == RETURN_UP:
            self._ui.get_widget('label_return_bandwidth').set_text(
                                'Bandwidth : ' + str(bandwidth) + ' MHz')


    def clear_graph(self, link):
        """Clear all the representaion"""
        if link == FORWARD_DOWN:
            self._ax_forward.cla()
            self._figure_forward.canvas.draw()
        else:
            self._ax_return.cla()
            self._figure_return.canvas.draw()


    def update_st_assignment(self, link):
        """Refresh the list of st assignment """
        group_list = []
        terminal_list = {}
        color = {0:'blue',
                 1:'green',
                 2:'cyan',
                 3:'magenta',
                 4:'yellow',
                 5:'black',
                 6:'red'}
        config = self._model.get_conf().get_configuration()
        host_list = self._model.get_hosts_list()

        category = { }
        category_type = config.get_simple_type("Category")
        if not category_type is None and not category_type["enum"] is None:
            i = 0
            for cat in category_type["enum"]:
                category[i] = cat
                i = i + 1

        if link == FORWARD_DOWN:
            for element in self._st_forward.get_children():
                self._st_forward.remove(element)
        elif link == RETURN_UP:
            for element in self._st_return.get_children():
                self._st_return.remove(element)

        #Get number of Group
        #TODO specify default group in the graphical_parameter gui, using a
        #     radio  button
        xpath = get_conf_xpath(TAL_AFFECTATIONS, link, self._spot, self._gw)
        defaulf_grp_path = get_conf_xpath(TAL_DEF_AFF, link,
                                          self._spot, self._gw)
        default_grp = config.get_value(config.get(defaulf_grp_path))
        default_cat = default_grp + " (default)"
        group_list.append(default_cat)

        for terminal in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(terminal)
            if content[CATEGORY] != default_grp:
                group_list.append(content[CATEGORY])
                terminal_list[content[TAL_ID]] = content[CATEGORY]

        xpath = get_conf_xpath(CARRIERS_DISTRIB, link, self._spot, self._gw)
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            if content[CATEGORY] != default_grp:
                group_list.append(content[CATEGORY])

        host_list = self._model.get_hosts_list()
        for host in host_list:
            if not host.get_name().lower().startswith(ST):
                continue
            if host.get_spot_id() != self._spot:
                continue
            if host.get_gw_id() != self._gw:
                continue
            if host.get_instance() not in terminal_list.keys():
                terminal_list[host.get_instance()] = default_cat

        if not terminal_list:
            hbox_gr_title = gtk.HBox()
            msg = "There is no ST associated with this spot and gw"
            label_color = gtk.Label("<span color='red'>%s</span>" % msg)
            label_color.set_use_markup(True)
            hbox_gr_title.pack_start(label_color,
                                     expand=False,
                                     fill=False,
                                     padding=10)

            if link == FORWARD_DOWN:
                self._st_forward.pack_start(hbox_gr_title,
                                            expand=False,
                                            fill=False)
            elif link == RETURN_UP:
                self._st_return.pack_start(hbox_gr_title,
                                           expand=False,
                                           fill=False)


        present = {k: group_list.count(k) for k in set(group_list)}
        for group in present:
            #create group field
            hbox_gr = gtk.HBox()
            evt = gtk.EventBox()
            #Create field for group title
            hbox_gr_title = gtk.HBox()
            hbox_gr.pack_start(hbox_gr_title, expand=False, fill=False)
            #Color for the group
            color_id = 6
            for key, value in category.items():
                if group.startswith(value):
                    color_id = key
                    break
            label_color = gtk.Label("<span background='%s'>     </span>" %
                                    color[color_id])
            label_color.set_use_markup(True)
            #Name of the group
            label_gr = "<b>Category %s</b>   " % group
            hbox_gr_title.pack_start(label_color,
                                     expand=False,
                                     fill=False,
                                     padding=10)

            # compter category scpc carrier and terminals
            nb_carrier_scpc = 0
            is_vcm_carriers = False
            nb_carrier = 0
            nb_tal_scpc = 0
            nb_tal = 0

            #Add all the host with the same group
            label_st = ""
            for host in host_list:
                if not host.get_name().lower().startswith(ST):
                    continue
                if host.get_instance() in terminal_list.keys() and \
                   terminal_list[host.get_instance()] == group:
                    label_st += host.get_name().upper() + " "
                    adv = host.get_advanced_conf()
                    xpath = "//dvb_rcs_tal/is_scpc"
                    tal_scpc = adv.get_configuration().get(xpath)
                    if tal_scpc is not None :
                        nb_tal += 1
                        if adv.get_configuration().get_value(tal_scpc) == "true":
                            nb_tal_scpc += 1

            expand_rate = gtk.Expander(label = (label_gr + label_st))
            expand_rate.set_use_markup(True)
            carrier_rate = "";
            carrier_arithmetic = None
            if link == FORWARD_DOWN:
                carrier_arithmetic = self._forward_carrier_arithmetic
            elif link == RETURN_UP:
                carrier_arithmetic = self._return_carrier_arithmetic
            carrier_arithmetic.update_rates(self._fmt_group[link]);

            for element in self._list_carrier[link]:
                if element.get_old_category(config) in group:
                    nb_carrier += element.get_nb_carriers()
                    if element.get_access_type() == SCPC:
                        nb_carrier_scpc += element.get_nb_carriers()
                    elif element.get_access_type() == VCM:
                        is_vcm_carriers = True
                    for (min_rate, max_rate) in element.get_rates():
                        carrier_rate += "%d carrier(s) rate: [%d, %d] kb/s\n" \
                                % (element.get_nb_carriers(), min_rate /
                                   1000, max_rate / 1000)
            total_rate = "Total rate: [%d, %d] kb/s" % \
                    (carrier_arithmetic.get_min_bitrate() / 1000,
                     carrier_arithmetic.get_max_bitrate() / 1000)
            label_rate = gtk.Label(carrier_rate + total_rate);
            label_rate.set_justify(gtk.JUSTIFY_LEFT)
            label_rate.set_alignment(0, 0.5)
            expand_rate.add(label_rate)
            evt.add(expand_rate)
            evt.modify_bg(gtk.STATE_NORMAL, gtk.gdk.Color(0xffff, 0xffff, 0xffff))
            hbox_gr_title.pack_start(evt,
                                     expand=False,
                                     fill=False,
                                     padding=10)

            # show warning
            if nb_carrier == 0 and nb_tal > 0:
                img_err = gtk.Image()
                img_err.set_from_stock(gtk.STOCK_DIALOG_ERROR,
                                       gtk.ICON_SIZE_MENU)
                self._desc_err[group] = img_err
                hbox_gr_title.pack_start(img_err, expand=True, fill=False, padding=1)
                self._desc_err[group].set_tooltip_text(
                    "There is no more carrier for terminal. Please, reassign it")
            elif link == RETURN_UP and nb_tal_scpc >= 1 and nb_carrier_scpc == 0:
                img_war = gtk.Image()
                img_war.set_from_stock(gtk.STOCK_DIALOG_WARNING,
                                       gtk.ICON_SIZE_MENU)
                self._desc_war[group] = img_war
                hbox_gr_title.pack_start(img_war, expand=True, fill=False, padding=1)
                self._desc_war[group].set_tooltip_text(
                    "There is no carrier for SCPC terminal")
            elif link == RETURN_UP and nb_tal_scpc > nb_carrier_scpc or \
               (nb_tal >= (nb_tal_scpc + 1)  and \
                nb_carrier < (nb_carrier_scpc + 1)):
                img_war = gtk.Image()
                img_war.set_from_stock(gtk.STOCK_DIALOG_WARNING,
                                   gtk.ICON_SIZE_MENU)
                self._desc_war[group] = img_war
                hbox_gr_title.pack_start(img_war, expand=True, fill=False, padding=1)
                self._desc_war[group].set_tooltip_text(
                    "There should be at most one SCPC terminal per category, it "
                    "will use all the SCPC carriers in it")
            elif link == FORWARD_DOWN and is_vcm_carriers:
                img_war = gtk.Image()
                img_war.set_from_stock(gtk.STOCK_DIALOG_WARNING,
                                       gtk.ICON_SIZE_MENU)
                self._desc_war[group] = img_war
                hbox_gr_title.pack_start(img_war, expand=True, fill=False, padding=1)
                self._desc_war[group].set_tooltip_text(
                    "Be careful to configure the relevant GW fifos for VCM "
                    "carriers")

            #Add the new group in window
            if link == FORWARD_DOWN:
                self._st_forward.pack_start(hbox_gr,
                                            expand=False,
                                            fill=False)
            elif link == RETURN_UP:
                self._st_return.pack_start(hbox_gr,
                                           expand=False,
                                           fill=False)

        self._ui.get_widget('vbox_resources').show_all()

    def update_scenario(self):
        self._return_link_std = None
        self.update_view(True)


    def is_modified(self):
        """ check if the configuration was modified by user
            (used in callback so no need to use locks) """
        try:
            if self._update_spot:
                return True

            config = self._model.get_conf()
            # payload_type
            widget = self._ui.get_widget(config.get_payload_type())
            if not widget.get_active():
                return True
            # emission_std
            widget = self._ui.get_widget(config.get_emission_std())
            if not widget.get_active():
                return True

            # return_up_encap
            if self._out_stack.get_stack() != config.get_return_up_encap():
                return True

            # forward_down_encap
            if self._in_stack.get_stack() != config.get_forward_down_encap():
                return True

        except:
            raise

        return False

    def on_stack_modif(self, source=None, event=None):
        """ 'changed' event on a combobox from the stack """
        self.enable_conf_buttons()

    def on_lan_stack_modif(self, source=None, event=None):
        """ 'changed' event on a combobox from the stack """
        # we need to check because buttons are modified when loading stack
        if self.is_lan_adapt_stack_modif():
            self.enable_conf_buttons()



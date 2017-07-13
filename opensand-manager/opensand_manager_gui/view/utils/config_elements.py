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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
config_elements.py - create configuration elements according to their types
"""

import gtk
import gobject
import os
import pango
from copy import deepcopy

from opensand_manager_core.utils import GW, GLOBAL, SPOT, ID, \
                                        TOPOLOGY, BANDWIDTH, SYMBOL_RATE,\
                                        ROLL_OFF, RATIO, SATDELAY, SATDELAY_CONF, \
                                        SATDELAY_TYPE, PATH_SATDELAY_CONF_MODULE, \
                                        to_underscore
from opensand_manager_core.my_exceptions import XmlException
from opensand_manager_gui.view.utils.config_entry import ConfEntry
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.popup.edit_dialog import EditDialog
from opensand_manager_gui.view.popup.edit_sequence_dialog import EditSequenceDialog

(TEXT, VISIBLE_CHECK_BOX, CHECK_BOX_SIZE, ACTIVE, \
 ACTIVATABLE, VISIBLE, RESTRICTED) = range(7)
(DISPLAYED, NAME, PROBE_ID, SIZE) = range(4)


class ProbeSelectionController(object):
    """ The program/probe list controller """
    def __init__(self, probe_view, program_listview, probe_listview):
        self._probe_view = probe_view
        self._program_listview = program_listview
        self._probe_listview = probe_listview
        self._collection_dialog = None
        self._program_list = {}
        self._current_program = None
        self._update_needed = False
        self._toggled = {}

        self._program_store = gtk.ListStore(str, int)
        self._program_store.set_sort_column_id(0, gtk.SORT_ASCENDING)
        program_listview.set_model(self._program_store)

        column = gtk.TreeViewColumn("Program", gtk.CellRendererText(), text=0)
        column.set_sort_column_id(0)
        program_listview.append_column(column)
        program_listview.connect('cursor-changed', self._prog_curs_changed)

        # The probe tree store uses four columns:
        # 0) bool: is the probe displayed ? (False if the row is not a probe)
        # 1) str: the probe name, or section name
        # 2) int: the probe ID (0 if the row is not a probe)
        # 3) int: the checkbox size (used to hide the checkbox on sections while
        #         keeping the alignment)
        # The tree view itself uses one column, with two renderers (checkbox
        # and text)
        self._probe_store = gtk.TreeStore(gobject.TYPE_BOOLEAN,
                                          gobject.TYPE_STRING,
                                          gobject.TYPE_INT,
                                          gobject.TYPE_INT)

        self._probe_store.set_sort_column_id(1, gtk.SORT_ASCENDING)
        self._probe_listview.set_model(self._probe_store)
        self._probe_listview.get_selection().set_mode(gtk.SELECTION_NONE)
        self._probe_listview.set_enable_tree_lines(True)


        column = gtk.TreeViewColumn("Probe")
        column.set_sort_column_id(NAME) # Sort on the probe/section name
        self._probe_listview.append_column(column)

        cell_renderer = gtk.CellRendererToggle()
        column.pack_start(cell_renderer, False)
        column.add_attribute(cell_renderer, "active", DISPLAYED)
        column.add_attribute(cell_renderer, "indicator-size", SIZE)
        cell_renderer.connect("toggled", self._probe_toggled)

        cell_renderer = gtk.CellRendererText()
        column.pack_start(cell_renderer, True)
        column.add_attribute(cell_renderer, "text", NAME)

    def clear_selection(self):
        """ clear selection """
        for program in self._program_list.itervalues():
            self._toggled[program.name] = []
            for probe in program.get_probes():
                try:
                    probe.displayed = False
                except ValueError, msg:
                    error_popup(str(msg))
        self._probe_store.foreach(self.unselect)
        self.probe_displayed_change()

    def unselect(self, model, path, iter):
        """ unselect a statistic """
        self._probe_store.set_value(iter, DISPLAYED, False)

    def register_collection_dialog(self, collection_dialog):
        """ register a collection dialog """
        self._collection_dialog = collection_dialog
        if collection_dialog:
            collection_dialog.update_list(self._program_list)

    def update_data(self, program_list):
        """ called when the probe list changes """
        self._program_list = program_list
        self._update_needed = True

        # set the previous toggled probes
        for program in self._program_list.itervalues():
            # first update toggle dict  if necessary
            if not program.name in self._toggled:
                self._toggled[program.name] = []
            for probe in program.get_probes():
                if probe.name in self._toggled[program.name]:
                    try:
                        probe.displayed = True
                    except ValueError:
                        # the probe is not available for this scenario
                        pass

        gobject.idle_add(self._update_data)

    def _update_data(self):
        """ update the current program list """
        if not self._update_needed:
            return

        self._update_needed = False

        self._program_store.clear()

        for program in self._program_list.itervalues():
            self._program_store.append([program.name,
                                        program.ident])

        selection = self._program_listview.get_cursor()
        if selection[0] is None:
            self._program_listview.set_cursor(0)

        self._update_probe_list()
        self.probe_displayed_change()

        if self._collection_dialog:
            self._collection_dialog.update_list(self._program_list)

    def _prog_curs_changed(self, _):
        """ called when the user selects a program in the list """
        self._update_probe_list()

    def _update_probe_list(self):
        """ called when the displayed probe list need to be updated """
        selection = self._program_listview.get_cursor()
        self._probe_store.clear()
        if selection[0] is None:
            self._current_program = None
            return

        it = self._program_store.get_iter(selection[0])
        prog_ident = self._program_store.get_value(it, NAME)

        self._current_program = self._program_list[prog_ident]

        groups = {}
        # Used to keep track of created probe groups in the tree view.
        # For instance, probe a.b.c.d will create three recursive tree paths,
        # stored respectively at groups['a'][''], groups['a']['b'][''],
        # and groups['a']['b']['c']

        for probe in self._current_program.get_probes():
            if probe.enabled:
                probe_parent = None
                cur_group = groups
                probe_path = probe.disp_name.split(".")
                probe_name = probe_path.pop().strip(" _")

                while probe_path:
                    group_name = probe_path.pop(0).strip(" _")
                    try:
                        cur_group = cur_group[group_name]
                    except KeyError:
                        # The needed tree path does not exist -- create it and
                        # continue

                        cur_group[group_name] = {
                            '': self._probe_store.append(probe_parent,
                                                         [False, group_name,
                                                          0, 0])
                        }

                        cur_group = cur_group[group_name]

                    probe_parent = cur_group['']

                self._probe_store.append(probe_parent, [probe.displayed,
                                                        probe_name,
                                                        probe.ident,
                                                        12])

    def _probe_toggled(self, _, path):
        """ called when the user selects or deselects a probe """
        it = self._probe_store.get_iter(path)
        probe_ident = self._probe_store.get_value(it, PROBE_ID)
        new_value = not self._probe_store.get_value(it, DISPLAYED)
        # this is a parent, expand or collapse it
        if self._probe_store.iter_has_child(it):
            if self._probe_listview.row_expanded(path):
                self._probe_listview.collapse_row(path)
            else:
                self._probe_listview.expand_row(path, False)
            return

        # if the probe is selected keep it to reselect it when the list will
        # be refreshed on restart
        probe = self._current_program.get_probe(probe_ident)
        if new_value:
            if not probe.name in self._toggled[self._current_program.name]:
                self._toggled[self._current_program.name].append(probe.name)
        elif probe.name in self._toggled[self._current_program.name]:
            self._toggled[self._current_program.name].remove(probe.name)

        try:
            probe.displayed = new_value
        except ValueError, msg:
            error_popup(str(msg))

        self._probe_store.set(it, DISPLAYED, new_value)

        self.probe_displayed_change()

    def probe_enabled_changed(self, probe, was_hidden):
        """ called when the enabled status of a probe is changed """
        if probe.program == self._current_program:
            self._update_probe_list()

        if was_hidden:
            self.probe_displayed_change()

    def probe_displayed_change(self):
        """ notifies the main view of the currently displayed probes """
        displayed_probes = []

        for program in self._program_list.itervalues():
            for probe in program.get_probes():
                if probe.displayed:
                    displayed_probes.append(probe)

        self._probe_view.displayed_probes_changed(displayed_probes)


class SpotTree(gtk.TreeStore):
    """ the OpenSAND configuration view tree """
    def __init__(self, treeview, col1_title, col1_changed_cb):
        # create a treestore with 1property
        # - text: the text of the 1st column
        gtk.TreeStore.__init__(self, gobject.TYPE_STRING,
                                     gobject.TYPE_BOOLEAN)

        self._treeselection = None
        self._cell_renderer_toggle = None
        
        # filter to hide row
        # attach store to the filter
        self._filter_visible = self.filter_new()
        self._filter_visible.set_visible_column(1)
        self._treeview = treeview 
        self._treeview.set_model(self._filter_visible)
        self._hidden_row = []
        self._restriction_row = []
        
        # load the treestore
        column = gtk.TreeViewColumn(col1_title)
        cell_renderer = gtk.CellRendererText()
        column.pack_start(cell_renderer, True)
        column.add_attribute(cell_renderer, "text", TEXT)
        column.add_attribute(cell_renderer, "visible", 1)
        
        self._treeview.append_column(column)

        # add a column to avoid large toggle column
        col = gtk.TreeViewColumn(' ')
        self._treeview.append_column(col)

        # get the tree selection
        self._treeselection = self._treeview.get_selection()
        self._treeselection.set_mode(gtk.SELECTION_SINGLE)
        if col1_changed_cb is not None:
            self._treeselection.connect('changed', col1_changed_cb)

    def add_spot(self, name, elt_info=None):
        """ add a host with its elements in the treeview """
        top_elt = self.append(None)
        self.set(top_elt, TEXT, name,
                 1, True)

    def add_child(self, name, parent_name, parents=False):
        """ add a module in the module tree """
        top_elt = None
        if parents:
            top_elt = self.get_parent(parent_name)

        # set top element, either child  or type depending on parents
        if top_elt is None:
            top_elt = self.append(None)
            top_name = name
            if parents:
                it = len(parent_name) -1
                top_name = parent_name[it]
            self.set(top_elt, TEXT, top_name,
                     1, True)

        if parents:
            sub_iter = self.append(top_elt)
            self.set(sub_iter, TEXT, name,
                     1, True)


    def del_elem(self, name):
        """ remove a host from the treeview """
        iterator = self.get_parent([name])
        if iterator is not None:
            self.remove(iterator)

    def get_parent(self, name):
        """ get a parent in the treeview """
        iterator = self.get_iter_first()
        last_it = iterator
        path_iter = 0
        while iterator is not None :
            if self.get_value(iterator, TEXT).lower() == name[path_iter].lower():
                if path_iter == len(name)-1:
                    return iterator
                path_iter += 1
                iterator = self.iter_children(iterator)

            if last_it == iterator:
                iterator = self.iter_next(iterator)
                last_it = iterator
            else:
                last_it = iterator
        return iterator
   
    def get_selection(self):
        """ get the treeview selection """
        return self._treeselection

    def get_treeview(self):
        """ get the treeview """
        return self._treeview

    def get_spot_gw(self, iterator, spot, gw):
        if self.get_value(iterator, TEXT).lower().startswith(GW):
            gw = self.get_value(iterator, TEXT).lower().split(GW)[1]
            if self.get_value(iterator.get_parent(),\
                              TEXT).lower().startswith(SPOT):
                spot = self.get_value(iterator.get_parent(),\
                                      TEXT).lower().split(SPOT)[1]
    def select_path(self, path):
        self._treeview.expand_to_path(path)
        self._treeselection.select_path(path)

class ConfigurationTree(gtk.TreeStore):
    """ the OpenSAND configuration view tree """
    def __init__(self, treeview, col1_title, col1_changed_cb, col1_toggled, adv_mode=None):
        # create a treestore with 7 properties
        # - text: the text of the 1st column
        # - visible: is the check box visible
        # - int: the checkbox size (used to hide the checkbox on sections while
        #        keeping the alignment)
        # - activate : the check box s activate 
        # - activatable: can we activate the check box of the 2nd column
        # - visible: the element is visible
        # - restricted: the element is restricted
        gtk.TreeStore.__init__(self, gobject.TYPE_STRING,
                                     gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_INT,
                                     gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_BOOLEAN,
                                     gobject.TYPE_BOOLEAN)

        self._treeselection = None
        self._cell_renderer_toggle = None
        if adv_mode == None:
            self._adv_mode = True
        else:
            self._adv_mode = adv_mode
        
        # filter to hide row
        # attach store to the filter
        self._filter_visible = self.filter_new()
        self._filter_visible.set_visible_column(4)
        self._treeview = treeview 
        self._treeview.set_model(self._filter_visible)
        self._hidden_row = []
        self._restriction_row = []
        
        #load the treestore 
        column = gtk.TreeViewColumn(col1_title)
        if not col1_toggled is None:
            cell_renderer = gtk.CellRendererToggle()
            column.pack_start(cell_renderer, False)
            column.set_resizable(True)
            column.add_attribute(cell_renderer, "visible", VISIBLE_CHECK_BOX)
            column.add_attribute(cell_renderer, "active", ACTIVE)
            column.add_attribute(cell_renderer, "indicator-size", CHECK_BOX_SIZE) 
            column.add_attribute(cell_renderer, "activatable", ACTIVATABLE)
            cell_renderer.connect("toggled", col1_toggled)
        
        cell_renderer = gtk.CellRendererText()
        column.pack_start(cell_renderer, True)
        column.add_attribute(cell_renderer, "text", TEXT)
        column.add_attribute(cell_renderer, "visible", VISIBLE)
        
        self._treeview.append_column(column)

        # add a column to avoid large toggle column
        col = gtk.TreeViewColumn('')
        self._treeview.append_column(col)

        # get the tree selection
        self._treeselection = self._treeview.get_selection()
        self._treeselection.set_mode(gtk.SELECTION_SINGLE)
        if col1_changed_cb is not None:
            self._treeselection.connect('changed', col1_changed_cb)

    def add_host(self, host, elt_info=None):
        """ add a host with its elements in the treeview """
        name = host.get_name()
        # append an element in the treestore
        # first Global, next SAT, then GW and ST
        if name == GLOBAL:
            top_elt = self.insert(None, 0)
        elif name == TOPOLOGY:
            top_elt = self.insert(None, 1)
        else:
            top_elt = self.append(None)

        # for tools and global in advanced configuration
        if elt_info is not None:
            self.set(top_elt, TEXT, name.upper(),
                              VISIBLE_CHECK_BOX, False,
                              CHECK_BOX_SIZE, 1,
                              ACTIVE, False,
                              ACTIVATABLE, True,
                              VISIBLE, True, 
                              RESTRICTED, False)
            for sub_name in elt_info.keys():
                activatable = True
                sub_iter = self.append(top_elt)
                # for tools or probes wihtout substatistic
                if elt_info[sub_name] is None:
                    activatable = False
                if not isinstance(elt_info[sub_name], list):
                    self.set(sub_iter, TEXT, sub_name,
                                       VISIBLE_CHECK_BOX, self._adv_mode,
                                       CHECK_BOX_SIZE, 12,  
                                       ACTIVE, False,
                                       ACTIVATABLE, activatable,
                                       VISIBLE, True, 
                                       RESTRICTED, False)
        else:
            # for advanced host
            # only set host activatable if developper mode is enabled
            if host.get_state() is None:
                activatable = False
            active = host.is_enabled()
            self.set(top_elt, TEXT, name.upper(),
                              VISIBLE_CHECK_BOX, self._adv_mode,
                              CHECK_BOX_SIZE, 12,
                              ACTIVE, active,
                              ACTIVATABLE, True,
                              VISIBLE, True,
                              RESTRICTED, False)
        

    def add_child(self, name, parent_name, hidden, parents=False):
        """ add a module in the module tree """
        top_elt = None
        if parents:
            top_elt = self.get_parent(parent_name)

        # set top element, either child  or type depending on parents
        if top_elt is None:
            top_elt = self.append(None)
            top_name = name
            if parents:
                it = len(parent_name) -1
                top_name = parent_name[it]
            self.set(top_elt, TEXT, top_name,
                     VISIBLE_CHECK_BOX, False,
                     CHECK_BOX_SIZE, 1,
                     ACTIVE, False,
                     ACTIVATABLE, True,
                     VISIBLE, True ,
                     RESTRICTED, False)

        if parents:
            sub_iter = self.append(top_elt)
            self.set(sub_iter, TEXT, name,
                     VISIBLE_CHECK_BOX, False,
                     CHECK_BOX_SIZE, 1,
                     ACTIVE, False,
                     ACTIVATABLE, not hidden,
                     VISIBLE , not hidden,
                     RESTRICTED, False)
            if hidden :
                self._hidden_row.append(sub_iter)


    def add_module(self, module, parents=False):
        """ add a module in the module tree """
        name = module.get_name()

        top_elt = None
        if parents:
            top_elt = self.get_parent([module.get_type()])

        # set top element, either module or type depending on parents
        if top_elt is None:
            top_elt = self.append(None)
            top_name = name
            if parents:
                top_name = module.get_type()
            self.set(top_elt, TEXT, top_name,
                              VISIBLE_CHECK_BOX, False,
                              ACTIVE, False,
                              ACTIVATABLE, True,
                              VISIBLE, True,
                              RESTRICTED, False)

        if parents:
            sub_iter = self.append(top_elt)
            self.set(sub_iter, TEXT, name,
                               VISIBLE_CHECK_BOX, False,
                               ACTIVE, False,
                               ACTIVATABLE, True,
                               VISIBLE, True,
                               RESTRICTED, False)

    def del_elem(self, name):
        """ remove a host from the treeview """
        iterator = self.get_parent([name])
        if iterator is not None:
            self.remove(iterator)

    def get_parent(self, name):
        """ get a parent in the treeview """
        iterator = self.get_iter_first()
        last_it = iterator
        path_iter = 0
        while iterator is not None :
            if self.get_value(iterator, TEXT).lower() == name[path_iter].lower():
                if path_iter == len(name)-1:
                    return iterator
                path_iter += 1
                iterator = self.iter_children(iterator)

            if last_it == iterator:
                iterator = self.iter_next(iterator)
                last_it = iterator
            else:
                last_it = iterator
        return iterator
   
    def set_hidden(self, hidden):
        for row in self._hidden_row:
            path = self.get_path(row)
            #visible is not (restricted or hide)
            self[path][VISIBLE] = not hidden 
            self[path][ACTIVATABLE] = not hidden
        # show all restiction
        if not hidden :
            for row in self._restriction_row:
                path = self.get_path(row)
                #visible is not (restricted or hide)
                self[path][VISIBLE] = not hidden 
                self[path][ACTIVATABLE] = not hidden
        
        iterator = self.get_iter_first()
        self.is_children_visible(iterator)

    def set_restricted(self, rows):
        del self._restriction_row[:]
        for row in rows:
            path = row.split('.')
            iterator = self.get_parent(path)
            if iterator is not None:
                self._restriction_row.append(iterator)
                xpath = self.get_path(iterator)
                self[xpath][RESTRICTED] = rows[row]
                #visible is not restricted and not hide (visible)
                self[xpath][VISIBLE] = not self[xpath][RESTRICTED]
                self[xpath][ACTIVATABLE] = not self[xpath][RESTRICTED]
        
        iterator = self.get_iter_first()
        self.is_children_visible(iterator)
                
    def is_children_visible(self, iterator, child=False):
        list_vis_child = []
        while iterator is not None:
            path = self.get_path(iterator)
            if not self[path][VISIBLE] and child:
                iterator = self.iter_next(iterator)
                continue
            if self.iter_has_child(iterator):
                first_child = self.iter_children(iterator)
                visible = self.is_children_visible(first_child, True)
                list_vis_child.append(visible)
                self[path][VISIBLE] = visible
                self[path][ACTIVATABLE] = visible
            else:
                list_vis_child.append(self[path][VISIBLE])
            iterator = self.iter_next(iterator)
        if True in list_vis_child:
            return True
        else: 
            return False
            
    def get_selection(self):
        """ get the treeview selection """
        return self._treeselection

    def get_treeview(self):
        """ get the treeview """
        return self._treeview


class ConfigurationNotebook(gtk.Notebook):
    """ the OpenSAND configuration view elements """
    def __init__(self, config, host, adv_mode, scenario, show_hidden, changed_cb, file_cb):
        gtk.Notebook.__init__(self)

        self._current_page = 0
        self._sections = []

        self.set_scrollable(True)
        self.set_tab_pos(gtk.POS_LEFT)
        self.connect('show', self.on_show)
        self.connect('hide', self.on_hide)

        for section in config.get_sections():
            conf_section = ConfSection(section, config, host, adv_mode,
                                       scenario, changed_cb, file_cb)
            if self.add_section(config, section,
                                conf_section, adv_mode):
                self._sections.append(conf_section)

        self.set_hidden(not show_hidden)

    def add_section(self, config, section, conf_section, adv_mode):
        """ add a section in the notebook and return the associated vbox """
        name = config.get_name(section)
        if config.do_hide_adv(name, adv_mode):
            return False
        scroll_notebook = gtk.ScrolledWindow()
        scroll_notebook.set_policy(gtk.POLICY_AUTOMATIC,
                                   gtk.POLICY_AUTOMATIC)
        scroll_notebook.add_with_viewport(conf_section)
        tab_label = gtk.Label()
        tab_label.set_justify(gtk.JUSTIFY_CENTER)
        tab_label.set_markup("<small><b>%s</b></small>" % name)
        self.append_page(scroll_notebook, tab_label)
        if config.do_hide(name):
            # the section itself is hidden
            conf_section.add_hidden(scroll_notebook)
        restriction = config.get_xpath_restrictions(name)
        if restriction is not None:
            conf_section.add_restriction(scroll_notebook, restriction)
        return True

    def set_hidden(self, val):
        """ change the hidden status """
        for section in self._sections:
            section.set_hidden(val)

    def save(self):
        """ save the configuration """
        for section in self._sections:
            section.save()

    def on_show(self, widget):
        """ notebook shown """
        self.set_current_page(self._current_page)

    def on_hide(self, widget):
        """ notebook hidden """
        self._current_page = self.get_current_page()

    def get_restrictions(self):
        """ get the restrictions in sections """
        restrictions = {}
        for section in self._sections:
            restrictions.update(section.get_restrictions())
        return restrictions

    def set_restrictions(self, restrictions):
        """ set the hidden widgets """
        for (widget, val) in restrictions.items():
            # enable hide_all and show_all actions on this widget
            widget.set_no_show_all(False)
            if val:
                widget.hide_all()
            else:
                widget.show_all()
            # disable hide_all and show_all actions on this widget
            # to avoid modifications from outside
            widget.set_no_show_all(True)

class ConfSection(gtk.VBox):
    """ a section in the configuration """
    def __init__(self, section, config, host, adv_mode, scenario,
                 changed_cb, file_cb, spot_id = None, gw_id = None,
                 modules = None):
        gtk.VBox.__init__(self)

        self._config = config
        self._host = host
        self._changed = []
        self._changed_cb = changed_cb
        self._file_cb = file_cb
        self._scenario = scenario
        self._adv_mode = adv_mode
        self._spot_id =  spot_id
        self._gw_id =  gw_id
        # keep ConfEntry objects else we sometimes loose their attributes in the
        # event callback
        self._entries = []

        # list of tables with format: {table path : [check button per line]}
        self._tables = {}
        # list of table length
        self._table_length = {}
        # list of tables with format: {table path : format}
        self._table_models = {}
        # list of add buttons
        self._add_buttons = []
        # list of delete buttons
        self._del_buttons = []
        # list of added lines
        self._new = []
        # list of removed lines
        self._removed = []
        # list of hidden elements
        self._hidden_widgets = []
        # dict of restriction per widget
        self._restrictions = {}
        # list of completions
        self._completions = []
        # bandwidth key
        self._bandwidth_entry = None
        self._roll_off = 0
        self._symbol_rates = []
        self._ratios = []

        # host modules
        self._modules = modules 

        self.fill(section)
        self.set_hidden(True)

    def set_spot_id(self, spot_id):
        self._spot_id = spot_id

    def get_restrictions(self):
        """ get the restrictions """
        return self._restrictions

    def set_restrictions(self, restrictions):
        """ set the hidden widgets """
        #restrictions.update(self._restrictions)
        for (widget, val) in restrictions.items():
            # enable hide_all and show_all actions on this widget
            widget.set_no_show_all(False)
            if val:
                widget.hide_all()
            else:
                widget.show_all()
            # disable hide_all and show_all actions on this widget
            # to avoid modifications from outside
            widget.set_no_show_all(True)

    def set_hidden(self, val):
        """ change the hidden status """
        for widget in self._hidden_widgets:
            # enable hide_all and show_all actions on this widget
            widget.set_no_show_all(False)
            if val:
                widget.hide_all()
            else:
                widget.show_all()
            # disable hide_all and show_all actions on this widget
            # to avoid modifications from outside
            widget.set_no_show_all(True)

    def fill(self, section):
        """ get the section content and fill the corresponding tab """
        # first add the section description
        name = self._config.get_name(section)
        description = self._config.get_documentation(name)
        if description != None:
            description = description.split("\n", 1)
            if len(description) > 1:
                section_descr = gtk.Expander(label=description[0])
                section_descr.set_use_markup(True)
                text = gtk.Label()
                text.set_markup(description[1])
                text.set_justify(gtk.JUSTIFY_LEFT)
                text.set_alignment(0, 0.5)
                section_descr.add(text)
            else:
                section_descr = gtk.Label(description[0])
                section_descr.set_markup(description[0])
                section_descr.set_justify(gtk.JUSTIFY_LEFT)
                section_descr.set_alignment(0, 0.5)

            evt = gtk.EventBox()
            evt.add(section_descr)
            evt.modify_bg(gtk.STATE_NORMAL, gtk.gdk.Color(0xffff, 0xffff, 0xffff))
            self.pack_start(evt)
            self.set_child_packing(evt, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_START)
        
        for key in self._config.get_keys(section):
            if self._spot_id is not None or self._gw_id is not None:
                if key.tag == SPOT:
                    if self._spot_id == key.get(ID) and \
                       (key.get(GW) is None or self._gw_id == key.get(GW)):
                        for s_key in self._config.get_keys(key):
                            if self._config.is_table(s_key):
                                table = self.add_table(s_key)
                                if table is not None:
                                    self.pack_end(table)
                                    self.set_child_packing(table, expand=False,
                                                           fill=False, padding=5,
                                                           pack_type=gtk.PACK_START)
                            else:
                                entry = self.add_key(s_key)
                                if entry is not None:
                                    self.pack_end(entry)
                                    self.set_child_packing(entry, expand=False,
                                                           fill=False, padding=5,
                                                           pack_type=gtk.PACK_START)
                elif key.tag == GW:
                    if self._gw_id == key.get(ID):
                        for s_key in self._config.get_keys(key):
                            if self._config.is_table(s_key):
                                table = self.add_table(s_key)
                                if table is not None:
                                    self.pack_end(table)
                                    self.set_child_packing(table, expand=False,
                                                           fill=False, padding=5,
                                                           pack_type=gtk.PACK_START)
                            else:
                                entry = self.add_key(s_key)
                                if entry is not None:
                                    self.pack_end(entry)
                                    self.set_child_packing(entry, expand=False,
                                                           fill=False, padding=5,
                                                           pack_type=gtk.PACK_START)
          
            else:
                if key.tag == SPOT or key.tag == GW:
                    continue
                elif self._config.is_table(key):
                    table = self.add_table(key)
                    if table is not None:
                        self.pack_end(table)
                        self.set_child_packing(table, expand=False,
                                               fill=False, padding=5,
                                               pack_type=gtk.PACK_START)
                else:
                    entry = self.add_key(key)
                    if entry is not None:
                        self.pack_end(entry)
                        self.set_child_packing(entry, expand=False,
                                              fill=False, padding=5,
                                              pack_type=gtk.PACK_START)

    def add_key(self, key):
        """ add a key and its corresponding entry in a tab """
        name = self._config.get_name(key)
        if self._config.do_hide_adv(name, self._adv_mode):
            return None
        key_box = gtk.HBox()
        key_label = gtk.Label()
        key_label.set_ellipsize(pango.ELLIPSIZE_END)
        key_label.set_markup(name)
        key_label.set_alignment(0.0, 0.5)
        key_label.set_width_chars(30)
        description = self._config.get_documentation(name)
        key_box.pack_start(key_label)
        key_box.set_child_packing(key_label, expand=False,
                                  fill=False, padding=5,
                                  pack_type=gtk.PACK_START)
        self.add_description(key_box, description)
        elt_type = self._config.get_type(name)
        source = self._config.get_file_source(name)
        if source is not None:
            source = self._config.adapt_filename(source, key)
            scenario = self._scenario
            if self._host != GLOBAL:
                scenario = os.path.join(self._scenario, self._host)
            source = os.path.join(scenario, source)
    
        entry = ConfEntry(self._config.get_type(name),
                          self._config.get_value(key),
                          self._config.get_path(key),
                          source,
                          self._host,
                          [self.handle_param_changed, self._changed_cb],
                          self._file_cb)
        self._entries.append(entry)

        key_box.pack_start(entry.get())
        key_box.set_child_packing(entry.get(), expand=False,
                                  fill=False, padding=5,
                                  pack_type=gtk.PACK_START)
        unit = self._config.get_unit(name)
        if unit is not None:
            unit_label = gtk.Label()
            unit_label.set_markup("<i> %s</i>" % unit)
            key_box.pack_start(unit_label)
            key_box.set_child_packing(unit_label, expand=False,
                                      fill=False, padding=2,
                                      pack_type=gtk.PACK_START)
        if self._config.do_hide(name):
            self._hidden_widgets.append(key_box)
        
        restriction = self._config.get_xpath_restrictions(name)
        if restriction is not None:
            self._restrictions.update({key_box: restriction})

        if name == BANDWIDTH:
            self._bandwidth_entry = entry
        return key_box

    def add_key_sequence(self, key):
        """ add a key sequence in the tab """
        name = self._config.get_name(key)
        if self._config.do_hide_adv(name, self._adv_mode):
            return None
        seq_frame = gtk.Frame()
        seq_frame.set_label_align(0, 0.5)
        seq_frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        alignment = gtk.Alignment(0.5, 0.5, 1, 1)
        seq_frame.add(alignment)
        seq_label = gtk.HBox()
        label_text = gtk.Label()
        label_text.set_markup("<b>%s</b>" % name)
        description = self._config.get_documentation(name)
        seq_label.pack_start(label_text)
        seq_label.set_child_packing(label_text, expand=False,
                                    fill=False, padding=5,
                                    pack_type=gtk.PACK_START)
        self.add_description(seq_label, description)
        seq_frame.set_label_widget(seq_label)
        align_vbox = gtk.VBox()
        alignment.add(align_vbox)
        # add lines
        for subkey in self._config.get_keys(key):
            hbox = self.add_key(subkey)
            align_vbox.pack_end(hbox)
            align_vbox.set_child_packing(hbox, expand=False,
                                         fill=False, padding=5,
                                         pack_type=gtk.PACK_START)

        self.check_sensitive()
        if self._config.do_hide(name):
            self._hidden_widgets.append(seq_frame)
        restriction = self._config.get_xpath_restrictions(name)
        if restriction is not None:
            self._restrictions.update({seq_frame: restriction})

        return seq_frame

    def add_table(self, key):
        """ add a table in the tab """
        name = self._config.get_name(key)
        if self._config.do_hide_adv(name, self._adv_mode):
            return None
        if self._config.do_not_list(name):
            return self.add_key_sequence(key)
        check_buttons = []
        table_frame = gtk.Frame()
        table_frame.set_label_align(0, 0.5)
        table_frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        alignment = gtk.Alignment(0.5, 0.5, 1, 1)
        table_frame.add(alignment)
        table_label = gtk.HBox()
        label_text = gtk.Label()
        label_text.set_markup("<b>%s</b>" % name)
        description = self._config.get_documentation(name)
        table_label.pack_start(label_text)
        table_label.set_child_packing(label_text, expand=False,
                                      fill=False, padding=5,
                                      pack_type=gtk.PACK_START)
        self.add_description(table_label, description)
        table_frame.set_label_widget(table_label)
        align_vbox = gtk.VBox()
        alignment.add(align_vbox)
        # add buttons to add/remove elements
        toolbar = gtk.Toolbar()
        align_vbox.pack_start(toolbar)
        align_vbox.set_child_packing(toolbar, expand=False,
                                     fill=False, padding=5,
                                     pack_type=gtk.PACK_START)
        add_button = gtk.ToolButton(gtk.STOCK_ADD)
        add_button.set_name(self._config.get_path(key))
        add_button.connect('clicked', self.on_add_button_clicked)
        add_button.connect('clicked', self._changed_cb)
        add_button.set_tooltip_text("Add a line in the table")
        self._add_buttons.append(add_button)
        del_button = gtk.ToolButton(gtk.STOCK_REMOVE)
        del_button.set_name(self._config.get_path(key))
        del_button.connect('clicked', self.on_del_button_clicked)
        del_button.connect('clicked', self._changed_cb)
        del_button.set_tooltip_text("Remove the selected lines from the table")
        self._del_buttons.append(del_button)
        toolbar.insert(add_button, -1)
        toolbar.insert(del_button, -1)
        # add lines
        self._table_length[self._config.get_path(key)] = 0
        for line in self._config.get_table_elements(key):
            self._table_length[self._config.get_path(key)] += 1
            hbox = self.add_line(key, line, check_buttons)
            align_vbox.pack_end(hbox)
            align_vbox.set_child_packing(hbox, expand=False,
                                         fill=False, padding=5,
                                         pack_type=gtk.PACK_START)

        self._tables[self._config.get_path(key)] = check_buttons
        self.check_sensitive()
        if self._config.do_hide(name):
            self._hidden_widgets.append(table_frame)
        restriction = self._config.get_xpath_restrictions(name)
        if restriction is not None:
            self._restrictions.update({table_frame: restriction})

        return table_frame

    def add_line(self, key, line, check_buttons):
        """ add a line in the configuration """
        hbox = gtk.HBox()
        key_path = self._config.get_path(key)
        new = False
        try:
            hbox.set_name(self._config.get_path(line))
        except:
            # this is a new line
            self._new.append(key_path)
            new = True
            name = self._config.get_name(line)
            nbr = len(self._config.get_all("/%s/%s" % (key_path, name)))
            hbox.set_name("/%s/%s[%d]" % (key_path, name,
                                          nbr + self._new.count(key_path)))
        dic = self._config.get_element_content(line)
        # keep the model of a line for line addition
        if not key_path in self._table_models:
            self._table_models[key_path] = deepcopy(line)

        # add a check bo to select elements to remove
        check_button = gtk.CheckButton()
        check_button.set_name(key_path)
        check_button.set_tooltip_text("Select the lines you want to remove")
        check_button.connect('toggled', self.on_remove_button_toggled)
        hbox.pack_start(check_button)
        hbox.set_child_packing(check_button, expand=False,
                               fill=False, padding=0,
                               pack_type=gtk.PACK_START)
        sep = gtk.VSeparator()
        hbox.pack_start(sep)
        hbox.set_child_packing(sep, expand=False,
                               fill=False, padding=0,
                               pack_type=gtk.PACK_START)
        check_buttons.append(check_button)
        # add attributes
        for att in dic.keys():
            name = self._config.get_name(line)
            att_label = gtk.Label()
            att_label.set_markup(att)
            att_label.set_alignment(0.1, 0.5)
            att_label.set_width_chars(len(att) + 1)
            att_description = self._config.get_documentation(att, name)
            hbox.pack_start(att_label)
            hbox.set_child_packing(att_label, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_START)
            self.add_description(hbox, att_description)
            elt_type = self._config.get_attribute_type(att, name)
            value = ''
            path = ''
            source = self._config.get_file_source(att, name)
            cb = [self.handle_param_changed, self._changed_cb]
            scenario = self._scenario
            if self._host != GLOBAL:
                scenario = os.path.join(self._scenario, self._host)

            try:
                line_path = self._config.get_path(line)
                path = '%s/@%s' % (line_path, att)
                pos = line_path.rfind('[')
                line_id = line_path[pos:].strip('[]')
                value = dic[att]
                if att == SYMBOL_RATE:
                    self._symbol_rates.append(value)
                if att == RATIO:
                    self._ratios.append(value)
                if source is not None:
                    source = self._config.adapt_filename(source, line, line_id)
                    source = os.path.join(scenario, source)
            except:
                # this is a new line entry
                nbr = len(self._config.get_all("/%s/%s" % (key_path, name)))
                line_path = '/%s/%s[%d]' % (key_path, name, nbr + self_new.count(key_path))
                path = '/%s/%s[%d]/@%s' % (key_path, name,
                                           nbr + self._new.count(key_path),
                                           att)
                value = dic[att]
                if att == SYMBOL_RATE:
                    self._symbol_rates.append(value)
                if att == RATIO:
                    self._ratios.append(value)

                # TODO the file won't exist and the line itself does not exist
                #      prevent access to this
                source = None
#                if source is not None:
#                    line_id = str(nbr + self._new.count(key_path))
#                    source = self._config.adapt_filename(source, line, line_id)
#                    source = os.path.join(scenario, source)
            entry = ConfEntry(elt_type, value, path, source, self._host,
                              cb, self._file_cb)
            if new:
                # add new lines to changed list
                self._changed.append(entry)
            self._entries.append(entry)
            hbox.pack_start(entry.get())
            hbox.set_child_packing(entry.get(), expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_START)
            unit = self._config.get_unit(att, name)
            if unit is not None:
                unit_label = gtk.Label()
                unit_label.set_markup("<i> %s</i>" % unit)
                hbox.pack_start(unit_label)
                hbox.set_child_packing(unit_label, expand=False,
                                       fill=False, padding=2,
                                       pack_type=gtk.PACK_START)
        # check if line has configurable items
        if self._config.do_conf(self._config.get_name(line)):
            def edit_sequence(edit_button, event):
                window = EditSequenceDialog(line_path,
                                            self._config,
                                            self._host,
                                            self._scenario,
                                            self._changed_cb)
                window.go()

            edit_button = gtk.Button(stock=gtk.STOCK_EDIT)
            edit_button.show()
            edit_button.connect('button-press-event', edit_sequence)
            edit_button.set_tooltip_text("Configure the plugin")
            hbox.pack_start(edit_button)
            hbox.set_child_packing(edit_button, expand=False,
                                   fill=False, padding=0,
                                   pack_type=gtk.PACK_START)
        return hbox


    def add_description(self, widget, description):
        """ add a description tooltip """
        if description == None:
            return
        img = gtk.Image()
        img.set_from_stock(gtk.STOCK_DIALOG_INFO,
                           gtk.ICON_SIZE_MENU)
        img.set_tooltip_text(description)
        widget.pack_start(img)
        widget.set_child_packing(img, expand=False,
                                 fill=False, padding=1,
                                 pack_type=gtk.PACK_START)


    def handle_param_changed(self, source=None, event=None):
        """ 'changed' event on configuration value """
        if source is not None:
            if not source in self._changed:
                self._changed.append(source)

    def save(self):
        """ save the configuration """
        if (len(self._changed) == 0 and
            len(self._removed) == 0 and
            len(self._new) == 0):
            return

        update_bandwidth = False
        try:
            for table in self._new:
                self._config.add_line(table)

            for entry in self._changed:
                path = entry.get_name().split('/@')
                val = entry.get_value()
                if val is None:
                    continue
                if len(path) == 0 or len(path) > 2:
                    raise XmlException("[save()] wrong xpath %s" % path)
                elif len(path) == 1:
                    self._config.set_value(val, path[0])
                elif len(path) == 2:
                    if path[1] == SYMBOL_RATE:
                        update_bandwidth = True
                        syb_rate = self._config.get(path[0])
                        if syb_rate.get(SYMBOL_RATE) in  self._symbol_rates:
                            self._symbol_rates.remove(syb_rate.get(SYMBOL_RATE))
                        self._symbol_rates.append(val)
                    if path[1] == RATIO:
                        update_bandwidth = True
                        ratio = self._config.get(path[0])
                        if ratio.get(RATIO) in  self._ratios:
                            self._ratios.remove(ratio.get(RATIO))
                        self._ratios.append(val)
                    self._config.set_value(val, path[0], path[1])
        except XmlException:
            raise

        # Update SatDelay plugin configuration
        for entry in self._changed:
            path = entry.get_name()
            name = path.split('/@')[0]
            if len(path.split('/@')) > 1:
                attr = path.split('/@')[1]
            if name.endswith(']'):
                name = name.split('[')[-2].split('/')[-1]
            if name == SATDELAY and attr == SATDELAY_TYPE:
                val = self._config.get(path)
                # Remove old element
                self._config.del_element(path.split('/@')[0] + 
                                         '/' + SATDELAY_CONF)
                # get the correct plugin_conf keys from the plugin xml file
                for module in self._modules:
                    if module.get_name() == val:
                        module_parser = module.get_config_parser()
                        if module_parser is None:
                            break
                        new_conf = module_parser.get(PATH_SATDELAY_CONF_MODULE)
                        if new_conf is None:
                            break
                        # Add new element
                        self._config.add_element(path.split('/@')[0] + 
                                                 '/' + SATDELAY_CONF, new_conf)
                        break

        # Update Bandwidth
        if update_bandwidth and self._bandwidth_entry:
            path = self._bandwidth_entry.get_name().split('/@')
            bandwidth = 0.0
            old_bandwidth = float(self._config.get_value(\
                                self._config.get(path[0]))) * 1E6
            path_roll_off = self._bandwidth_entry.get_name().split('/@')[0].split('/spot')
            roll_off = float(self._config.get_value(self._config.get(path_roll_off[0] 
                                                                 + '/' + ROLL_OFF)))
            cumul_ratio_rs = 0
            for i in range(len(self._ratios)):
                ratios = map(lambda x: float(x), self._ratios[i].split(';'))
                cumul_ratio_rs += float(self._symbol_rates[i]) * sum(ratios)
            
            nb_porteuse = 1
            for i in range(len(self._ratios)):
                ratios = map(lambda x: float(x), self._ratios[i].split(';'))
                nb_carrier = int(round(sum(ratios) / cumul_ratio_rs * \
                                 old_bandwidth / (1 + roll_off)))
                if nb_carrier < 1:
                    nb_carrier = 1
                
                bandwidth += float(self._symbol_rates[i]) / 1000000 * \
                        (roll_off + 1) * nb_carrier
            self._bandwidth_entry.get().set_text(str(bandwidth))
            self._config.set_value(bandwidth, path[0])

        # remove lines in reversed order because each suppression shift indexes
        # in the XML document
        for line in reversed(self._removed):
            line_path = line.get_name()
            self._config.del_element(line_path)
            line.destroy()

        self._config.write()
        self._changed = []
        self._removed = []
        self._new = []

    def on_add_button_clicked(self, source=None, event=None):
        """ add button clicked """
        table_key = source.get_name()
        key = self._config.get(table_key)
        align = source.get_parent().get_parent()
        hbox = self.add_line(key, self._table_models[table_key],
                             self._tables[table_key])
        align.pack_end(hbox)
        align.set_child_packing(hbox, expand=False,
                                fill=False, padding=5,
                                pack_type=gtk.PACK_START)

        self._table_length[table_key] += 1
        self.check_sensitive()
        # add the completions if necessary
        for (completion, path, att) in self._completions:
            self.update_completion(completion, path, att)
        hbox.show_all()

    def on_del_button_clicked(self, source=None, event=None):
        """ delete button clicked """
        name = source.get_name()
        if not name in self._tables:
            return

        for check_button in [check for check in self._tables[name]
                                   if check.get_active()]:
            line = check_button.get_parent()
            if line is not None:
                line.hide()
                self._removed.append(line)
                self._table_length[name] -= 1

        self.check_sensitive()

    def on_remove_button_toggled(self, source=None, event=None):
        """ remove button toggled """
        cancel = False

        if source.get_inconsistent():
            cancel = True
        else:
            found = False
            for button in [but for but in self._del_buttons if but.get_name() in self._tables]:
                name = button.get_name()
                i = 0
                for check_button in self._tables[name]:
                    if check_button.get_name() == source.get_name():
                        found = True
                    if not check_button.get_active():
                        i = i + 1

                if found:
                    key = self._config.get(name)
                    key_entries = self._config.get_table_elements(key)
                    if key_entries:
                        key_name = self._config.get_name(key_entries[0])
                        cancel = (i < self._config.get_minoccurs(key_name))
                    break

        if cancel:
            # Cancel toggled because there is only one line
            source.set_sensitive(False)
            source.set_active(False)
            source.set_sensitive(True)
            return

        self.check_sensitive()

    def check_sensitive(self):
        """ check if remove button should be sensitive or not """
        for button in [but for but in self._del_buttons
                           if but.get_name() in self._tables]:
            button.set_sensitive(False)
            for check_button in self._tables[button.get_name()]:
                check_button.set_inconsistent(False)
            name = button.get_name()
            key = self._config.get(name)
            key_entries = self._config.get_table_elements(key)
            if key_entries:
                key_name = self._config.get_name(key_entries[0])
                if self._table_length[name] <= self._config.get_minoccurs(key_name):
                    # we should not remove button else the configuration won't be
                    # valid
                    for check_button in self._tables[button.get_name()]:
                        check_button.set_inconsistent(True)
                    continue
                for check_button in [check
                                 for check in self._tables[button.get_name()]
                                 if check.get_active()]:
                    if not check_button.get_parent() in self._removed:
                        button.set_sensitive(True)
                        break
        for button in self._add_buttons:
            button.set_sensitive(True)
            name = button.get_name()
            key = self._config.get(name)
            key_entries = self._config.get_table_elements(key)
            if key_entries:
                key_name = self._config.get_name(key_entries[0])
                if self._table_length[name] >= self._config.get_maxoccurs(key_name):
                    button.set_sensitive(False)

    def add_hidden(self, widget):
        """ add a hidden widget in the section """
        self._hidden_widgets.append(widget)

    def add_restriction(self, widget, restriction):
        """ add a restriction on a widget in the section """
        self._restrictions.update({widget: restriction})

    def update_completion(self, completion, path, att=None):
        """ add existing completion in gtk TextEntries """
        for entry in self._entries:
            if att is not None:
                try:
                    (entry_path, entry_att) = entry.get_name().split('/@')
                except:
                    continue
                if entry.get_name().startswith(entry_path) and att == entry_att:
                    entry.add_completion(completion)
            else:
                if entry.get_name().startswith(path):
                    entry.add_completion(completion)


    def set_completion(self, completion, path, att=None):
        """ add completion in matching gtk TextEntries """
        self._completions.append((completion, path, att))
        self.update_completion(completion, path, att)

class InstallNotebook(gtk.Notebook):
    """ the OpenSAND configuration view elements """
    def __init__(self, files, changed_cb=None):
        gtk.Notebook.__init__(self)

        self._files = files
        self._current_page = 0
        self._changed_cb = changed_cb

        self.set_scrollable(True)
        self.set_tab_pos(gtk.POS_TOP)
        self.connect('show', self.on_show)
        self.connect('hide', self.on_hide)

        self.load()

    def load(self):
        """ load the configuration view """
        for host in self._files:
            tab = self.add_host(host)
            self.fill_host(host, self._files[host], tab)

    def add_host(self, host_name):
        """ add a tab with host name in the notebook and return the associated
            vbox """
        scroll_notebook = gtk.ScrolledWindow()
        scroll_notebook.set_policy(gtk.POLICY_AUTOMATIC,
                                   gtk.POLICY_AUTOMATIC)
        tab_vbox = gtk.VBox()
        scroll_notebook.add_with_viewport(tab_vbox)
        tab_label = gtk.Label()
        tab_label.set_justify(gtk.JUSTIFY_CENTER)
        tab_label.set_markup("<small><b>%s</b></small>" % host_name)
        if host_name == GLOBAL:
            self.insert_page(scroll_notebook, tab_label, position=0)
        else:
            self.append_page(scroll_notebook, tab_label)
        return tab_vbox

    def fill_host(self, host, key_list, tab_vbox):
        """ fill the tabvbox with source and destination file to deploy """
        for elem in key_list:
            entry = self.add_line(host, elem[0], elem[1], elem[2])
            tab_vbox.pack_end(entry)
            tab_vbox.set_child_packing(entry, expand=False,
                                       fill=False, padding=5,
                                       pack_type=gtk.PACK_START)

    def add_line(self, host, xpath, src, dst):
        """ add a line containing the name of the element to deploy and the
            source and destination """
        hbox = gtk.HBox()
        src_vbox = gtk.VBox()
        hbox.pack_start(src_vbox)
        hbox.set_child_packing(src_vbox, expand=False,
                               fill=False, padding=5,
                               pack_type=gtk.PACK_START)
        sep = gtk.VSeparator()
        hbox.pack_start(sep)
        hbox.set_child_packing(sep, expand=False,
                               fill=False, padding=5,
                               pack_type=gtk.PACK_START)
        dst_vbox = gtk.VBox()
        hbox.pack_start(dst_vbox)
        hbox.set_child_packing(dst_vbox, expand=False,
                               fill=False, padding=5,
                               pack_type=gtk.PACK_START)

        frame = gtk.Frame()
        frame.set_label_align(0, 0.5)
        frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        alignment = gtk.Alignment(0.5, 0.5, 1, 1)
        frame.add(alignment)
        label = gtk.Label()
        label.set_markup("<b>%s</b>" % xpath_to_name(xpath))
        frame.set_label_widget(label)
        alignment.add(hbox)

        # source
        src_entry = gtk.Entry()
        src_entry.set_text(src)
        src_entry.set_name("%s:%s" % (host, xpath))
        src_entry.set_width_chars(50)
        src_entry.set_inner_border(gtk.Border(1, 1, 1, 1))
        src_entry.connect('changed', self._changed_cb)
        src_vbox.pack_end(src_entry)
        src_vbox.set_child_packing(src_entry, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_END)

        src_label = gtk.Label()
        src_label.set_markup("<b>Source</b>")
        src_label.set_alignment(0.5, 0.5)
        src_vbox.pack_end(src_label)
        src_vbox.set_child_packing(src_label, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_END)

        # destination
        dst_entry = gtk.Entry()
        dst_entry.set_text(dst)
        dst_entry.set_width_chars(len(dst))
        dst_entry.set_inner_border(gtk.Border(1, 1, 1, 1))
        dst_entry.set_editable(False)
        dst_vbox.pack_end(dst_entry)
        dst_vbox.set_child_packing(dst_entry, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_END)

        dst_label = gtk.Label()
        dst_label.set_markup("<b>Destination</b>")
        dst_label.set_alignment(0.5, 0.5)
        dst_vbox.pack_end(dst_label)
        dst_vbox.set_child_packing(dst_label, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_END)

        return frame



def xpath_to_name(xpath):
    """ convert a xpath value to a configuration key name """
    try:
        path = xpath.rsplit('/', 2)
        att = path[2]
        key = path[1]
        if '@' in xpath:
            return "%s/%s" % (key, att)
        else:
            return key
    except:
        return xpath



class ManageSpot:

    def __init__(self, model):
        self._model = model
        
    def add_spot(self, spot_id):
        for host in self._model.get_hosts_list() + [self._model,
                                                    self._model.get_topology()]:
            adv = host.get_advanced_conf()
            config = adv.get_configuration()
            config.add_spot(spot_id) 
            config.write()
 
        # update the content of the new created spots
        for host in self._model.get_hosts_list() + [self._model,
                                                    self._model.get_topology()]:
            adv = host.get_advanced_conf()
            adv.update_conf(spot_id=spot_id)


    def remove_spot(self, spot_id):
        for host in self._model.get_hosts_list() + [self._model,
                                                    self._model.get_topology()]:
            adv = host.get_advanced_conf()
            config = adv.get_configuration()
            config.remove_spot(spot_id) 
            adv.update_conf()
            config.write()





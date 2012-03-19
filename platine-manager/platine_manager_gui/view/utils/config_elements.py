#!/usr/bin/env python 
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
#
#
# This file is part of the Platine testbed.
#
#
# Platine is free software : you can redistribute it and/or modify it under the
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
from copy import deepcopy

from platine_manager_core.my_exceptions import XmlException

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class ConfigurationTree(gtk.TreeStore):
    """ the Platine configuration view tree """
    def __init__(self, treeview, col1_title, col2_title,
                 col1_changed_cb, col2_toggled_cb):
        # create a treesore with 4 properties
        # - text: the text of the 1st column
        # - visible: is the check box of the 2nd column visible
        # - active: is the check box of the 2nd column active
        # - activatable: can we activate the check box of the 2nd column
        gtk.TreeStore.__init__(self, str, gobject.TYPE_BOOLEAN,
                                          gobject.TYPE_BOOLEAN,
                                          gobject.TYPE_BOOLEAN)

        self._treeselection = None
        self._cell_renderer_toggle = None
        self._is_global = 0

        self.load(treeview, col1_title, col2_title, col1_changed_cb,
                  col2_toggled_cb)

    def load(self, treeview, col1_title, col2_title,
             col1_changed_cb, col2_toggled_cb):
        """ load the treestore """
        # attach the treestore to the treeview
        treeview.set_model(self)

        cell_renderer = gtk.CellRendererText()
        # Connect check box on the treeview
        self._cell_renderer_toggle = gtk.CellRendererToggle()
        self._cell_renderer_toggle.connect('toggled', col2_toggled_cb)

        column = gtk.TreeViewColumn(col1_title, cell_renderer, text=TEXT)
        column.set_resizable(True)
        column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)

        column_toggle = gtk.TreeViewColumn(col2_title,
                                           self._cell_renderer_toggle,
                                           visible=VISIBLE, active=ACTIVE,
                                           activatable=ACTIVATABLE)
        column_toggle.set_resizable(True)
        column_toggle.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)

        treeview.append_column(column)
        treeview.append_column(column_toggle)

        # add a column to avoid large toggle column
        col = gtk.TreeViewColumn('')
        treeview.append_column(col)

        # get the tree selection
        self._treeselection = treeview.get_selection()
        self._treeselection.set_mode(gtk.SELECTION_SINGLE)
        if col1_changed_cb is not None:
            self._treeselection.connect('changed', col1_changed_cb)

    def add_host(self, host, elt_info = None):
        """ add a host with its elements in the treeview """
        name = host.get_name()
        # append an element in the treestore
        # first Global, next SAT, then GW and ST
        if name == 'global':
            top_elt = self.insert(None, 0)
            self._is_global = 1
        elif name == 'sat':
            top_elt = self.insert(None, self._is_global)
        elif name == 'gw':
            top_elt = self.insert(None, self._is_global + 1)
        else:
            top_elt = self.append(None)

        if elt_info is not None:
            self.set(top_elt, TEXT, name.upper(),
                              VISIBLE, False,
                              ACTIVE, False,
                              ACTIVATABLE, False)
            for sub_name in elt_info.keys():
                activatable = True
                sub_iter = self.append(top_elt)
                # for tools or probes wihtout substatistic
                if elt_info[sub_name] is None:
                    activatable = False
                if not isinstance(elt_info[sub_name], list):
                    self.set(sub_iter, TEXT, sub_name,
                                       VISIBLE, True,
                                       ACTIVE, False,
                                       ACTIVATABLE, activatable)
                else:
                # for probes
                    activatable = False
                    self.set(sub_iter, TEXT, sub_name,
                                       VISIBLE, False,
                                       ACTIVE, False,
                                       ACTIVATABLE, False)
                    for sub2_name in elt_info[sub_name]:
                        sub2_iter = self.append(sub_iter)
                        self.set(sub2_iter, TEXT, sub2_name.upper(),
                                            VISIBLE, True,
                                            ACTIVE, False,
                                            ACTIVATABLE, True)

        else:
            # for advanced host
            activatable = True
            if host.get_state() is None:
                activatable = False
            active = host.is_enabled()
            self.set(top_elt, TEXT, name.upper(),
                              VISIBLE, True,
                              ACTIVE, active,
                              ACTIVATABLE, activatable)

    def del_host(self, host_name):
        """ remove a host from the treeview """
        name = host_name.upper()
        iterator = self.get_iter_first()
        while iterator is not None and \
              self.get_value(iterator, TEXT) != name:
            iterator = self.iter_next(iterator)
        if iterator is not None:
            self.remove(iterator)

    def disable_all(self):
        """ set all the check boxes activatable property to False """
        # FIXME
        self._cell_renderer_toggle.set_property('activatable', False)

    def get_selection(self):
        """ get the treeview selection """
        return self._treeselection

class ConfigurationNotebook(gtk.Notebook):
    """ the Platine configuration view elements """
    def __init__(self, config, changed_cb=None):
        gtk.Notebook.__init__(self)

        self._config = config
        self._current_page = 0
        self._changed = []
        self._changed_cb = changed_cb

        self.set_scrollable(True)
        self.set_tab_pos(gtk.POS_LEFT)
        self.connect('show', self.on_show)
        self.connect('hide', self.on_hide)
        # list of tables with format: {table path : [check button per line]}
        self._tables = {}
        # list of tables with format: {table path : format}
        self._table_models = {}
        # list of delete buttons
        self._del_buttons = []
        # list of added lines
        self._new = []
        # list of removed lines
        self._removed = []

        self.load()

    def load(self):
        """ load the configuration view """
        for section in self._config.get_sections():
            tab = self.add_section(section)
            self.fill_section(section, tab)
        
    def add_section(self, section, description = ''):
        """ add a section in the notebook and return the associated vbox """
        scroll_notebook = gtk.ScrolledWindow()
        scroll_notebook.set_policy(gtk.POLICY_AUTOMATIC,
                                   gtk.POLICY_AUTOMATIC)
        tab_vbox = gtk.VBox()
        scroll_notebook.add_with_viewport(tab_vbox)
        tab_label = gtk.Label()
        tab_label.set_justify(gtk.JUSTIFY_CENTER)
        tab_label.set_markup("<small><b>%s</b></small>" %
                             self._config.get_name(section))
        tab_label.set_tooltip_text(description)
        if description != '':
            tab_label.set_has_tooltip(True)
        self.append_page(scroll_notebook, tab_label)
        return tab_vbox

    def fill_section(self, section, tab):
        """ get the section content and fill the corresponding tab """
        for key in self._config.get_keys(section):
            if self._config.is_table(key):
                table = self.add_table(key)
                tab.pack_end(table)
                tab.set_child_packing(table, expand=False,
                                      fill=False, padding=5,
                                      pack_type=gtk.PACK_START)
            else:
                entry = self.add_key(key)
                tab.pack_end(entry)
                tab.set_child_packing(entry, expand=False,
                                      fill=False, padding=5,
                                      pack_type=gtk.PACK_START)

    def add_key(self, key, description = ''):
        """ add a key and its corresponding entry in a tab """
        key_box = gtk.HBox()
        key_label = gtk.Label()
        key_label.set_markup(self._config.get_name(key))
        key_label.set_alignment(0.0, 0.5)
        key_label.set_width_chars(25)
        key_label.set_tooltip_text(description)
        if description != '':
            key_label.set_has_tooltip(True)
        key_box.pack_start(key_label)
        key_box.set_child_packing(key_label, expand=False,
                                  fill=False, padding=5,
                                  pack_type=gtk.PACK_START)
        # TODO get the type of data and depending on it add correponding entry
        #      type
        entry = gtk.Entry()
        entry.set_text(self._config.get_value(key))
        entry.set_width_chars(50)
        entry.set_inner_border(gtk.Border(1, 1, 1, 1))
        entry.connect('changed', self.handle_param_chanded)
        if self._changed_cb is not None:
            entry.connect('changed', self._changed_cb)
        entry.set_name(self._config.get_path(key))
        key_box.pack_start(entry)
        key_box.set_child_packing(entry, expand=False,
                                  fill=False, padding=5,
                                  pack_type=gtk.PACK_START)
        return key_box

    def add_table(self, key, description = ''):
        """ add a table in the tab """
        check_buttons = []
        table_frame = gtk.Frame()
        table_frame.set_label_align(0, 0.5)
        table_frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        alignment = gtk.Alignment(0.5, 0.5, 1, 1)
        table_frame.add(alignment)
        table_label = gtk.Label()
        table_label.set_markup("<b>%s</b>" % self._config.get_name(key))
        table_frame.set_label_widget(table_label)
        table_label.set_tooltip_text(description)
        if description != '':
            table_label.set_has_tooltip(True)
        align_vbox = gtk.VBox()
        alignment.add(align_vbox)
        # add buttons to add/remove elements
        toolbar = gtk.Toolbar()
        align_vbox.pack_start(toolbar)
        align_vbox.set_child_packing(toolbar, expand=False,
                                     fill=False, padding=5,
                                     pack_type=gtk.PACK_START)
        add_button = gtk.ToolButton('gtk-add')
        add_button.set_name(self._config.get_path(key))
        add_button.connect('clicked', self.on_add_button_clicked)
        add_button.connect('clicked', self._changed_cb)
        del_button = gtk.ToolButton('gtk-remove')
        del_button.set_name(self._config.get_path(key))
        del_button.connect('clicked', self.on_del_button_clicked)
        del_button.connect('clicked', self._changed_cb)
        self._del_buttons.append(del_button)
        toolbar.insert(add_button, -1)
        toolbar.insert(del_button, -1)
        # add lines
        for line in self._config.get_table_elements(key):
            hbox = self.add_line(key, line, check_buttons)
            align_vbox.pack_end(hbox)
            align_vbox.set_child_packing(hbox, expand=False,
                                         fill=False, padding=5,
                                         pack_type=gtk.PACK_START)

        self._tables[self._config.get_path(key)] = check_buttons
        self.check_sensitive()
        return table_frame


    def add_line(self, key, line, check_buttons):
        """ add a line in the configuration """
        hbox = gtk.HBox()
        key_path = self._config.get_path(key)
        try:
            hbox.set_name(self._config.get_path(line))
        except:
            self._new.append(key_path)
            name = self._config.get_name(line)
            nbr = len(self._config.get_all("//%s" % name))
            hbox.set_name("//%s[%d]" % (name, nbr + 
                                        self._new.count(key_path)))
        dic = self._config.get_element_content(line)
        # keep the model of a line for line addition
        if not key_path in self._table_models:
            self._table_models[key_path] = deepcopy(line)

        # add a check bo to select elements to remove
        check_button = gtk.CheckButton()
        check_button.set_name(key_path)
        check_button.connect('toggled', self.on_remove_button_toggled)
        hbox.pack_start(check_button)
        hbox.set_child_packing(check_button, expand=False,
                               fill=False, padding=0,
                               pack_type=gtk.PACK_START)
        check_buttons.append(check_button)
        # add attributes
        for att in dic.keys():
            att_description = ''
            att_label = gtk.Label()
            att_label.set_markup(att)
            att_label.set_alignment(0.0, 0.5)
            att_label.set_width_chars(len(att) + 1)
            att_label.set_tooltip_text(att_description)
            if att_description != '':
                att_label.set_has_tooltip(True)
            hbox.pack_start(att_label)
            hbox.set_child_packing(att_label, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_START)
            entry = gtk.Entry()
            entry.set_text(dic[att])
            entry.set_width_chars(50)
            entry.set_inner_border(gtk.Border(1, 1, 1, 1))
            if self._changed_cb is not None:
                entry.connect('changed', self._changed_cb)
            try:
                entry.set_name('%s--%s' % (self._config.get_path(line), att))
                entry.connect('changed', self.handle_param_chanded)
            except:
                # this is a new line entry
                entry.set_name('//%s[last()]--%s' %
                               (self._config.get_name(line), att)) 
                entry.connect('changed', self.handle_param_chanded)
                entry.set_text('')
            hbox.pack_start(entry)
            hbox.set_child_packing(entry, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_START)

        return hbox


    def handle_param_chanded(self, source=None, event=None):
        """ 'changed' event on configuration value """
        if source is not None:
            self._changed.append(source)

    def save(self):
        """ save the configuration """
        if (len(self._changed) == 0 and
            len(self._removed) == 0 and
            len(self._new) == 0):
            return

        for table in self._new:
            self._config.add_line(table)

        for entry in self._changed:
            path = entry.get_name().split('--')
            val = entry.get_text()
            try:
                if len(path) == 0 or len(path) > 2:
                    raise XmlException("wrong xpath")
                elif len(path) == 1:
                    self._config.set_value(val, path[0])
                elif len(path) == 2:
                    self._config.set_value(val, path[0], path[1])
            except XmlException:
                raise

        # remove lines in reversed order because each suppression shift indexes
        # in the XML document
        for line in reversed(self._removed):
            line_path = line.get_name()
            self._config.del_element(line_path)
            line.destroy()


        self._config.write()
        self._changed = []
        self._removed = []
        self._new = {}


    def on_show(self, widget):
        """ notebook shown """
        self.set_current_page(self._current_page)

    def on_hide(self, widget):
        """ notebook hidden """
        self._current_page = self.get_current_page()

    def on_add_button_clicked(self, source=None, event=None):
        """ add button clicked """
        table_key = source.get_name()
        key = self._config.get(table_key)
#        self._config.add_table_element(table_key_path)
        align = source.get_parent().get_parent()
        hbox = self.add_line(key, self._table_models[table_key],
                             self._tables[table_key])
        align.pack_end(hbox)
        align.set_child_packing(hbox, expand=False,
                                fill=False, padding=5,
                                pack_type=gtk.PACK_START)
        hbox.show_all()

    def on_del_button_clicked(self, source=None, event=None):
        """ delete button clicked """
        name = source.get_name()
        if not name in self._tables:
            return

        for check_button in [check for check in self._tables[name]
                                   if check.get_active()]:
            line = check_button.get_parent()
            line.hide()
            self._removed.append(line)

        self.check_sensitive()

    def on_remove_button_toggled(self, source=None, event=None):
        """ remove button toggled """
        self.check_sensitive()

    def check_sensitive(self):
        """ check if remove button should be sensitive or not """
        for button in [but for but in self._del_buttons
                           if but.get_name() in self._tables]:
            button.set_sensitive(False)
            for check_button in [check
                                 for check in self._tables[button.get_name()]
                                 if check.get_active()]:
                if not check_button.get_parent() in self._removed:
                    button.set_sensitive(True)
                    break



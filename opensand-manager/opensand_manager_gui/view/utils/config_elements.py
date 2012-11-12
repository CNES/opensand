#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 TAS
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
from copy import deepcopy

from opensand_manager_core.my_exceptions import XmlException

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class ConfigurationTree(gtk.TreeStore):
    """ the OpenSAND configuration view tree """
    def __init__(self, treeview, col1_title, col2_title,
                 col1_changed_cb, col2_toggled_cb):
        # create a treestore with 4 properties
        # - text: the text of the 1st column
        # - visible: is the check box of the 2nd column visible
        # - active: is the check box of the 2nd column active
        # - activatable: can we activate the check box of the 2nd column
        gtk.TreeStore.__init__(self, str, gobject.TYPE_BOOLEAN,
                                          gobject.TYPE_BOOLEAN,
                                          gobject.TYPE_BOOLEAN)

        self._treeselection = None
        self._cell_renderer_toggle = None
        self._is_first_elt = 0

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
            self._is_first_elt = 1
        elif name == 'sat':
            top_elt = self.insert(None, self._is_first_elt)
        elif name == 'gw':
            top_elt = self.insert(None, self._is_first_elt + 1)
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

    def add_modules(self, modules):
        """ insert the modules int the tree """
        if len(modules) == 0:
            return

        top_elt = self.insert(None, 0)
        self._is_first_elt = 1
        self.set(top_elt, TEXT, 'Plugins',
                          VISIBLE, False,
                          ACTIVE, False,
                          ACTIVATABLE, False)
        for name in modules.keys():
            sub_iter = self.append(top_elt)
            self.set(sub_iter, TEXT, name,
                               VISIBLE, False,
                               ACTIVE, False,
                               ACTIVATABLE, False)

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
    """ the OpenSAND configuration view elements """
    def __init__(self, config, changed_cb=None):
        gtk.Notebook.__init__(self)

        self._config = config
        self._current_page = 0
        self._changed = []
        self._changed_cb = changed_cb
        # keep ConEntry objects else we sometimes loose their attributes in the
        # event callback
        self._backup = []

        self.set_scrollable(True)
        self.set_tab_pos(gtk.POS_LEFT)
        self.connect('show', self.on_show)
        self.connect('hide', self.on_hide)
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

        self.load()

    def load(self):
        """ load the configuration view """
        for section in self._config.get_sections():
            tab = self.add_section(section)
            self.fill_section(section, tab)

    def add_section(self, section):
        """ add a section in the notebook and return the associated vbox """
        name = self._config.get_name(section)
        scroll_notebook = gtk.ScrolledWindow()
        scroll_notebook.set_policy(gtk.POLICY_AUTOMATIC,
                                   gtk.POLICY_AUTOMATIC)
        tab_vbox = gtk.VBox()
        scroll_notebook.add_with_viewport(tab_vbox)
        tab_label = gtk.Label()
        tab_label.set_justify(gtk.JUSTIFY_CENTER)
        tab_label.set_markup("<small><b>%s</b></small>" % name)
        description = self._config.get_documentation(name)
        if description != None:
            # remove indentation
            descr = ' '.join(description.split())
            descr = descr.replace('-', '\n\t-')
            tab_label.set_tooltip_text(descr)
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

    def add_key(self, key):
        """ add a key and its corresponding entry in a tab """
        name = self._config.get_name(key)
        key_box = gtk.HBox()
        key_label = gtk.Label()
        key_label.set_markup(name)
        key_label.set_alignment(0.0, 0.5)
        key_label.set_width_chars(25)
        description = self._config.get_documentation(name)
        if description != None:
            # remove indentation
            descr = ' '.join(description.split())
            descr = descr.replace('-', '\n\t-')
            key_label.set_tooltip_text(descr)
            key_label.set_has_tooltip(True)
        key_box.pack_start(key_label)
        key_box.set_child_packing(key_label, expand=False,
                                  fill=False, padding=5,
                                  pack_type=gtk.PACK_START)
        elt_type = self._config.get_type(name)
        entry = ConfEntry(elt_type, self._config.get_value(key),
                          self._config.get_path(key),
                          [self.handle_param_chanded, self._changed_cb])
        self._backup.append(entry)

        key_box.pack_start(entry.get())
        key_box.set_child_packing(entry.get(), expand=False,
                                  fill=False, padding=5,
                                  pack_type=gtk.PACK_START)
        return key_box

    def add_table(self, key):
        """ add a table in the tab """
        name = self._config.get_name(key)
        check_buttons = []
        table_frame = gtk.Frame()
        table_frame.set_label_align(0, 0.5)
        table_frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        alignment = gtk.Alignment(0.5, 0.5, 1, 1)
        table_frame.add(alignment)
        table_label = gtk.Label()
        table_label.set_markup("<b>%s</b>" % name)
        table_frame.set_label_widget(table_label)
        description = self._config.get_documentation(name)
        if description != None:
            # remove indentation
            descr = ' '.join(description.split())
            descr = descr.replace('-', '\n\t-')
            table_label.set_tooltip_text(descr)
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
        add_button.set_tooltip_text("Add a line in the table")
        add_button.set_has_tooltip(True)
        self._add_buttons.append(add_button)
        del_button = gtk.ToolButton('gtk-remove')
        del_button.set_name(self._config.get_path(key))
        del_button.connect('clicked', self.on_del_button_clicked)
        del_button.connect('clicked', self._changed_cb)
        del_button.set_tooltip_text("Remove the selected lines from the table")
        del_button.set_has_tooltip(True)
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
        check_button.set_tooltip_text("Select the lines you want to remove")
        check_button.set_has_tooltip(True)
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
            att_label.set_alignment(1.0, 0.5)
            att_label.set_width_chars(len(att) + 1)
            att_description = self._config.get_documentation(att, name)
            if att_description != None:
                # remove indentation
                descr = ' '.join(att_description.split())
                descr = descr.replace('-', '\n\t-')
                att_label.set_tooltip_text(descr)
                att_label.set_has_tooltip(True)
            hbox.pack_start(att_label)
            hbox.set_child_packing(att_label, expand=False,
                                   fill=False, padding=5,
                                   pack_type=gtk.PACK_START)
            elt_type = self._config.get_attribute_type(att, name)
            value = ''
            path = ''
            cb = [self.handle_param_chanded, self._changed_cb]
            try:
                path = '%s--%s' % (self._config.get_path(line), att)
                value = dic[att]
            except:
                # this is a new line entry
                path = '//%s[last()]--%s' % (name, att)
            entry = ConfEntry(elt_type, value, path, cb)
            if value == '':
                # add new lines to changed list
                self._changed.append(entry)
            self._backup.append(entry)
            hbox.pack_start(entry.get())
            hbox.set_child_packing(entry.get(), expand=False,
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

        try:
            for table in self._new:
                self._config.add_line(table)

            for entry in self._changed:
                path = entry.get_name().split('--')
                val = entry.get_value()
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
        self._new = []


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
        align = source.get_parent().get_parent()
        hbox = self.add_line(key, self._table_models[table_key],
                             self._tables[table_key])
        align.pack_end(hbox)
        align.set_child_packing(hbox, expand=False,
                                fill=False, padding=5,
                                pack_type=gtk.PACK_START)

        self._table_length[table_key] += 1
        self.check_sensitive()
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
            self._table_length[name] -= 1

        self.check_sensitive()

    def on_remove_button_toggled(self, source=None, event=None):
        """ remove button toggled """
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
            key_name = self._config.get_name(key_entries[0])
            if self._table_length[name] >= self._config.get_maxoccurs(key_name):
                button.set_sensitive(False)

        


class ConfEntry(object):
    """ element for configuration entry """
    def __init__(self, entry_type, value, path, sig_handlers):
        self._type = entry_type
        self._value = value
        self._path = path
        self._entry = None
        self._sig_handlers = sig_handlers

        type_name = ""
        if self._type is None:
            self.load_default()
            type_name = "string"
        else:
            type_name = self._type["type"]
        if type_name == "boolean":
            self.load_bool()
        elif type_name == "enum":
            self.load_enum()
        elif type_name == "integer":
            self.load_int()
        else:
            self.load_default()

    def load_default(self):
        """ load a gtk.Entry """
        self._entry = gtk.Entry()
        self._entry.set_text(self._value)
        self._entry.set_width_chars(30)
        self._entry.set_inner_border(gtk.Border(1, 1, 1, 1))
        self._entry.connect('changed', self.global_handler)

    def load_bool(self):
        """ load a gtk.CheckButton """
        self._entry = gtk.CheckButton()
        if self._value == "true":
            self._entry.set_active(1)
        else:
            self._entry.set_active(0)
        self._entry.connect('toggled', self.global_handler)

    def load_enum(self):
        """ load a gtk.ComboBox """
        self._entry = gtk.ComboBox()
        store = gtk.ListStore(gobject.TYPE_STRING)
        for elt in self._type["enum"]:
            store.append([elt])
        self._entry.set_model(store)
        cell = gtk.CellRendererText()
        self._entry.pack_start(cell, True)
        self._entry.add_attribute(cell, 'text', 0)
        if self._value != '':
            self._entry.set_active(self._type["enum"].index(self._value))
        self._entry.connect('changed', self.global_handler)
        self._entry.connect('scroll-event', self.do_not_scroll)

    def load_int(self):
        """ load a gtk.SpinButton """
        low = 0
        up = 100000
        if "min" in self._type:
            low = float(self._type["min"])
        if "max" in self._type:
            up = float(self._type["max"])
        if self._value != '':
            val = float(self._value)
        else:
            val = low
        adj = gtk.Adjustment(value=val, lower=low, upper=up,
                             step_incr=1, page_incr=0, page_size=0)
        self._entry = gtk.SpinButton(adjustment=adj, climb_rate=1)
        self._entry.connect('value-changed', self.global_handler)
        self._entry.connect('scroll-event', self.do_not_scroll)

    def do_not_scroll(self, source=None, event=None):
        """ stop scolling in the element which emits the scroll-event signal """
        source.emit_stop_by_name('scroll-event')
        return False

    def get(self):
        """ get the gtk element """
        return self._entry

    def get_name(self):
        """ get the path for entry """
        return self._path

    def get_value(self):
        """ get the value fot the entry """
        type_name = ""
        if self._type is None:
            type_name = "string"
        else:
            type_name = self._type["type"]

        if type_name == "boolean":
            if self._entry.get_active():
                return "true"
            return "false"
        elif type_name == "enum":
            model = self._entry.get_model()
            active = self._entry.get_active_iter()
            return model.get_value(active, 0)
        elif type_name == "integer":
            return self._entry.get_text()
        else:
            return self._entry.get_text()

    def global_handler(self, source=None, event=None):
        """ handler used to abstract source type """
        for handler in [hdl for hdl in self._sig_handlers
                            if hdl is not None]:
            handler(self, event)



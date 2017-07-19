#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2017 TAS
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

# Author : Joaquin MUGUERZA

"""
graphical_parameter.py - Some graphical parameters for band configuration
"""

import gtk
import gobject
import pango
import os

from opensand_manager_core.utils import GLOBAL
from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.utils.config_entry import ConfEntry

class EditSequenceDialog(WindowView):
    """ an band configuration window """
    def __init__(self, line_path, config, host, scenario, changed_cb):

        WindowView.__init__(self, None, 'edit_sequence_dialog')

        self._dlg = self._ui.get_widget('edit_sequence_dialog')
        self._dlg.set_keep_above(True)

        self._config = config
        self._line_path = line_path
        self._line = self._config.get(line_path)
        self._scenario = scenario
        self._host = host

        self._changed_cb = changed_cb
        #self._file_cb = file_cb
        
        self._changed = []
        self._removed = []
        self._new = []

        self._vbox = self._ui.get_widget('edit_sequence_vbox')
        self._vbox.show_all()

    def go(self):
        """ run the window """
        try:
            self.fill()
        except ModelException, msg:
            error_popup(str(msg))
        self._dlg.set_title("Edit sequence - OpenSAND Manager")
        self._dlg.run()


    def fill(self):
        """ load the sequence configuration """
        # iterate over sequences
        for elem in self._config.get_keys(self._line):
            # first add the element description
            name = self._config.get_name(elem)
            description = self._config.get_documentation(name)
            elem_frame = gtk.Frame()
            elem_frame.set_label_align(0, 0.5)
            elem_frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
            alignment = gtk.Alignment(0.5, 0.5, 1, 1)
            elem_frame.add(alignment)
            elem_label = gtk.HBox()
            label_text = gtk.Label()
            label_text.set_markup("<b>%s</b>" % name)
            elem_label.pack_start(label_text)
            elem_label.set_child_packing(label_text, expand=False,
                                         fill=False, padding=5,
                                         pack_type=gtk.PACK_START)
            self.add_description(elem_label, description)
            elem_frame.set_label_widget(elem_label)
            align_vbox = gtk.VBox()
            alignment.add(align_vbox)
            
            for key in self._config.get_keys(elem):
                # TODO: consider the possibility of a table in here
                # For now, we only consider keys
                entry = self.add_key(key)
                if entry is not None:
                    align_vbox.pack_end(entry)
                    align_vbox.set_child_packing(entry, expand=False,
                                                 fill=False, padding=5,
                                                 pack_type=gtk.PACK_START)

            self._vbox.pack_end(elem_frame)
            self._vbox.set_child_packing(elem_frame, expand=False,
                                         fill=False, padding=5,
                                         pack_type=gtk.PACK_START)

        self._vbox.show_all()

    def add_key(self, key):
        name = self._config.get_name(key)
        # don't consider advanced mode
        #if self._config.do_hide_adv(name, self._adv_mode):
        #    return None
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
                          [self.handle_param_changed],
                          [])
        #self._entries.append(entry)

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
        # don't consider hide
        #if self._config.do_hide(name):
        #    self._hidden_widgets.append(key_box)

        restriction = self._config.get_xpath_restrictions(name)
        if restriction is not None:
            self._restrictions.update({key_box: restriction})
        
        return key_box

    def handle_param_changed(self, source=None, event=None):
        """ 'changed' event on configuration value """
        if source is not None:
            if not source in self._changed:
                self._changed.append(source)

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
                path = entry.get_name().split('/@')
                val = entry.get_value()
                if val is None:
                    continue
                if len(path) == 0 or len(path) > 2:
                    raise XmlException("[save()] wrong xpath %s" % path)
                elif len(path) == 1:
                    self._config.set_value(val, path[0])
                elif len(path) == 2:
                    self._config.set_value(val, path[0], path[1])
        except XmlException:
            raise

        # remove lines in reversed order because each suppression shifts indexes
        # in the XML document
        for line in reversed(self._removed):
            line_path = line.get_name()
            self._config.del_element(line_path)
            line.destroy()

        self._changed = []
        self._removed = []
        self._new = []


    def close(self):
        """ close the window"""
        self._dlg.destroy()

    def on_edit_sequence_dialog_save(self, source=None, event=None):
        self.save()
        self._changed_cb()
        self.close()

    def on_edit_sequence_dialog_destroy(self, source=None, event=None):
        """Close the window """
        self.close()

    def on_edit_sequence_delete_event(self, source=None, event=None):
        """Close the window """
        self.close()

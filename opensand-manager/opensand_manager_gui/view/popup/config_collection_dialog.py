#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 TAS
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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
config_collection_dialog.py - Dialog allowing to enable and disable probes
"""

import gtk
import gobject

from opensand_manager_gui.view.window_view import WindowView

(ACTIVE, NAME, PROG_ID, ID) = range(4)

class ConfigCollectionDialog(WindowView):
    """ an advanced configuration window """
    def __init__(self, model, manager_log, sel_controller):
        WindowView.__init__(self, None, 'config_collection_window')

        self._dlg = self._ui.get_widget('config_collection_window')
        self._dlg.set_title("Configure probes collection - OpenSAND Manager")
        self._dlg.set_icon_name(gtk.STOCK_PREFERENCES)
        widget = self._ui.get_widget('config_collection_scroll')
        self._listview = gtk.TreeView()
        widget.add(self._listview)
        widget.show_all()
        self._sel_controller = sel_controller
        self._model = model
        self._log = manager_log
        self._shown = False
        self._programs = {}

        self._probe_store = gtk.TreeStore(gobject.TYPE_BOOLEAN,
                                          gobject.TYPE_STRING,
                                          gobject.TYPE_INT,
                                          gobject.TYPE_INT)
        self._listview.set_model(self._probe_store)
        self._listview.get_selection().set_mode(gtk.SELECTION_NONE)

        column_text = gtk.TreeViewColumn("Probe")
        self._listview.append_column(column_text)

        cellrenderer_toggle = gtk.CellRendererToggle()
        column_text.pack_start(cellrenderer_toggle, False)
        column_text.add_attribute(cellrenderer_toggle, "active", ACTIVE)
        cellrenderer_toggle.connect("toggled", self._cell_toggled)

        cellrenderer_text = gtk.CellRendererText()
        column_text.pack_start(cellrenderer_text, True)
        column_text.add_attribute(cellrenderer_text, "text", NAME)

    def show(self):
        """ show the window """
        if self._shown:
            self._dlg.present()
            return

        self._shown = True

        self._sel_controller.register_collection_dialog(self)

        self._dlg.show()

    def hide(self):
        """ hide the window """
        if not self._shown:
            return

        self._sel_controller.register_collection_dialog(None)
        self._shown = False
        self._dlg.hide()

    def update_list(self, program_dict):
        """ update the probe list shown by the window """

        self._programs = program_dict

        self._probe_store.clear()

        for program in self._programs.itervalues():
            parent = self._probe_store.append(None, [True, program.name,
                                                     program.ident, -1])
            for probe in program.get_probes():
                if not probe.enabled:
                    # probes are not all active, untick box
                    self._probe_store.set_value(parent, ACTIVE, False)
                self._probe_store.append(parent, [probe.enabled,
                                                  probe.full_name.split('.', 1)[1],
                                                  probe.program.ident,
                                                  probe.ident])


    def _cell_toggled(self, _, path):
        """ a window cell has been toggled """
        it = self._probe_store.get_iter(path)
        parent = self._probe_store.iter_parent(it)
        if parent is None:
            # program name, get the active value
            val = not self._probe_store.get_value(it, ACTIVE)
            self._probe_store.set_value(it, ACTIVE, val)
            for idx in range(self._probe_store.iter_n_children(it)):
                child = self._probe_store.iter_nth_child(it, idx)
                self._probe_toggled(child, it, val)
            return
        self._probe_toggled(it, parent)
        

    def _probe_toggled(self, it, parent, val=None):
        """ a probe has been toggled """
        new_value = val
        if val is None:
            new_value = not self._probe_store.get_value(it, ACTIVE)
        prog_id = self._probe_store.get_value(it, PROG_ID)
        probe_id = self._probe_store.get_value(it, ID)

        probe = self._programs[prog_id].get_probe(probe_id)
        was_hidden = False
        if probe.displayed:
            was_hidden = True
            probe.displayed = False

        probe.enabled = new_value
        self._probe_store.set(it, ACTIVE, new_value)

        # update parent box
        # if this one is False untick
        # TODO see gtk.TreeModel.row_has_child_toggled !!
        if new_value is False:
            self._probe_store.set_value(parent, ACTIVE, False)
        else: # we need to check if there is a child that is not activated
            # first set active
            self._probe_store.set_value(parent, ACTIVE, True)
            for idx in range(self._probe_store.iter_n_children(parent)):
                child = self._probe_store.iter_nth_child(parent, idx)
                if not self._probe_store.get_value(child, ACTIVE):
                    # one is not active, untick box
                    self._probe_store.set_value(parent, ACTIVE, False)

        self._sel_controller.probe_enabled_changed(probe, was_hidden)


    def on_config_collection_window_delete_event(self, _widget, _event):
        """ the window close button was clicked """
        self.hide()
        return True

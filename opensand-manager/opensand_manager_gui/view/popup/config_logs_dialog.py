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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
config_logs_dialog.py - Dialog allowing to set logs level
"""

import gtk
import gobject

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_core.loggers.levels import LOG_LEVELS

(NAME, ID, LEVEL, VISIBLE) = range(4)

class ConfigLogsDialog(WindowView):
    """ an advanced configuration window """
    def __init__(self):
        WindowView.__init__(self, None, 'config_collection_window')

        self._dlg = self._ui.get_widget('config_collection_window')
        self._dlg.set_title("Configure logs collection - OpenSAND Manager")
        self._dlg.set_icon_name(gtk.STOCK_PREFERENCES)
        widget = self._ui.get_widget('config_collection_scroll')
        self._listview = gtk.TreeView()
        widget.add(self._listview)
        widget.show_all()
        self._shown = False
        self._program = None

        self._log_store = gtk.TreeStore(gobject.TYPE_STRING,
                                        gobject.TYPE_INT,
                                        gobject.TYPE_STRING,
                                        gobject.TYPE_BOOLEAN)
        self._listview.set_model(self._log_store)

        column_text = gtk.TreeViewColumn("Logs")
        column_level = gtk.TreeViewColumn("Level")
        self._listview.append_column(column_text)
        self._listview.append_column(column_level)

        cellrenderer_text = gtk.CellRendererText()
        column_text.pack_start(cellrenderer_text, True)
        column_text.add_attribute(cellrenderer_text, "text", NAME)

        store = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_INT)
        for level in LOG_LEVELS:
            store.append([LOG_LEVELS[level].name, level])
        cellrenderer_combo = gtk.CellRendererCombo()
        cellrenderer_combo.set_property("editable", True)
        cellrenderer_combo.set_property("model", store)
        cellrenderer_combo.set_property("text-column", 0)
        cellrenderer_combo.connect("changed", self._cell_edited, store)
        column_level.pack_start(cellrenderer_combo, True)
        column_level.add_attribute(cellrenderer_combo, "text", LEVEL)
        column_level.add_attribute(cellrenderer_combo, "visible", VISIBLE)

    def show(self):
        """ show the window """
        if self._shown:
            self._dlg.present()
            return

        self._shown = True

        self._dlg.show()

    def hide(self):
        """ hide the window """
        if not self._shown:
            return

        self._shown = False
        self._dlg.hide()

    def update_list(self, program):
        """ update the probe list shown by the window """
        self._log_store.clear()
        self._program = program
        for log in self._program.get_logs():
            if log.display_level > len(LOG_LEVELS):
                # do not add events here
                continue
            log_name = log.full_name.split('.', 1)[1]
            names = log_name.split('.', 3)
            parent = None
            for i in range(len(names)):
                if i < (len(names) - 1):
                    nbr = self._log_store.iter_n_children(parent)
                    found = False
                    for child in range(nbr):
                        it = self._log_store.iter_nth_child(parent, child)
                        if self._log_store.get_value(it, NAME) == names[i]:
                            parent = it
                            found = True
                            break
                    if not found:
                        parent = self._log_store.append(parent, [names[i],
                                                                 -1,
                                                                 0,
                                                                False])
                else:
                    # probes are not all active, untick box
                    self._log_store.append(parent, [names[i],
                                                    log.ident,
                                                    LOG_LEVELS[log.display_level].name,
                                                    True])

    def _cell_edited(self, cell, path, new_text, model):
        """ a window cell has been edited """
        it = self._log_store.get_iter(path)
        log_id = self._log_store.get_value(it, ID)
        level_name = model.get_value(new_text, 0)
        level_id = model.get_value(new_text, 1)
        self._log_store.set_value(it, LEVEL, level_name)
        try:
            log = self._program.get_log(log_id)
        except KeyError:
            pass
        else:
            log.display_level = int(level_id)


    def on_config_collection_window_delete_event(self, _widget, _event):
        """ the window close button was clicked """
        self.hide()
        return True
        """ the window close button was clicked """
        self.hide()
        return True

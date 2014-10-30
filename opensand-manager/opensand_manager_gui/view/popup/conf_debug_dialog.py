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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
conf_debug_dialog.py - A window that enable to configure the debug values
"""


from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.utils.config_elements import ConfSection

class ConfigDebugDialog(WindowView):
    """ a window enabling to edit the debug values in configuration """
    def __init__(self):
        WindowView.__init__(self, None, 'config_collection_window')

        self._dlg = self._ui.get_widget('config_collection_window')
        self._dlg.set_title("Configure initial debug levels - OpenSAND Manager")
        self._dlg.set_icon_name('gtk-properties')
        self._scroll = self._ui.get_widget('config_collection_scroll')
        self._shown = False
        self._program = None
        self._current_conf = None
        self._dlg.connect('delete-event', self._delete)

    def show(self):
        """ run the window """
        if self._shown:
            self._dlg.present()
            return

        self._shown = True

        self._dlg.set_icon_name('gtk-properties')
        self._dlg.show()

    def hide(self):
        """ hide the window """
        if not self._shown:
            return

        self._shown = False
        self._dlg.hide()

    def update(self, program):
        """ update the configuration for current program """
        self._program = program
        host = self._program.get_host_model()
        debug = host.get_debug()
        if self._current_conf is not None:
            child = self._scroll.get_children()
            self._scroll.remove(child[0])
        self._current_conf = None
        if debug is None:
            return
        config = host.get_advanced_conf().get_configuration()
        self._current_conf = ConfSection(debug,
                                         config,
                                         host.get_name(),
                                         True, "",
                                         self.edit_cb, None)
        logs = set()
        for log in self._program.get_logs():
            log_name = log.full_name.split('.', 1)[1]
            names = log_name.split(".")
            logs |= set(names)
        logs = set(map(lambda x: x.lower(), logs))
        self._current_conf.set_completion(logs, "//debug/levels/level", "name")

        self._current_conf.show_all()
        self._scroll.add_with_viewport(self._current_conf)
        self._scroll.show_all()

    def edit_cb(self, source=None, event=None):
        self._current_conf.save()

    def has_debug(self, program):
        """ check if the program has a debug section """
        if program is None:
            return False
        host = program.get_host_model()
        debug = host.get_debug()
        if debug is None:
            return False
        return True

    def _delete(self, _widget, _event):
        """ the window close button was clicked """
        self.hide()
        return True

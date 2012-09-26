#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
# Copyright © 2011 CNES
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
tool_view.py - the tool tab view
"""

import gtk
import gobject
import threading

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.utils.config_elements import ConfigurationTree

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class ToolView(WindowView):
    """ Elements of the tool tab """
    def __init__(self, parent, model, manager_log):
        """constructor, initialization """
        WindowView.__init__(self, parent)

        self._model = model
        self._log = manager_log

        # create the list of hosts
        self._hosts_name = []
        # the list of tools parameters widgets that changed identified by a name
        self._tool_lock = threading.Lock()
        # the tools we have selected
        self._selected_tools = {}
        # the selected tools once the configuration has been saved
        self._saved_tools = {}
        # the available modules
        self._modules = self._model.get_modules()
        self._missing_modules = self._model.get_missing()

        with gtk.gdk.lock:
            # get the description widget
            self._desc_txt = self._ui.get_widget('tool_description')
            self._desc_txt.set_alignment(0.0, 0.0)
            self._desc_txt.set_padding(20, 20)
            # get the configuration area and add a vbox because if we directly
            # add a widget to the sroll view, once it will be destroyed the
            # scroll view will also be destroyed because it won't have
            # reference anymore with the vbox we can add the new widget before
            # destroying the older
            self._config_view = gtk.VBox()
            config_area = self._ui.get_widget('tool_config_area')
            config_area.add_with_viewport(self._config_view)

            self._current_notebook = None

            # create the tree for tools selection
            treeview = self._ui.get_widget('tools_selection_tree')
            self._tree = ConfigurationTree(treeview, 'Tools/modules',
                                           'Selected', self.on_selection,
                                           self.tool_toggled_cb)
            # populate the tree
            self._tree.add_modules(self._modules)

            # disable save button
            self._ui.get_widget('save_tool_conf').set_sensitive(False)
            self._ui.get_widget('undo_tool_conf').set_sensitive(False)
            # update the tree immediately
            self.update_tool_tree()

    def tool_toggled_cb(self, cell, path):
        """ defined in tool_event """
        pass

    def on_selection(self, selection):
        """ defined in tool_event """
        pass

    def update_tool_tree(self):
        """ update the tool tree """
        self._tool_lock.acquire()
        # disable tool selection when running
        if self._model.is_running():
            self._tree.disable_all()

        for host in [elt for elt in self._model.get_all()
                     if elt.get_name() not in self._hosts_name]:
            tools = {}
            self._hosts_name.append(host.get_name())
            self._selected_tools[host.get_name()] = []
            self._saved_tools[host.get_name()] = []
            for tool in host.get_tools():
                tools[tool.get_name().upper()] = tool.get_state()

            gobject.idle_add(self._tree.add_host,
                             host, tools)

        real_names = []
        for host in self._model.get_all():
            real_names.append(host.get_name())

        old_host_names = set(self._hosts_name) - set(real_names)
        for host_name in old_host_names:
            self._hosts_name.remove(host_name)
            del self._selected_tools[host_name]
            del self._saved_tools[host_name]
            gobject.idle_add(self._tree.del_host, host_name)

        self._tool_lock.release()

        # continue to refresh
        return True

    def handle_param_changed(self, source=None, event=None):
        """ defined in tool_event """
        pass

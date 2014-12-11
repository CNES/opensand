#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
# Copyright © 2014 CNES
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
tool_event.py - the events on tool tab
"""

import gobject

from opensand_manager_gui.view.tool_view import ToolView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.utils.config_elements import ConfigurationNotebook
from opensand_manager_core.my_exceptions import XmlException, ModelException

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class ToolEvent(ToolView):
    """ Events for the tool tab """
    def __init__(self, parent, model, manager_log):
        ToolView.__init__(self, parent, model, manager_log)
        self._refresh_tool_tree = None

    def close(self):
        """ close tool tab """
        self._log.debug("Tool Event: close")
        if self._refresh_tool_tree is not None:
            gobject.source_remove(self._refresh_tool_tree)
        self._log.debug("Tool Event: closed")

    def activate(self, val):
        """ 'activate' signal handler """
        if val == False and self._refresh_tool_tree is not None:
            gobject.source_remove(self._refresh_tool_tree)
            self._refresh_tool_tree = None
        elif val:
            # update the tree immediately then add a periodic update
            self.update_tree()
            self._refresh_tool_tree = gobject.timeout_add(1000,
                                                          self.update_tree)

    def on_selection(self, selection):
        """ callback called when a tool is selected """
        (tree, iterator) = selection.get_selected()
        if iterator is None:
            self._desc_txt.set_markup('')
            self._config_view.hide_all()
            return

        # Tree format:
        #
        # > Host name
        #   > tool name
        
        start = "<span size='x-large' foreground='#1088EB'><b>"
        end = "</b></span>"
        # Host name
        if tree.iter_parent(iterator) == None:
            # host without tool
            if not tree.iter_has_child(iterator):
                self._desc_txt.set_markup("%sThis host has no available "
                                          "tool%s" % (start, end))
            # host with tools
            else:
                self._desc_txt.set_markup("%sDeploy host to see "
                                          "available tools%s" %
                                          (start, end))

            self._config_view.hide_all()
        else:
            self.on_tool_select(iterator)
                
    def on_tool_select(self, iterator):
        """ a tool has been selected """
        tool_name = self._tree.get_value(iterator, TEXT).lower()
        host_name = self._tree.get_value(self._tree.iter_parent(iterator), TEXT)
        host = self._model.get_host(host_name.lower())

        tool_model = None
        description = ''
        if host is not None:
            tool_model = host.get_tool(tool_name)
            description = tool_model.get_description()

        if tool_model is None or \
           tool_model.get_state() is None or \
           tool_model.get_description() is None or \
           tool_model.get_config_parser() is None:
            warning =  "<span size='x-large' background='red' " + \
                       "foreground='white'>" + \
                       "This tool failed to load\n\n</span>"
            if description is not None:
                description = warning + description
            else:
                description = warning

        self._desc_txt.set_markup(description)

        self.display_config_view(tool_model, iterator)
        
    def tool_toggled_cb(self, cell, path):
        """ sets the toggled state on the toggle button to true or false
            and modify tool state from None to False or conversely """
        self._ui.get_widget('save_tool_conf').set_sensitive(True)
        self._ui.get_widget('undo_tool_conf').set_sensitive(True)
        self._tool_lock.acquire()
        curr_iter = self._tree.get_iter_from_string(path)
        curr_name = self._tree.get_value(curr_iter, TEXT)
        parent_iter = self._tree.iter_parent(curr_iter)
        parent_name = self._tree.get_value(parent_iter, TEXT)

        # modify ACTIVE property
        val = not self._tree.get_value(curr_iter, ACTIVE)
        self._tree.set(curr_iter, ACTIVE, val)

        self._log.debug("tool %s toggled with host %s" %
                        (curr_name, parent_name))

        if not val:
            self._selected_tools[parent_name.lower()].remove(curr_name.lower())
        else:
            self._selected_tools[parent_name.lower()].append(curr_name.lower())

        self._tool_lock.release()

    def display_config_view(self, model, iterator):
        """ display the configuration as read in configuration file
            (should be used with gobject.idle_add outside gtk handlers) """
        config_parser = model.get_config_parser()
        if config_parser is None:
            self._tree.set(iterator, ACTIVATABLE, False, ACTIVE, False)
            self._config_view.hide_all()
            return

        notebook = model.get_conf_view()
        if notebook is None:
            notebook = ConfigurationNotebook(config_parser,
                                             model.get_host().lower(),
                                             self._model.get_adv_mode(),
                                             self._model.get_scenario(),
                                             # we have no button to hide/show
                                             # hidden, show them in adv mode
                                             self._model.get_adv_mode(),
                                             self.handle_param_changed,
                                             # TODO
                                             None)

        model.set_conf_view(notebook)

        if notebook != self._current_notebook:
            self._config_view.hide_all()
            self._config_view.pack_start(notebook)
            if self._current_notebook is not None:
                self._config_view.remove(self._current_notebook)
            self._config_view.show_all()
            self._current_notebook = notebook
        else:
            self._config_view.show_all()

    def on_save_tool_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on save button """
        self._tool_lock.acquire()
        for host in self._model.get_all():
            host_name = host.get_name()

            # reset the tools
            map(lambda x : x.set_selected(False), host.get_tools())
            # mark selected tools
            if not host_name.lower() in self._selected_tools.keys():
                continue
            for tool in [elt for elt in host.get_tools()
                         if elt.get_name() in
                         self._selected_tools[host_name.lower()]]:
                self._log.debug("tool %s enabled for %s" %
                                (elt.get_name().upper(), host_name.upper()))
                tool.set_selected()
            # save the tool configuration
            for tool in host.get_tools():
                notebook = tool.get_conf_view()
                if notebook is None:
                    continue
                try:
                    self._log.debug("save %s config for %s" %
                                    (tool.get_name(), host_name))
                    notebook.save()
                except XmlException, error:
                    error_popup("%s: %s" % (host_name, error.description))
                    continue

        # do that to copy contents else saved tools will contain references on
        # selected tools
        for host_name in self._selected_tools:
            self._saved_tools[host_name] = list(self._selected_tools[host_name])

        self._tool_lock.release()
        self._ui.get_widget('save_tool_conf').set_sensitive(False)
        self._ui.get_widget('undo_tool_conf').set_sensitive(False)

    def on_undo_tool_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        self._tool_lock.acquire()
        # do that to copy contents else selected tools will contain references
        # on saved tools
        for host_name in self._saved_tools:
            self._selected_tools[host_name] = list(self._saved_tools[host_name])
        # delete tools vbox to reload tool info
        for host in self._model.get_all():
            for tool in host.get_tools():
                try:
                    tool.reload_conf(self._model.get_scenario())
                except ModelException, msg:
                    self._tool_lock.release()
                    error_popup("%s: %s" % (tool.get_name(), msg))
                    self._tool_lock.acquire()

        self._tree.foreach(self.select_saved)

        page = self._current_notebook.get_current_page()
        self.on_selection(self._tree.get_selection())
        self._current_notebook.set_current_page(page)

        self._tool_lock.release()
        self._ui.get_widget('save_tool_conf').set_sensitive(False)
        self._ui.get_widget('undo_tool_conf').set_sensitive(False)

    def select_saved(self, tree, path, iterator):
        """ toggled the elements as in saved_tools list
            (used in callback so no need to use locks) """
        parent = tree.iter_parent(iterator)
        if parent == None:
            return

        tree.set(iterator, ACTIVE, False)
        host_name = tree.get_value(parent, TEXT).lower()
        if host_name.lower() in self._selected_tools:
            tool_name = tree.get_value(iterator, TEXT).lower()
            if tool_name in self._selected_tools[host_name.lower()]:
                tree.set(iterator, ACTIVE, True)

    def handle_param_changed(self, source=None, event=None):
        """ 'changed' event on configuration value """
        self._ui.get_widget('save_tool_conf').set_sensitive(True)
        self._ui.get_widget('undo_tool_conf').set_sensitive(True)



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
tool_event.py - the events on tool tab
"""

import gtk
import gobject
import ConfigParser

from platine_manager_gui.view.tool_view import ToolView
from platine_manager_gui.view.popup.infos import error_popup
from platine_manager_core.my_exceptions import ToolException

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class ToolEvent(ToolView):
    """ Events for the tool tab """
    def __init__(self, parent, model, manager_log):
        ToolView.__init__(self, parent, model, manager_log)

    def close(self):
        """ close tool tab """
        self._log.debug("Tool Event: close")
        gobject.source_remove(self._refresh_tool_tree)
        self._log.debug("Tool Event: closed")

    def on_tool_select(self, selection):
        """ callback called when a tool is selected """
        (tree, iterator) = selection.get_selected()
        if iterator is None:
            self._desc_txt.set_markup('')
            self._config_view.hide_all()
            return

        if tree.iter_parent(iterator) == None:
            start = "<span size='x-large' foreground='#1088EB'><b>"
            end = "</b></span>"
            # host
            if not tree.iter_has_child(iterator):
                self._desc_txt.set_markup("%sThis host has no available " \
                                             "tool%s" % (start, end))
            else:
                self._desc_txt.set_markup("%sDeploy host to see " \
                                             "available tools%s" % (start, end))

            self._config_view.hide_all()
        else:
            name = tree.get_value(iterator, TEXT).lower()
            parent_name = tree.get_value(tree.iter_parent(iterator), TEXT)
            parent_name = parent_name.lower()
            # tool
            tool_name = tree.get_value(iterator, TEXT).lower()
            tool_model = self._tools_infos[parent_name][tool_name]

            description = tool_model.get_description()
            if tool_model.get_state() is None or \
               tool_model.get_description() is None:
                warning =  "<span size='x-large' background='red' " + \
                           "foreground='white'>" + \
                           "This module failed to load\n\n</span>"
                if description is not None:
                    description = warning + description
                else:
                    description = warning

            self._desc_txt.set_markup(description)

            self.display_config_view(name, parent_name, iterator)

    def col1_toggled_cb(self, cell, path):
        """ sets the toggled state on the toggle button to true or false
            and modify tool state from None to False or conversely """
        self._ui.get_widget('save_tool_conf').set_sensitive(True)
        self._ui.get_widget('undo_tool_conf').set_sensitive(True)
        self._tool_lock.acquire()
        curr_iter = self._treestore.get_iter_from_string(path)
        curr_name = self._treestore.get_value(curr_iter, TEXT)
        parent_iter = self._treestore.iter_parent(curr_iter)
        parent_name = self._treestore.get_value(parent_iter, TEXT)

        # modify ACTIVE property
        val = not self._treestore.get_value(curr_iter, ACTIVE)
        self._treestore.set(curr_iter, ACTIVE, val)

        self._log.debug("tool %s toggled with host %s" %
                        (curr_name, parent_name))

        if not val:
            self._selected_tools[parent_name.lower()].remove(curr_name.lower())
        else:
            self._selected_tools[parent_name.lower()].append(curr_name.lower())

        self._tool_lock.release()

    def display_config_view(self, tool_name, host_name, iterator):
        """ display the configuration as read in configuration file
            (should be used with gobject.idle_add outside gtk handlers) """
        tool_model = self._tools_infos[host_name][tool_name]

        config_parser = tool_model.get_config_parser()
        if config_parser is None:
            self._treestore.set(iterator, ACTIVATABLE, False, ACTIVE, False)
            self._config_view.hide_all()
            return

        if tool_model.get_conf_view() is not None:
            vbox = tool_model.get_conf_view()
        else:
            # create a configuration box as for configuration tab
            # vbox/frame/align(label:section)/vbox/hbox/label:option/entry:value
            vbox = gtk.VBox()
            for section in config_parser.sections():
                self._log.debug("tool %s host %s: add section: '%s'" %
                                (tool_name, host_name, section))
                frame = gtk.Frame()
                frame.set_label_align(0, 0.5)
                frame.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
                alignment = gtk.Alignment(0.5, 0.5, 1, 1)
                label = gtk.Label()
                label.set_markup("<b>%s</b>" % section)
                frame.set_label_widget(label)
                align_vbox = gtk.VBox()
                for opt in config_parser.options(section):
                    self._log.debug("tool %s host %s: add option: '%s'" %
                                    (tool_name, host_name, opt))
                    hbox = gtk.HBox()
                    opt_label = gtk.Label()
                    opt_label.set_markup(opt)
                    opt_label.set_alignment(0.0, 0.5)
                    opt_label.set_size_request(150, -1)
                    hbox.pack_start(opt_label, expand=False,
                                    fill=False, padding=5)
                    entry = gtk.Entry()
                    entry.set_inner_border(gtk.Border(1, 1, 1, 1))
                    entry.set_text(config_parser.get(section, opt))
                    entry.connect('changed', self.handle_param_chanded)
                    entry.set_name('%s_%s_%s_%s' %
                                   (host_name.replace('_', '-'),
                                    tool_name.replace('_', '-'),
                                    section.replace('_', '-'),
                                    opt.replace('_', '-')))
                    entry.set_size_request(350, -1)
                    hbox.pack_start(entry, expand=False,
                                    fill=False, padding=5)
                    align_vbox.pack_end(hbox, expand=False,
                                        fill=False, padding=10)
                alignment.add(align_vbox)
                frame.add(alignment)
                vbox.pack_start(frame, expand=False,
                                fill=False, padding=10)

            tool_model.set_conf_view(vbox)

        if vbox != self._current_vbox:
            self._config_view.hide_all()
            self._config_view.pack_start(vbox)
            if self._current_vbox is not None:
                self._config_view.remove(self._current_vbox)
            self._config_view.show_all()
            self._current_vbox = vbox
        else:
            self._config_view.show_all()

    def on_save_tool_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on save button """
        self._tool_lock.acquire()
        for host in self._model.get_hosts_list():
            host_name = host.get_name()
            # reset the tools
            for tool in host.get_tools():
                tool.set_selected(False)
            # mark selected tools
            if host_name in self._selected_tools.keys():
                for tool_name in self._tools_infos[host_name].keys():
                    if tool_name in self._selected_tools[host_name]:
                        self._log.debug("tool %s enable for %s" %
                                        (tool_name.upper(),
                                         host_name.upper()))
                        self._tools_infos[host_name][tool_name].set_selected()

        if len(self._parameters_entry_changed) > 0:
            # save conf for each tool except unsupported
            for host_name in self._tools_infos.keys():
                for tool_name in self._tools_infos[host_name].keys():
                    try:
                        self._log.debug("save %s config for %s" %
                                        (tool_name, host_name))
                        tool_model = self._tools_infos[host_name][tool_name]
                        if tool_model.get_state() is not None:
                            self.set_config(host_name, tool_name,
                                            tool_model.get_config_parser())
                            tool_model.save_config()
                    except IOError, (errno, strerror):
                        error = "failed to open file to save the new " \
                                 "configuration"
                        text = "File: %s\nReason: %s" % \
                               (tool_model.get_conf_src(), strerror)
                        error_popup(error, text)
                        self.undo_conf_for_tool(host_name, tool_name)
                        continue
                    except Exception, msg:
                        self._log.debug("save config exception: %s" % msg)
                        self.undo_conf_for_tool(host_name, tool_name)
                        continue

        # do that to copy contents else saved tools will contain references on
        # selected tools
        for host_name in self._selected_tools:
            self._saved_tools[host_name] = list(self._selected_tools[host_name])

        self._tool_lock.release()
        self._ui.get_widget('save_tool_conf').set_sensitive(False)
        self._ui.get_widget('undo_tool_conf').set_sensitive(False)

    def undo_conf_for_tool(self, host_name, tool_name):
        """ reset configuration for a specific tool
            (should be used with gobject.idle_add outside gtk handlers) """
        self._tools_infos[host_name][tool_name].reload_conf()
        self.on_tool_select(self._treeselection)

    def on_undo_tool_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        self._tool_lock.acquire()
        # do that to copy contents else selected tools will contain references
        # on saved tools
        for host_name in self._saved_tools:
            self._selected_tools[host_name] = list(self._saved_tools[host_name])
        # delete tools vbox to reload tool info
        for host_name in self._tools_infos.keys():
            for tool in self._tools_infos[host_name].keys():
                self._tools_infos[host_name][tool].reload_conf()

        self._treestore.foreach(self.select_saved)

        self.on_tool_select(self._treeselection)

        self._tool_lock.release()
        self._ui.get_widget('save_tool_conf').set_sensitive(False)
        self._ui.get_widget('undo_tool_conf').set_sensitive(False)

    def select_saved(self, tree, path, iterator):
        """ toggled the elements as in saved_tools list
            (used in callback so no need to use locks) """
        parent = tree.iter_parent(iterator)
        if parent != None:
            tree.set(iterator, ACTIVE, False)
            host_name = tree.get_value(parent, TEXT).lower()
            if host_name in self._selected_tools:
                tool_name = tree.get_value(iterator, TEXT).lower()
                if tool_name in self._selected_tools[host_name]:
                    tree.set(iterator, ACTIVE, True)

    def handle_param_chanded(self, source=None, event=None):
        """ 'changed' event on configuration value """
        # add the entry to parameters list which changed
        self._parameters_entry_changed.append(source)

        self._ui.get_widget('save_tool_conf').set_sensitive(True)
        self._ui.get_widget('undo_tool_conf').set_sensitive(True)

    def set_config(self, host_name, tool_name, conf_parser):
        """ set a tool configuration
            (should be used with gobject.idle_add outside gtk handlers) """
        for param in self._parameters_entry_changed:
            name = param.get_name()
            if not name.startswith('%s_%s' % (host_name.replace('_', '-'),
                                              tool_name.replace('_', '-'))):
                continue
            name = name.split('_')
            if len(name) != 4:
                error = "Error when updating parameter, " \
                        "your configuration won't be saved"
                text = "parameter data: '%s'" % name
                error_popup(error, text)
                raise ToolException
            val = param.get_text()

            # name = hostname_toolname_section_option
            try:
                conf_parser.set(name[2].replace('-', '_'),
                                name[3].replace('-', '_'), val)
            except ConfigParser.Error, msg:
                error = "Error when updating parameter, " \
                        "your configuration won't be saved"
                text = "parameter data: '%s', Error: '%s'" % \
                       (name, msg)
                error_popup(error, text)
                raise ToolException

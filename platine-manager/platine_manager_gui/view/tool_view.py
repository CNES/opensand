#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
tool_view.py - the tool tab view
"""

import gtk
import gobject
import threading

from platine_manager_gui.view.window_view import WindowView

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

#TODO class to create a treeview (also used in probes)
class ToolView(WindowView):
    """ Elements of the tool tab """
    def __init__(self, parent, model, manager_log):
        """constructor, initialization """
        WindowView.__init__(self, parent)

        self._model = model
        self._log = manager_log

        # create the tools elements and thread to refresh it
        self._tools_infos = {} #{host_name:{tool_name:ToolModel}}
        # the list of tools parameters widgets that changed identified by a name
        self._parameters_entry_changed = []
        self._tool_lock = threading.Lock()
        # the tools we have selected
        self._selected_tools = {}
        # the selected tools once the configuration has been saved
        self._saved_tools = {}

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

            self._current_vbox = None

            self._treeview = self._ui.get_widget('tools_selection_tree')
            #Create the listStore Model to use with the treeView
            self._treestore = gtk.TreeStore(str, gobject.TYPE_BOOLEAN,
                                                 gobject.TYPE_BOOLEAN,
                                                 gobject.TYPE_BOOLEAN)
            #Attatch the model to the treeView
            self._treeview.set_model(self._treestore)

            cell_renderer = gtk.CellRendererText()

            # Connect check box on the treeview
            self._cell_renderer_toggle = gtk.CellRendererToggle()
            self._cell_renderer_toggle.connect('toggled', self.col1_toggled_cb)

            column = gtk.TreeViewColumn('Tools', cell_renderer, text=TEXT)
            column.set_resizable(True)
            column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)

            column_toggle = gtk.TreeViewColumn('Selected',
                                               self._cell_renderer_toggle,
                                               visible=VISIBLE, active=ACTIVE,
                                               activatable=ACTIVATABLE)
            column_toggle.set_resizable(True)
            column_toggle.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)

            self._treeview.append_column(column)
            self._treeview.append_column(column_toggle)

            # add a column to avoid large toggle column
            col = gtk.TreeViewColumn('')
            self._treeview.append_column(col)

            # get the tree selection
            self._treeselection = self._treeview.get_selection()
            self._treeselection.set_mode(gtk.SELECTION_SINGLE)
            self._treeselection.connect('changed', self.on_tool_select)

            # disable save button
            self._ui.get_widget('save_tool_conf').set_sensitive(False)
            self._ui.get_widget('undo_tool_conf').set_sensitive(False)

        self._refresh_tool_tree = gobject.timeout_add(1000, self.refresh)
        #TODO add a field to specify additional files to deploy, it should be preloaded
        #     from /usr/share/platine/tools/NAME/HOST/files
        #TODO in dev mode: add an option to edit the binary file to deploy
        #     (do the same in advanced config for hosts binaries)

    def col1_toggled_cb(self, cell, path):
        """ defined in tool_event """
        pass

    def on_tool_select(self, selection):
        """ defined in tool_event """
        pass

    def refresh(self):
        """ refresh the tool list per host"""
        self._tool_lock.acquire()
        # disable tool selection when running
        if self._model.is_running():
            #TODO fix it
            self._cell_renderer_toggle.set_property('activatable', False)

        real_names = []
        for host in self._model.get_hosts_list():
            real_names.append(host.get_name())

        new_tools = {}
        for host in self._model.get_hosts_list():
            if host.get_name() not in self._tools_infos.keys():
                # initialize the list in tools dictionaries
                self._tools_infos[host.get_name()] = {}
                new_tools[host.get_name()] = []
                self._selected_tools[host.get_name()] = []
                self._saved_tools[host.get_name()] = []
                for tool in host.get_tools():
                    self._tools_infos[host.get_name()][tool.get_name()] = \
                                                                        tool
                    new_tools[host.get_name()].append(tool.get_name())

        old_host_names = set(self._tools_infos.keys()) - set(real_names)
        for host_name in old_host_names:
            del self._tools_infos[host_name]

        gobject.idle_add(self.update_selection_tree,
                         new_tools, old_host_names)

        self._tool_lock.release()

        # continue to refresh
        return True

    def update_selection_tree(self, new, old):
        """ update the tools per host tree view
            (should be used with gobject.idle_add outside gtk handlers) """
        tree = self._treestore
        for host_name in new.keys():
            self._log.debug("add tools for " + host_name.upper())
            # append a top element in the treestore
            # first SAT, then GW and ST
            if host_name == 'sat':
                top_iter = tree.insert(None, 0)
            elif host_name == 'gw':
                top_iter = tree.insert(None, 1)
            else:
                top_iter = tree.append(None)
            tree.set(top_iter, TEXT, host_name.upper(),
                               VISIBLE, False,
                               ACTIVE, False,
                               ACTIVATABLE, False)
            for tool_name in new[host_name]:
                tool_iter = tree.append(top_iter)
                activatable = True
                if self._tools_infos[host_name][tool_name].get_state() is None:
                    activatable = False
                tree.set(tool_iter, TEXT, tool_name.upper(),
                                    VISIBLE, True,
                                    ACTIVE, False,
                                    ACTIVATABLE, activatable)

        for host_name in old:
            self._log.debug("remove tools for " + host_name.upper())
            iterator = tree.get_iter_first()
            while iterator is not None and \
                  tree.get_value(iterator, TEXT) != host_name.upper():
                iterator = tree.iter_next(iterator)
            if iterator is not None:
                tree.remove(iterator)


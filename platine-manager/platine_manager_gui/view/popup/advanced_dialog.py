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
advanced_dialog.py - The Platine advanced configuration
"""

import gobject
import gtk
import threading

from platine_manager_gui.view.window_view import WindowView
from platine_manager_gui.view.popup.infos import error_popup
from platine_manager_core.my_exceptions import ModelException, XmlException
from platine_manager_gui.view.utils.config_elements import ConfigurationTree, \
                                                           ConfigurationNotebook

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class AdvancedDialog(WindowView):
    """ an advanced configuration window """
    def __init__(self, model, manager_log):
        WindowView.__init__(self, None, 'advanced_dialog')

        self._dlg = self._ui.get_widget('advanced_dialog')
        self._model = model
        self._log = manager_log
        self._tree = None
        self._config_view = None
        self._current_notebook = None
        self._hosts_name = []
        self._enabled = []
        self._saved = []
        self._host_lock = threading.Lock()
        self._refresh_tree = None

    def go(self):
        """ run the window """
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
            self.close()
            return
        self._dlg.set_title("Advanced configuration - PtManager")
        self._dlg.set_icon_name('gtk-properties')
        self._dlg.run()

    def close(self):
        """ close the window """
        if self._refresh_tree is not None:
            gobject.source_remove(self._refresh_tree)
        self._dlg.destroy()

    def load(self):
        """ load the hosts configuration """
        try:
            self.reset()
        except ModelException:
            raise

        # add a widget to the sroll view, once it will be destroyed the
        # scroll view will also be destroyed because it won't have
        # reference anymore with the vbox we can add the new widget before
        # destroying the older
        self._config_view = gtk.VBox()
        host_config = self._ui.get_widget('host_config')
        host_config.add_with_viewport(self._config_view)

        treeview = self._ui.get_widget('hosts_selection_tree')
        self._tree = ConfigurationTree(treeview, 'Host', 'Enabled',
                                       self.on_host_selected,
                                       self.toggled_cb)
        for host in [elt for elt in self._model.get_hosts_list()
                         if elt.is_enabled()]:
            self._enabled.append(host)

        # add the global configuration
        gobject.idle_add(self._tree.add_host, self._model.get_conf(),
                         {})
        # update tree immediatly then add a periodic update
        self.update_host_tree()
        self._refresh_tree = gobject.timeout_add(1000,
                                                 self.update_host_tree)

        # disable apply button
        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)

    def reset(self):
        """ reset the advanced configuration """
        for host in self._model.get_hosts_list() + [self._model.get_conf()]:
            adv = host.get_advanced_conf()
            if adv is None:
                raise ModelException("cannot get advanced configuration for %s"
                                     % host.get_name().upper())
            try:
                adv.reload_conf()
            except ModelException:
                raise

    def update_host_tree(self):
        """ update the host tree """
        self._host_lock.acquire()
        # disable tool selection when running
        if self._model.is_running():
            self._tree.disable_all()

        for host in [elt for elt in self._model.get_hosts_list()
                         if elt.get_name() not in self._hosts_name]:
            self._hosts_name.append(host.get_name())
            gobject.idle_add(self._tree.add_host, host)

        real_names = []
        for host in self._model.get_hosts_list():
            real_names.append(host.get_name())

        old_host_names = set(self._hosts_name) - set(real_names)
        for host_name in old_host_names:
            gobject.idle_add(self._tree.del_host, host_name)
            self._hosts_name.remove(host_name)

        self._host_lock.release()

        # continue to refresh
        return True

    def on_host_selected(self, selection):
        """ callback called when a host is selected """
        (tree, iterator) = selection.get_selected()
        if iterator is None:
            self._config_view.hide_all()
            return

        name = tree.get_value(iterator, TEXT).lower()
        host = self._model.get_host(name)
        if host is None:
            error_popup("cannot find host model for %s" % name.upper())
            self._config_view.hide_all()
            return

        adv = host.get_advanced_conf()
        config = adv.get_configuration()
        if config is None:
            tree.set(iterator, ACTIVATABLE, False, ACTIVE, False)
            self._config_view.hide_all()
            return

        notebook = adv.get_conf_view()
        if notebook is None:
            notebook = ConfigurationNotebook(config, self.handle_param_chanded)

        adv.set_conf_view(notebook)
        if notebook != self._current_notebook:
            self._config_view.hide_all()
            self._config_view.pack_start(notebook)
            if self._current_notebook is not None:
                self._config_view.remove(self._current_notebook)
            self._config_view.show_all()
            self._current_notebook = notebook
        else:
            self._config_view.show_all()

    def toggled_cb(self, cell, path):
        """ enable host toggled callback """
        # modify ACTIVE property
        curr_iter = self._tree.get_iter_from_string(path)
        name = self._tree.get_value(curr_iter, TEXT).lower()
        val = not self._tree.get_value(curr_iter, ACTIVE)
        self._tree.set(curr_iter, ACTIVE, val)

        self._log.debug("host %s toggled" % name)
        host = self._model.get_host(name)
        if host is None:
            return
        if val:
            self._enabled.append(host)
        else:
            self._enabled.remove(host)

        self._ui.get_widget('apply_advanced_conf').set_sensitive(True)

    def on_apply_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on apply button """
        self._host_lock.acquire()
        #TODO currently this button destroy the popup because it emits the
        #     reponse signal, try to avoid destroying it
        #     A solution could be to use a Window instead of a Dialog
        for host in self._model.get_hosts_list() + [self._model.get_conf()]:
            host.enable(False)
            if host in self._enabled:
                host.enable(True)
            name = host.get_name()
            adv = host.get_advanced_conf()
            self._log.debug("save %s advanced configuration" % name)
            notebook = adv.get_conf_view()
            if notebook is None:
                continue
            try:
                notebook.save()
            except XmlException, error:
                error_popup("%s: %s" % (name, error), error.description)

        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)

        # copy the list (do not only copy the address)
        self._saved = list(self._enabled)
        self._host_lock.release()

    def handle_param_chanded(self, source=None, event=None):
        """ 'changed' event on configuration value """
        self._ui.get_widget('apply_advanced_conf').set_sensitive(True)

    def on_save_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on OK button """
        self.on_apply_advanced_conf_clicked(source, event)
        self.close()

    def on_undo_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        self._host_lock.acquire()
        # delete vbox to reload advanced configurations
        try:
            self.reset()
        except ModelException:
            raise

        # copy the list (do not only copy the address)
        self._enabled = list(self._saved)

        self._tree.foreach(self.select_enabled)
        self._host_lock.release()
        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)

    def select_enabled(self, tree, path, iterator):
        """ store the saved enabled value in advanced model
            (used in callback so no need to use locks) """
        tree.set(iterator, ACTIVE, False)
        name = tree.get_value(iterator, TEXT).lower()
        for host in self._enabled:
            if host.get_name() == name:
                host.enable(True)
                tree.set(iterator, ACTIVE, True)


if __name__ == "__main__":
    from platine_manager_core.loggers.manager_log import ManagerLog
    from platine_manager_core.platine_model import Model

    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)
    WindowView(None, 'none', 'platine.glade')
    DIALOG = AdvancedDialog(MODEL, LOGGER)
    DIALOG.go()
    DIALOG.close()

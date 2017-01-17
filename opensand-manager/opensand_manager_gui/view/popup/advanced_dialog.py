#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2016 TAS
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
# Author: Joaquin MUGUERZA / <jbernard@toulouse.viveris.com>

"""
advanced_dialog.py - The OpenSAND advanced configuration
"""

import gobject
import gtk
import threading

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_core.utils import SPOT, ID, GW, TOPOLOGY, GW_types
from opensand_manager_gui.view.utils.config_elements import ConfigurationTree, \
                                                           ConfigurationNotebook, \
                                                           ConfSection

(TEXT, VISIBLE_CHECK_BOX, CHECK_BOX_SIZE, ACTIVE, \
 ACTIVATABLE, VISIBLE, RESTRICTED) = range(7)

class AdvancedDialog(WindowView):
    """ an advanced configuration window """
    def __init__(self, model, manager_log, update_cb):
        WindowView.__init__(self, None, 'advanced_dialog')

        self._dlg = self._ui.get_widget('advanced_dialog')
        self._dlg.set_keep_above(True)
        self._dlg.set_modal(True)
        self._model = model
        self._log = manager_log
        self._host_tree = None
        self._host_conf_view = None
        self._host_list = {}
        self._current_host_frame = None
        self._hosts_name = []
        self._enabled = []
        self._saved = []
        self._host_lock = threading.Lock()
        self._refresh_trees = None
        self._current_host = None
        # modules
        self._modules_tree = {}
        self._modules_conf_view = None
        self._modules_tree_view = None
        self._modules_name = {}
        self._module_label = None
        self._current_module_notebook = None
        self._all_modules = False
        self._show_hidden = False
        self._update_cb = update_cb
        self._vbox = self._ui.get_widget('advanced_vbox')
        info = gtk.InfoBar()
        self._vbox.pack_start(info, expand=False, fill=False, padding=0)
        sep = self._ui.get_widget('advanced_separator')
        self._vbox.reorder_child(sep, 2)
        self._vbox.reorder_child(info, 1)
        info.set_message_type(gtk.MESSAGE_INFO)
        msg = gtk.Label("Do not forget to configure plugins if necessary")
        info.get_content_area().pack_start(msg, False, False)
        def ok(source = None, event = None):
            source.hide_all()
        info.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
        info.connect("response", ok)
        self._vbox.show_all()

    def go(self):
        """ run the window """
        try:
            gobject.idle_add(self.load,priority=gobject.PRIORITY_HIGH_IDLE)
        except ModelException, msg:
            error_popup(str(msg))
        self._dlg.set_title("Advanced configuration - OpenSAND Manager")
        self._dlg.set_icon_name(gtk.STOCK_PREFERENCES)
        # show advanced mode elements
        widget = self._ui.get_widget("hide_checkbutton")
        widget.set_visible(self._model.get_adv_mode())
        self._dlg.run()

    def close(self):
        """ close the window """
        if self._refresh_trees is not None:
            gobject.source_remove(self._refresh_trees)
        self._dlg.destroy()

    def on_advanced_dialog_delete_event(self, source=None, event=None):
        """ delete-event on window """
        self.close()

    def load(self):
        """ load the hosts configuration """
        self.reset()

        # add a widget to the scroll view, once it will be destroyed the
        # scroll view will also be destroyed because it won't have
        # reference anymore with the vbox we can add the new widget before
        # destroying the older
        self._host_conf_view = gtk.VBox()
        host_config = self._ui.get_widget('host_config')
        host_config.add_with_viewport(self._host_conf_view)

        treeview = self._ui.get_widget('hosts_selection_tree')
        self._host_tree = ConfigurationTree(treeview, 'Host', 
                                            self.on_host_selected,
                                            self.toggled_cb, 
                                            self._model.get_adv_mode())
        for host in [elt for elt in self._model.get_hosts_list()
                         if elt.is_enabled()]:
            self._enabled.append(host)

        # add the global configuration
        self._host_tree.add_host(self._model,{})

        host = self._model.get_host(self._model.get_name())
        self.add_host_children(host)
        
        host = self._model.get_host(TOPOLOGY)
        self._host_tree.add_host(host,{})
        self.add_host_children(host)

        if not self._model.get_adv_mode():
            treeview = self._host_tree.get_treeview()
            column = treeview.get_column(1)
            column.set_visible(False)

        # get the modules tree
        self._modules_conf_view = gtk.VBox()
        modules_config = self._ui.get_widget('plugins_config')
        modules_config.add_with_viewport(self._modules_conf_view)
        self._modules_tree_view = self._ui.get_widget('plugins_tree')

        # update trees immediatly then add a periodic update
        self.update_trees()
        self.update_restrictions()
        self._refresh_trees = gobject.timeout_add(1000,
                                                 self.update_trees)

        # disable apply button
        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)


    def reset(self):
        """ reset the advanced configuration """
        self._host_lock.acquire()
        for host in self._model.get_hosts_list() + [self._model]:
            adv = host.get_advanced_conf()
            if adv is None:
                self._host_lock.release()
                error_popup("cannot get advanced configuration for %s" %
                            host.get_name().upper())
                self._host_lock.acquire()
                continue

            try:
                adv.reload_conf(self._model.get_scenario())
            except ModelException, msg:
                self._host_lock.release()
                error_popup("error when reloading %s advanced configuration: %s"
                            % (host.get_name().upper(), msg))
                self._host_lock.acquire()

            # remove modules configuration view to reload them
            for module in host.get_modules():
                module.set_conf_view(None)
        self._host_lock.release()

    def update_trees(self):
        """ update the host and modules trees """
        self._host_lock.acquire()
        
        # add host and its children
        for host in [elt for elt in self._model.get_hosts_list() 
                     if elt.get_name() not in self._hosts_name]:
            name = host.get_name()
            self._hosts_name.append(name)
        
            self._host_tree.add_host(host, None)
            self.add_host_children(host)

        real_names = []
        for host in self._model.get_hosts_list():
            real_names.append(host.get_name())

        old_host_names = set(self._hosts_name) - set(real_names)
        for host_name in old_host_names:
            gobject.idle_add(self._host_tree.del_elem, host_name.upper())
            self._hosts_name.remove(host_name)
            # old host, remove module tree
            if host_name in self._modules_tree:
                del self._modules_tree[host_name]

        # update modules
        self.update_modules_tree()
        self._host_lock.release()

        # continue to refresh
        return True

    def add_host_children(self, host):
        list_children = []
        adv = host.get_advanced_conf()
        config = None
        if adv is not None:
            config = adv.get_configuration()
            if config is not None:
                for section in config.get_sections():
                    list_parent = []
                    list_parent.append(host.get_name().upper())
                    name = config.get_name(section)
                    list_parent.append(name)
                    list_children.append(name)

                    if not config.do_hide_adv(name,
                                              self._model.get_adv_mode()) :
                        self._host_tree.add_child(name,
                                                  [host.get_name().upper()],
                                                  config.do_hide(name), True)
                    
                    else:
                         self._host_tree.add_child(name,
                                                   [host.get_name().upper()],
                                                   True, True)
   

                    for key in config.get_keys(section):
                        if key.tag == SPOT or key.tag == GW:
                            gw = ""
                            if key.get(GW) is not None:
                                gw = "_gw"+ key.get(GW)
                            gobject.idle_add(self._host_tree.add_child,
                                             key.tag+key.get(ID)+gw,
                                             list_parent,
                                             config.do_hide(name), True,
                                             priority=gobject.PRIORITY_HIGH_IDLE+40)

                # create view associate to host children
                conf_sections = {} 
                self.create_conf_section(conf_sections, config, host.get_name())
                adv.set_conf_view(conf_sections)


        self._host_list[host.get_name()] = list_children

    def update_modules_tree(self):
        """ update the modules tree """
        if self._current_host is None:
            return

        used_names = []
        host_name = self._current_host.get_name()
        modules = self.get_used_modules()
        # Not module for this Host
        if not modules:
            return
        
        if not host_name in self._modules_tree:
            # new host, add a module tree
            treeview = gtk.TreeView()
            self._modules_tree[host_name] = \
                    ConfigurationTree(treeview, 'Plugin',
                                      self.on_module_selected, None)
            self._modules_name[host_name] = []
        tree =  self._modules_tree[host_name]

        for module in modules:
            module_name = module.get_name()
            used_names.append(module_name)
            if module_name in self._modules_name[host_name]:
                # module already loaded in tree
                continue
            # second argument to say if we use the module_type in tree
            gobject.idle_add(tree.add_module, module, self._all_modules)
            # add module in dic
            self._modules_name[host_name].append(module_name)

        if self._all_modules:
            # nothing more to do
            return

        old_names = set(self._modules_name[host_name]) - set(used_names)
        for module_name in old_names:
            gobject.idle_add(tree.del_elem, module_name)
            self._modules_name[host_name].remove(module_name)

    def get_used_modules(self):
        """ get the modules used by a host """
        if self._current_host.get_modules() != []:
            all_modules = list(self._current_host.get_modules())
            # header modifications modules have their configuration in st and gw
            # but a global target, so get them
            all_modules += self._model.get_global_lan_adaptation_modules().values()
        else:
            all_modules = self._model.get_global_lan_adaptation_modules().values()
        all_modules += self._model.get_global_satdelay_modules().values()

        if self._all_modules:
            return all_modules

        with_phy_layer = self._model.get_conf().get_enable_physical_layer()
        modules = []
        adv = self._current_host.get_advanced_conf()
        try:
            modules += adv.get_stack("lan_adaptation_schemes",
                                     'proto').itervalues()
        except ModelException:
            pass
        try:
            modules += adv.get_stack("return_up_encap_schemes",
                                     'encap').itervalues()
        except ModelException:
            pass
        try:
            modules += adv.get_stack("forward_down_encap_schemes",
                                     'encap').itervalues()
        except ModelException:
            pass
        try:
            modules += adv.get_params("delay")
        except ModelException:
            pass
        if with_phy_layer == "true":
            modules += adv.get_params("attenuation_model_type")
            modules += adv.get_params("minimal_condition_type")
            modules += adv.get_params("error_insertion_type")
        used_modules = []
        for module in all_modules:
            if module.get_name() in modules:
                used_modules.append(module)
        return used_modules

    def create_conf_section(self, conf_sections, config, host_name):
        for section in config.get_sections():
            global_section = False
            # look for spot section
            for key in config.get_keys(section):
                if key.tag == SPOT or key.tag == GW:
                    if key.tag == SPOT:
                        gw = ""
                        if key.get(GW) != None:
                            gw ="_"+GW+key.get(GW)

                        conf_sections[host_name + "." + config.get_name(section) +
                                      "." + key.tag+key.get(ID) + gw] = \
                                ConfSection(section, config, host_name,
                                            self._model.get_adv_mode(),
                                            self._model.get_scenario(),
                                            self.handle_param_chanded,
                                            self._model.handle_file_changed,
                                            key.get(ID),
                                            key.get(GW))
                    elif key.tag == GW:
                        key.tag+key.get(ID) + gw
                        conf_sections[host_name + "." + config.get_name(section) +
                                      "." + key.tag+key.get(ID) + gw] = \
                                ConfSection(section, config, host_name,
                                            self._model.get_adv_mode(),
                                            self._model.get_scenario(),
                                            self.handle_param_chanded,
                                            self._model.handle_file_changed,
                                            None, key.get(ID))

                    # hidden section
                    if config.do_hide(config.get_name(section)):
                        conf_sections[host_name + "." + config.get_name(section) +
                                      "." + key.tag + key.get(ID) + gw
                                     ].set_hidden(not self._show_hidden)
                    # restrictions section
                    restriction =  config.get_xpath_restrictions(
                                        config.get_name(section))
                    if restriction is not None:
                        conf_sections[host_name + "." + config.get_name(section) +
                                      "." + key.tag + key.get(ID) + gw
                                     ].add_restriction(conf_sections[host_name + 
                                                "." + config.get_name(section) +
                                                "." + key.tag +key.get(ID)],
                                                restriction)

                else:
                    global_section = True
                
                
            # global section
            if global_section:
                conf_sections[host_name+"."+config.get_name(section)] = \
                ConfSection(section, config, host_name,
                            self._model.get_adv_mode(),
                            self._model.get_scenario(),
                            self.handle_param_chanded,
                            self._model.handle_file_changed)
                    
                # restrictions section
                restriction = config.get_xpath_restrictions(config.get_name(section))
                if restriction is not None:
                    conf_sections[host_name + "." + config.get_name(section)
                                 ].add_restriction(conf_sections[host_name + 
                                                   "." + config.get_name(section)],
                                                   restriction)
                    
                # hidden section
                if config.do_hide(config.get_name(section)):
                    conf_sections[host_name + "." + config.get_name(section)
                                 ].set_hidden(not self._show_hidden)
                

    def on_host_selected(self, selection):
        """ callback called when a host is selected """
        self._modules_conf_view.hide_all()
        for widget in self._modules_tree_view.get_children():
            self._modules_tree_view.remove(widget)

        # selected item
        (tree, iterator) = selection.get_selected()

        if iterator is None:
            self._host_conf_view.hide_all()
            return

        # host name
        tree_path = tree.get_path(iterator)
        host_iter = tree.get_iter(tree_path[0]) 
        host_name = tree.get_value(host_iter, TEXT).lower()
        
        # selected item name
        selected_name = tree.get_value(iterator, TEXT).lower()
        
        # section name
        section_name = host_name
        i = 2
        while i <= len(tree_path):
            it = tree.get_iter(tree_path[:i])
            section_name += "."+tree.get_value(it, TEXT).lower()
            i += 1
        host = self._model.get_host(host_name)
        
        if host is None:
            error_popup("cannot find host model for %s" % selected_name.upper())
            self._host_conf_view.hide_all()
            return
        self._current_host = host

        # get host configuration
        adv = host.get_advanced_conf()
        config = None
        if adv is not None:
            config = adv.get_configuration()
        if config is None:
            tree.set(iterator, ACTIVATABLE, False, VISIBLE, False)
            self._host_conf_view.hide_all()
            return
        
        # create/get sections
        conf_sections = adv.get_conf_view()
        if conf_sections is None:
            conf_sections = {} 
            self.create_conf_section(conf_sections, config, host_name)
            adv.set_conf_view(conf_sections)
        
        self.update_restrictions()

        adv.set_conf_view(conf_sections)
        # collapse/expand row
        if tree.iter_has_child(iterator):
            path =  tree.get_path(iterator)
            if self._host_tree.get_treeview().row_expanded(path):
                self._host_tree.get_treeview().collapse_row(path)
            else:
                self._host_tree.get_treeview().expand_row(path, False)

        if conf_sections.get(section_name) is None:
            self._host_conf_view.hide_all()
        elif conf_sections.get(section_name) != self._current_host_frame:
            self._host_conf_view.hide_all()
            self._host_conf_view.pack_start(conf_sections.get(section_name))
            if self._current_host_frame is not None:
                self._host_conf_view.remove(self._current_host_frame)
            self._current_host_frame = conf_sections.get(section_name)
        self._host_conf_view.show_all()

        self.update_modules_tree()
        if host_name in self._modules_tree.keys():
            self._modules_tree_view.add(self._modules_tree[host_name].get_treeview())
            self._modules_tree_view.show_all()
            # call on_module_selected if a plugin is already selected
            self.on_module_selected(self._modules_tree[host_name].get_selection())

    def on_module_selected(self, selection):
        """ callback called when a host is selected """
        (tree, iterator) = selection.get_selected()
        if self._module_label is not None:
            self._modules_conf_view.remove(self._module_label)
            self._module_label = None
        if iterator is None:
            self._modules_conf_view.hide_all()
            return
        if tree.iter_parent(iterator) == None and self._all_modules:
            # plugin category, nothin to do
            return

        module_name = tree.get_value(iterator, TEXT)
        module = self._current_host.get_module(module_name)
        if module is None:
            self._module_label = gtk.Label()
            self._module_label.set_markup("<span size='x-large' background='red' " +
                                          "foreground='white'>" +
                                          "Cannot find %s module</span>" %
                                          (module_name))
            self._modules_conf_view.pack_start(self._module_label)
            if self._current_module_notebook is not None:
                self._modules_conf_view.remove(self._current_module_notebook)
            self._current_module_notebook = None
            self._modules_conf_view.show_all()
            return

        config = module.get_config_parser()
        if config is None:
            self._module_label = gtk.Label()
            self._module_label.set_markup("<span size='large'>Nothing to " +
                                          "configure for this module</span>")
            self._modules_conf_view.pack_start(self._module_label)
            if self._current_module_notebook is not None:
                self._modules_conf_view.remove(self._current_module_notebook)
            self._current_module_notebook = None
            self._modules_conf_view.show_all()
            return
        notebook = module.get_conf_view()
        if notebook is None:
            notebook = ConfigurationNotebook(config,
                                             self._current_host.get_name().lower(),
                                             self._model.get_adv_mode(),
                                             self._model.get_scenario(),
                                             self._show_hidden,
                                             self.handle_param_chanded,
                                             self._model.handle_file_changed)
        # TODO set tab label red if the host does not declare this module

        module.set_conf_view(notebook)
        if notebook != self._current_module_notebook:
            self._modules_conf_view.hide_all()
            self._modules_conf_view.pack_start(notebook)
            if self._current_module_notebook is not None:
                self._modules_conf_view.remove(self._current_module_notebook)
            self._current_module_notebook = notebook
        self._modules_conf_view.show_all()

    def toggled_cb(self, cell, path):
        """ enable host toggled callback """
        path = self.update_path(path)
        # modify ACTIVE property
        curr_iter = self._host_tree.get_iter_from_string(path)
        name = self._host_tree.get_value(curr_iter, TEXT).lower()
        val = not self._host_tree.get_value(curr_iter, ACTIVE)
        self._host_tree.set(curr_iter, ACTIVE, val)

        self._log.debug("host %s toggled" % name)
        host = self._model.get_host(name)
        if host is None:
            return
        if val:
            self._enabled.append(host)
        elif host in self._enabled:
            self._enabled.remove(host)

        self._ui.get_widget('apply_advanced_conf').set_sensitive(True)

    def update_path(self, path):
        propagate = False
        next_iter = None
        connection_map = {}
        tree_iter = self._host_tree.get_iter_first()
        propagate = False
        while not tree_iter is None:
            host_path = self._host_tree.get_path(tree_iter)
            hide = not self._host_tree.get_value(tree_iter, VISIBLE)

            if hide or propagate: 
                next_iter = self._host_tree.iter_next(tree_iter)
                if next_iter is None:
                    break
                next_path = self._host_tree.get_path(next_iter)
                connection_map[str(host_path[0])] = str(next_path[0])
                propagate = True
            else:
                connection_map[str(host_path[0])] = str(host_path[0])
            tree_iter = self._host_tree.iter_next(tree_iter)
       
        return connection_map[path]


    def on_apply_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on apply button """
        self._host_lock.acquire()
        for host in self._model.get_hosts_list() + [self._model] + \
                    [self._model.get_host(TOPOLOGY)]:
            host.enable(False)
            if host in self._enabled:
                host.enable(True)
            name = host.get_name()
            adv = host.get_advanced_conf()
            list_sections = None
            if adv is not None:
                self._log.debug("save %s advanced configuration" % name)
                list_sections = adv.get_conf_view()
            if list_sections is None:
                continue
            try:
                for section_name in list_sections:
                    list_sections[section_name].save()
            except XmlException, error:
                self._host_lock.release()
                error_popup("%s: %s" % (name, error), error.description)
                self._host_lock.acquire()
            except BaseException, error:
                self._host_lock.release()
                error_popup("Unknown exception when saving configuration: %s" %
                            (error))
                self._host_lock.acquire()

            # remove modules configuration view to reload them
            modules = host.get_modules()
            for module in modules:
                try:
                    self._log.debug("Save module %s on %s" %
                                    (module.get_name(), name))
                    module.save()
                except XmlException, error:
                    self._host_lock.release()
                    error_popup("Cannot save %s module on %s: %s" %
                                (module.get_name(), name,
                                 error.description))
                    self._host_lock.acquire()
            try:
                adv.update_conf()
            except XmlException, error:
                self._host_lock.release()
                error_popup("Cannot save %s configuration: %s" %
                            (name, error))
                self._host_lock.acquire()

        self.update_restrictions()
        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)

        # copy the list (do not only copy the address)
        self._saved = list(self._enabled)
        self._host_lock.release()
        gobject.idle_add(self._update_cb)
        # tell model that file changed has been saved
        self._model.conf_apply()

    def handle_param_chanded(self, source=None, event=None):
        """ 'changed' event on configuration value """
        self._ui.get_widget('apply_advanced_conf').set_sensitive(True)

    def on_save_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on OK button """
        self.on_apply_advanced_conf_clicked(source, event)
        self.close()

    def on_undo_advanced_conf_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        # tell model that file changed has been saved
        self._model.conf_undo()
        # delete vbox to reload advanced configurations
        self.reset()

        self._host_lock.acquire()
        # copy the list (do not only copy the address)
        self._enabled = list(self._saved)

        self._host_tree.foreach(self.select_enabled)
        self._host_lock.release()
#        self.on_host_selected(self._host_tree.get_selection())
#        self._ui.get_widget('apply_advanced_conf').set_sensitive(False)
        gobject.idle_add(self._update_cb)
        self.close()

    def on_plugins_checkbutton_toggled(self, source=None, event=None):
        """ The see all plugins per host checkbutton has been toggled """
        self._all_modules = not self._all_modules
        for widget in self._modules_tree_view.get_children():
            self._modules_tree_view.remove(widget)
        self._modules_tree = {}
        self._modules_name = {}
        self.update_modules_tree()
        if self._current_host is not None:
            treeview =  self._modules_tree[self._current_host.get_name()].get_treeview()
            self._modules_tree_view.add(treeview)
            self._modules_tree_view.show_all()

    def on_hide_checkbutton_toggled(self, source=None, event=None):
        """ The see all sections checkbutton has been toggled """
        self._show_hidden = not self._show_hidden
        self._host_tree.set_hidden(not self._show_hidden)
        
        for host_name in self._modules_tree:
            self._modules_tree[host_name].set_hidden(not self._show_hidden)
        
        for host in self._model.get_hosts_list() + [self._model] + \
                    [self._model.get_host(TOPOLOGY)]:
            adv = host.get_advanced_conf()
            if adv is None:
                continue
            list_view = adv.get_conf_view()
            if list_view is None:
                continue
            for view in list_view:
                list_view[view].set_hidden(not self._show_hidden)
        self.update_restrictions()


    def select_enabled(self, tree, path, iterator):
        """ store the saved enabled value in advanced model
            (used in callback so no need to use locks) """
        tree.set(iterator, VISIBLE, False)
        name = tree.get_value(iterator, TEXT).lower()
        for host in self._enabled:
            if host.get_name() == name:
                host.enable(True)
                tree.set(iterator, VISIBLE, True)

    def update_restrictions(self):
        """ update the restrictions in configuration """
        configs = []
        rows = {}
        # get all the configurations
        for host in [self._model] + \
                    [self._model.get_host(TOPOLOGY)]:
            adv = host.get_advanced_conf()
            configs.append(adv.get_configuration())

        for host in self._model.get_hosts_list() + [self._model] + \
                    [self._model.get_host(TOPOLOGY)]:
            # get notebooks to update their restrictions
            adv = host.get_advanced_conf()
            list_view = adv.get_conf_view()
            if list_view is None:
                continue
            for view in list_view:
                restrictions = list_view[view].get_restrictions()
                new_restrictions = {}
                # get all the widget concerned by restriction
                for (widget, restriction) in restrictions.items():
                    restricted = False
                    # if hidden widgets are shown, only set all the restrictions
                    # to False in order to force widget display
                    if self._show_hidden:
                        new_restrictions[widget] = restricted
                        continue
                    # get the restriction parameter and the value for wich
                    # the widget is not hidden
                    for (xpath, val) in restriction.items():
                        # try to find the parameter in all configurations
                        for config in configs + [adv.get_configuration()]:
                            xpath = xpath.replace("spot.","spot[@id='"+view[-1]+"']/")
                            elem = config.get("//" + xpath.replace(".", "/"))
                            if elem is not None:
                                break
                        if elem is None:
                            continue
                        # a parameter was not found,
                        # we do a & between all parameters, so if one is not found, 
                        # we do not display the widget
                        if config.get_value(elem) != val:
                            restricted = True
                   
                    new_restrictions[widget] = restricted
                   
                    # hide Tree row for section restricted
                    if isinstance(widget, ConfSection):
                        rows[view] = restricted
                list_view[view].set_restrictions(new_restrictions)

        # Hide restricted confSection / Tree
        self._host_tree.set_restricted(rows)

if __name__ == "__main__":
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model

    def nothing():
        pass

    LOGGER = ManagerLog(7, True, True, True)
    MODEL = Model(LOGGER)
    MODEL.set_adv_mode(True)
    WindowView(None, 'none', 'opensand.glade')
    DIALOG = AdvancedDialog(MODEL, LOGGER, nothing)
    DIALOG.go()
    DIALOG.close()

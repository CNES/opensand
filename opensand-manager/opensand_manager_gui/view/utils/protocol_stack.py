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
protocol_stack.py - The protocol stack view
"""

import gtk
import gobject

from opensand_manager_core.my_exceptions import ConfException

class ProtocolStack():
    """ The protocol stack for the configuration view """
    def __init__(self, vbox_stack, modules, modif_callback,
                 vbox_header_modif=None, frame_header_modif=None):
        self._vbox_stack = vbox_stack
        self._vbox_header_modif = vbox_header_modif
        self._frame = frame_header_modif
        # do a copy to avoid deleting elements from model
        self._modules = dict(modules)
        self._header_modif_plugins = {}
        # remove header modification plugins from stack
        for module in self._modules:
            if self._modules[module].get_condition('header_modif'):
                self._header_modif_plugins[self._modules[module]] = None
        # separate stack modules and header modif modules
        for module in self._header_modif_plugins:
            del self._modules[module.get_name()]
        # the list of selected encapsulation protocols as in XML configuration
        # file {pos:name}
        self._displayed_stack = {}
        self._payload = ''
        self._emission_std = ''
        self._stack = {}
        self._modif_cb = modif_callback

    def load(self, stack, payload='', emission_std=''):
        """ load or reload the protocol stack from configuration """
        self._payload = payload.lower()
        self._emission_std = emission_std.lower()
        self.reset()
        upper_val = ""
        idx_stack = 0
        # remove header modif plugins from active stack and keep them for
        # setting buttons' state
        enabled_header_modif = []
        for module in self._header_modif_plugins:
            name = module.get_name()
            for pos in stack:
                if name == stack[pos]:
                    del stack[pos]
                    enabled_header_modif.append(name)
                    break
        # handle header compression/suppression modules
        if self._vbox_header_modif is not None:
            for module in self._header_modif_plugins:
                button = None
                # all hosts should have the same header modification plugins
                # activated so check if there is already one and use it
                for child in self._vbox_header_modif.get_children():
                    if child.get_name() == module.get_name():
                        button = child
                        break
                if button is None:
                    button = gtk.CheckButton(label=module.get_name().upper())
                    button.connect('clicked', self._modif_cb)
                    button.set_name(module.get_name())
                    button.set_active(False)
                    self._vbox_header_modif.pack_start(button)
                if module.get_name() in enabled_header_modif:
                    button.set_active(True)
                self._header_modif_plugins[module] = button
            self._vbox_header_modif.show_all()
            if len(self._header_modif_plugins) == 0:
                self._frame.hide()

        for pos in sorted(stack.keys()):
            if not self.add_layer(upper_val, idx_stack, stack[pos]):
                # add an empty layer
                self.reset()
                self.add_layer(upper_val, idx_stack)
                self._vbox_stack.show_all()
                raise ConfException("cannot find the %s "
                                    "plugin or it can not encapsulate "
                                    "%s packets" % (stack[pos], upper_val))
            upper_val = stack[pos]
            idx_stack += 1
        # add an empty layer
        self.add_layer(upper_val, idx_stack)
        self._vbox_stack.show_all()

    def update(self, modif_pos=0):
        """ update the protocol stack """
        upper_val = ""
        erase = False
        current_val = ''
        last = 0
        previous = 0
        for pos in sorted(self._stack.keys()):
            if pos < modif_pos:
                current_val = get_combo_val(self._stack[pos])
                upper_val = current_val
                last = pos
                previous = last
                continue
            # the previous stack was empty or removed
            if erase:
                self._vbox_stack.remove(self._stack[pos])
                del self._stack[pos]
                continue
            # get the stack value
            current_val = get_combo_val(self._stack[pos])
            if current_val == '':
                last = previous
                erase = True
                self._vbox_stack.remove(self._stack[pos])
                del self._stack[pos]
                continue
            previous = pos
            # get the selected value
            module = self._modules[current_val]
            # reload the layer, if the selected value is not compatible with the
            # upper_val layer, use the default value
            self._vbox_stack.remove(self._stack[pos])
            last = pos
            del self._stack[pos]
            if upper_val not in module.get_available_upper_protocols(self._payload)\
               and not (upper_val == "" or module.handle_upper_bloc()):
                current_val = ''
                erase = True
            self.add_layer(upper_val, pos, current_val)
            upper_val = current_val

        # add a layer if the last one is not empty
        if len(self._stack) == 0:
            self.add_layer(upper_val, 0)
        elif last in self._stack and get_combo_val(self._stack[last]) != '':
            self.add_layer(upper_val, last + 1)
        self._vbox_stack.show_all()

    def reset(self):
        """ reset the vbox """
        for child in self._vbox_stack.get_children():
            self._vbox_stack.remove(child)
        self._stack.clear()

    def on_stack_changed(self, source=None):
        """ 'changed' event on an encap Combobox """
        if source not in self._vbox_stack.get_children():
            return
        pos = self._vbox_stack.child_get_property(source, "position")
        self.update(pos)

    def add_layer(self, upper_val, idx_stack, active=''):
        """ add a new layer in the protocol stack """
        removed = False
        combo = gtk.ComboBox()
        combo.connect('changed', self.on_stack_changed)
        if self._modif_cb is not None:
            combo.connect('changed', self._modif_cb)
        store = gtk.ListStore(gobject.TYPE_STRING)
        idx = 0
        active_idx = 0
        # add an empty value that will be used to unselect the layer
        empty = store.append([''])
        # add each encapsulation  protocol that can encapsulate the upper_val
        # protocol
        for name in self.get_lower_list(upper_val):
            # if the module cannot be encapsulated and does not support the emission
            # standard do not keep it
            lower = self.get_lower_list(name)
            # check if condition is explicitely False because it can be None
            if self._modules[name].get_condition(self._emission_std) == False:
                if len(lower) == 0:
                    continue
            idx += 1
            store.append([name])
            # keep the index of the active protocol
            if active == name:
                active_idx = idx
        # if the module need a lower encapsulation protocol and there is
        # only one remove the empty value
        if upper_val != '' and \
           self._modules[upper_val].get_condition('mandatory_down') and \
           idx == 1:
            removed = True
            store.remove(empty)
            active_idx = 0
        if upper_val != '' and \
           self._modules[upper_val].get_condition(self._emission_std) == False and \
           idx == 1 and not removed:
            removed = True
            store.remove(empty)
            active_idx = 0
        if idx == 0:
            return True
        combo.set_model(store)
        cell = gtk.CellRendererText()
        combo.pack_start(cell, True)
        combo.add_attribute(cell, 'text', 0)
        # update the stack
        self._stack[idx_stack] = combo
        combo.set_active(active_idx)
        self._vbox_stack.pack_start(combo)
        self._vbox_stack.reorder_child(combo, idx_stack)
        self._vbox_stack.set_child_packing(combo, expand=False, fill=False,
                                     padding=5, pack_type=gtk.PACK_START)
        if active != '' and active_idx == 0 and not removed:
            return False
        return True

    def get_lower_list(self, module_name):
        """ get the list of protocols that can encapsulate the current one """
        lower = []
        for name in self._modules.keys():
            module = self._modules[name]
            if module_name in module.get_available_upper_protocols(self._payload) \
               or (module_name == "" and module.handle_upper_bloc()):
                lower.append(name)
        return lower


    def set_payload_type(self, payload):
        """ set the satellite payload type """
        self._payload = payload.lower()
        self.update()

    def get_stack(self):
        """ get the protocol stack """
        stack = {}
        previous = "none"
        pos = 0
        enabled_header_modif = []
        for module in [mod for mod in self._header_modif_plugins
                       if self._header_modif_plugins[mod] is not None and \
                       self._header_modif_plugins[mod].get_active()]:
            enabled_header_modif.append(module)
        enabled_header_modif = self.rearange_header_modif(enabled_header_modif)
        for key in self._stack.keys():
            for module in enabled_header_modif:
                if previous in module.get_available_upper_protocols():
                    module_name = self._header_modif_plugins[module].get_name()
                    stack[pos] = module_name
                    previous = module_name
                    pos += 1
                    enabled_header_modif.remove(module)
                    break
            name = get_combo_val(self._stack[key])
            if name != '':
                stack[pos] = name
                previous = name
                pos += 1
        for module in enabled_header_modif:
            if previous in module.get_available_upper_protocols():
                module_name = self._header_modif_plugins[module].get_name()
                stack[pos] = module_name
                previous = module_name
                pos += 1

        return stack

    def rearange_header_modif(self, plugins):
        """ set header modif plugins order to build the stack correctly """
        new_list = list(plugins)
        for pos in range(len(plugins)):
            module = plugins[pos]
            pos += 1
            temp_list = []
            for mod in new_list:
                name = self._header_modif_plugins[mod].get_name()
                if name in module.get_available_upper_protocols():
                    temp_list.insert(0, mod)
                else:
                    temp_list.append(mod)
            new_list = list(temp_list)
        plugins = new_list
        return plugins

    def get_box(self):
        """ get the vbox containing the stack """
        return self._vbox_stack

# TODO move somewhere to be able to use it with other ComboBox!
def get_combo_val(combo):
    """ get the active value of a combobox """
    model = combo.get_model()
    active = combo.get_active_iter()
    if active:
        return model.get_value(active, 0)

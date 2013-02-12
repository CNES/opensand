#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 CNES
# Copyright © 2012 CNES
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
    def __init__(self, vbox, modules, modif_callback):
        self._vbox = vbox
        # do a copy to avoid deleting elements from model
        self._modules = dict(modules)
        # remove IP options from stack
        ip_option = []
        for module in self._modules:
            if self._modules[module].get_condition('ip_option'):
                ip_option.append(module)
        for activ in ip_option:
            del self._modules[activ]
        # the list of selected encapsulation protocols as in XML configuration
        # file {pos:name}
        self._displayed_stack = {}
        self._payload = ''
        self._emission_std = ''
        self._stack = {}
        self._modif_cb = modif_callback

    def load(self, stack, payload, emission_std=''):
        """ load or reload the protocol stack from configuration """
        self._payload = payload.lower()
        self.reset()
        upper_val = "IP"
        idx_stack = 0
        for pos in sorted(stack.keys()):
            if not self.add_layer(upper_val, idx_stack, stack[pos]):
                # add an empty layer
                self.reset()
                self.add_layer(upper_val, idx_stack)
                self._vbox.show_all()
                raise ConfException("cannot find the %s encapsulation "
                                    "plugins or it can not encapsulate "
                                    "%s packets" % (stack[pos], upper_val))
            upper_val = stack[pos]
            idx_stack += 1
        # add an empty layer
        self.add_layer(upper_val, idx_stack)
        self._vbox.show_all()

    def update(self, modif_pos=0):
        """ update the protocol stack """
        upper_val = "IP"
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
                self._vbox.remove(self._stack[pos])
                del self._stack[pos]
                continue
            # get the stack value
            current_val = get_combo_val(self._stack[pos])
            if current_val == '':
                last = previous
                erase = True
                self._vbox.remove(self._stack[pos])
                del self._stack[pos]
                continue
            previous = pos
            # get the selected value
            module = self._modules[current_val]
            # reload the layer, if the selected value is not compatible with the
            # upper_val layer, use the default value
            self._vbox.remove(self._stack[pos])
            last = pos
            del self._stack[pos]
            if upper_val not in module.get_available_upper_protocols(self._payload):
                current_val = ''
                erase = True
            self.add_layer(upper_val, pos, current_val)
            upper_val = current_val

        # add a layer if the last one is not empty
        if len(self._stack) == 0:
            self.add_layer(upper_val, 0)
        elif last in self._stack and get_combo_val(self._stack[last]) != '':
            self.add_layer(upper_val, last + 1)
        self._vbox.show_all()

    def reset(self):
        """ reset the vbox """
        for child in self._vbox.get_children():
            self._vbox.remove(child)
        self._stack.clear()

    def on_stack_changed(self, source=None):
        """ 'changed' event on an encap Combobox """
        if source not in self._vbox.get_children():
            return
        pos = self._vbox.child_get_property(source, "position")
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
            if not self._modules[name].get_condition(self._emission_std):
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
           not self._modules[upper_val].get_condition(self._emission_std) and \
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
        self._vbox.pack_start(combo)
        self._vbox.reorder_child(combo, idx_stack)
        self._vbox.set_child_packing(combo, expand=False, fill=False,
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
        for pos in self._stack:
            encap_name = get_combo_val(self._stack[pos])
            if encap_name != '':
                stack[str(pos)] = encap_name
        return stack

# TODO move somewhere to be able to use it with other ComboBox!
def get_combo_val(combo):
    """ get the active value of a combobox """
    model = combo.get_model()
    active = combo.get_active_iter()
    if active:
        return model.get_value(active, 0)

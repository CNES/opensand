#!/usr/bin/env python 
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
protocol_stack.py - The protocol stack view
"""

import gtk
import gobject

from platine_manager_core.my_exceptions import ConfException

class ProtocolStack():
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
        self._stack = {}
        self._modif_cb = modif_callback
       
    def load(self, stack, payload):
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
        combo = gtk.ComboBox()
        combo.connect('changed', self.on_stack_changed)
        if self._modif_cb is not None:
            combo.connect('changed', self._modif_cb)
        store = gtk.ListStore(gobject.TYPE_STRING)
        idx = 0
        active_idx = 0
        # add an empty value that will be used to unselect the layer
        store.append([''])
        # add each encapsulation  protocol that can encapsulate the upper_val
        # protocol
        for name in self._modules.keys():
            module = self._modules[name]
            if upper_val in module.get_available_upper_protocols(self._payload):
                idx += 1
                store.append([name])
            # keep the index of the active protocol
            if active == name:
                active_idx = idx
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
        if active != '' and active_idx == 0:
            return False
        return True

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
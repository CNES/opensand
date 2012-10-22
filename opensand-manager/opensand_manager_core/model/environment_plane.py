# -*- coding: utf8 -*-

"""
Environment Plane model.
"""

import struct


class EventLevel(object):
    DEBUG = 0
    INFO = 1
    WARNING = 2
    ERROR = 3

class Program(object):
    """
    Represents a running program.
    """

    def __init__(self, controller, ident, name, probes, events):
        self.ident = ident
        self.name = name
        self._probes = []
        for i, (p_name, unit, storage_type, enabled, disp) in enumerate(probes):
            probe = Probe(controller, self, i, p_name, unit, storage_type,
                enabled, disp)
            self._probes.append(probe)
        self._events = events
    
    def get_probes(self):
        """
        Returns a list of the probe objects associated with this program
        """
        
        return self._probes
    
    def get_probe(self, ident):
        """
        Returns the probe identified by ident
        """
        
        return self._probes[ident]
    
    def get_event(self, ident):
        """
        Returns the event identified by ident as a (name, level) tuple
        """
        
        return self._events[ident]
    
    def __str__(self):
        return self.name
    
    def __repr__(self):
        return "<Program: %s [%d]>" % (self.name, self.ident)


class Probe(object):
    """
    Represents a probe
    """
    
    def __init__(self, controller, program, ident, name, unit, storage_type,
        enabled, displayed):
        self._controller = controller
        self.program = program
        self.ident = ident
        self.name = name
        self.unit = unit
        self._storage_type = storage_type
        self._enabled = enabled
        self._displayed = displayed
    
    def _read_value(self, data, pos):
        """
        Read a probe value from the specified data string at the specified
        position. Returns the new position.
        """

        if self._storage_type == 0:
            value = struct.unpack("!i", data[pos:pos + 4])[0]
            return value, pos + 4

        if self._storage_type == 1:
            value = struct.unpack("!f", data[pos:pos + 4])[0]
            return value, pos + 4

        if self._storage_type == 2:
            value = struct.unpack("!d", data[pos:pos + 8])[0]
            return value, pos + 8

        raise Exception("Unknown storage type")
    
    @property
    def enabled(self):
        return self._enabled
    
    @enabled.setter
    def enabled(self, value):
        value = bool(value)
        
        if value == self._enabled:
            return
        
        if not value:
            self._displayed = False
        
        self._enabled = value
        self._controller._update_probe_status(self)

    @property
    def displayed(self):
        return self._displayed
    
    @displayed.setter
    def displayed(self, value):
        value = bool(value)
        
        if value == self._displayed:
            return
        
        if value and not self._enabled:
            raise ValueError("Cannot display a disabled probe")
        
        self._displayed = value
        self._controller._update_probe_status(self)
    
    @property
    def global_ident(self):
        return self.ident | (self.program.ident << 8)
    
    def __str__(self):
        return self.name
    
    def __repr__(self):
        return "<Probe: %s [%d]>" % (self.name, self.ident)
    


# -*- coding: utf8 -*-

"""
This module manages the different hosts, programs, and probes which are
connected to the collector.
"""

import logging
import struct

LOGGER = logging.getLogger("probes_manager")


class Probe(object):
    def __init__(self, name, storage_type, enabled):
        self.name = name
        self.storage_type = storage_type
        self.enabled = enabled
        self.displayed = False

    def read_value(self, data, pos):
        if self.storage_type == 0:
            value = struct.unpack("!i", data[pos:pos + 4])[0]
            return value, pos + 4

        if self.storage_type == 1:
            value = struct.unpack("!f", data[pos:pos + 4])[0]
            return value, pos + 4

        if self.storage_type == 2:
            value = struct.unpack("!d", data[pos:pos + 8])[0]
            return value, pos + 8

        raise Exception("Unknown storage type")

    def __str__(self):
        return self.name

    def __repr__(self):
        return "<Probe %s: storage = %d, enabled = %s>" % (self,
            self.storage_type, self.enabled)


class Event(object):
    def __init__(self, ident, level):
        self.ident = ident
        self.level = level

    def __str__(self):
        return self.ident

    def __repr__(self):
        return "<Event %s>" % self


class Program(object):
    def __init__(self, name, probes, events):
        self.name = name
        self.probes = probes
        self.events = events

    def get_probe(self, probe_id):
        return self.probes[probe_id]

    def get_event(self, event_id):
        return self.events[event_id]

    def __str__(self):
        return self.name

    def __repr__(self):
        return "<Program %s: %r, %r>" % (self.name, self.probes, self.events)


class Host(object):
    """
    Represents an host running a collector.
    """

    def __init__(self, ident, name, address):
        self.ident = ident
        self.name = name
        self.address = address
        self._programs = {}

    def add_program(self, ident, name, probe_list, event_list):
        if ident in self._programs:
            LOGGER.error("Tried to add program with ID %d already on host %s",
                ident, self)

        probes = [Probe(*args) for args in probe_list]
        events = [Event(*args) for args in event_list]
        self._programs[ident] = Program(name, probes, events)
        LOGGER.info("Program %s was added to host %s. Probes: %r, events: %r",
            name, self, probes, events)

    def remove_program(self, ident):
        if ident not in self._programs:
            LOGGER.error("Tried to remove program with ID %d not on host %s",
                ident, self)
            return

        name = self._programs[ident].name
        del self._programs[ident]
        LOGGER.info("Program %s was removed from host %s", name, self)

    def get_program(self, ident):
        return self._programs[ident]

    def __str__(self):
        return self.name

    def __repr__(self):
        return "<Host %r: %r>" % (self.name, self._programs)


class HostManager(object):
    """
    Manages the hosts identified by Avahi messaging.
    """

    def __init__(self):
        self._host_by_name = {}
        self._host_by_addr = {}
        self._used_idents = set()

    def add_host(self, name, address):
        """
        Adds an host with the specified address (IP, port tuple)
        """

        if name in self._host_by_name:
            LOGGER.error("Host name %s is already registered, ignoring.", name)
            return

        if address in self._host_by_addr:
            LOGGER.error("Host address %s is already registered, ignoring.",
                address)
            return

        ident = self._new_ident()

        if ident == 255:
            LOGGER.error("Unable to add host %s: no more available IDs", name)
            return

        host = Host(ident, name, address)
        self._host_by_name[name] = host
        self._host_by_addr[address] = host

        LOGGER.info("Host %s (%s:%d) registered.", name, *address)

    def remove_host(self, name):
        """
        Removes a host identified by an address.
        """

        host = self._host_by_name.pop(name)
        del self._host_by_addr[host.address]
        self._used_idents.remove(host.ident)

    def get_host(self, address):
        """
        Returns the host corresponding to a given address
        """

        return self._host_by_addr[address]

    def _new_ident(self):
        """
        Returns an unused identifier which will be used for communication
        with the manager.
        """

        for i in xrange(0, 255):
            if i not in self._used_idents:
                self._used_idents.add(i)
                return i

        return 255

    def __repr__(self):
        return "<HostManager: %r>" % self._host_by_name.values()


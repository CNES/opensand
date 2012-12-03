#!/usr/bin/env python2
# -*- coding: utf8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 TAS
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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>

"""
manager.py  - This module manages the different hosts, programs,
              and probes which are connected to the collector.
"""

from os import mkdir
from os.path import join, isdir
import logging
import shutil
import struct
import tempfile

LOGGER = logging.getLogger("manager")


class Probe(object):
    """
    Class representing a probe
    """

    def __init__(self, program, name, unit, storage_type, enabled):
        self._program = program
        self._name = name
        self._unit = unit
        self._storage_type = storage_type
        self._enabled = enabled
        self._displayed = False
        self._log_file = None
        self._create_file()

    def read_value(self, data, pos):
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

    def encode_value(self, value):
        """
        Encodes a probe value to a string (which may be decoded later with
        read_value)
        """
        if self._storage_type == 0:
            return struct.pack("!i", value)

        if self._storage_type == 1:
            return struct.pack("!f", value)

        if self._storage_type == 2:
            return struct.pack("!d", value)

        raise Exception("Unknown storage type")

    def switch_storage(self):
        """
        Handle the switch of storage folder.
        """
        self.cleanup()  # Close the opened log file
        self._create_file()

    def cleanup(self):
        """
        Close the log file (should be used when the probe object is to be
        released)
        """
        if self._log_file:
            self._log_file.close()
        self._log_file = None

    def restore(self):
        """
        Restore previously cleaned resources
        """
        self._create_file(mode='a')

    def log_value(self, time, value):
        """
        Writes the specified value to the log.
        """
        self._log_file.write("%d %s\n" % (time, value))

    def attributes(self):
        """
        Returns the probe’s properties as a tuple.
        """
        return (self._name, self._unit, self._storage_type, self._enabled,
                self._displayed)

    def _create_file(self, mode='w'):
        """
        Create the probe log file in the program’s storage folder.
        """
        path = self._program.get_storage_path(self._name + ".log")
        try:
            self._log_file = open(path, mode)
        except (IOError, OSError), err:
            LOGGER.error("cannot open probe log file: %s", err)
        if mode != 'a':
            LOGGER.debug("Creating probe log file %s", path)
            self._log_file.write("%s\n" % self._unit)
        else:
            LOGGER.debug("Opening probe log file %s", path)

    @property
    def enabled(self):
        """
        Check whether the probe is enabled
        """
        return self._enabled

    @enabled.setter
    def enabled(self, value):
        """
        Set the enabled value
        """
        self._enabled = value

    @property
    def displayed(self):
        """
        Check whether the probe is displayed
        """
        return self._displayed

    @displayed.setter
    def displayed(self, value):
        """
        Set the displayed value
        """
        self._displayed = value

    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Probe %s: storage = %d, enabled = %s>" % (self,
                                                           self._storage_type,
                                                           self._enabled)


class Event(object):
    """
    Class representing an event
    """

    def __init__(self, program, ident, level):
        self._program = program
        self._ident = ident
        self._level = level
        self._log_file = None

    def log(self, time, text):
        """
        Writes the specified value to the log.
        """
        self._program.write_event("%.6f %s %s\n" % (time, self._ident, text))

    def attributes(self):
        """
        Returns the event’s properties as a tuple.
        """
        return (self._ident, self._level)

    def __str__(self):
        return self._ident

    def __repr__(self):
        return "<Event %s>" % self


class Program(object):
    """
    Class representing a program
    """

    def __init__(self, host, ident, name, probe_list, event_list):
        self._host = host
        self._ident = ident
        self._name = name

        self._event_log_file = None
        self._setup_storage()

        self._probes = [Probe(self, *args) for args in probe_list]
        self._events = [Event(self, *args) for args in event_list]

    def get_probe(self, probe_id):
        """
        Get the specified probe
        """
        return self._probes[probe_id]

    def get_event(self, event_id):
        """
        Get the specified event
        """
        return self._events[event_id]

    def switch_storage(self):
        """
        Handle the switch of storage folder.
        """
        if self._event_log_file:
            self._event_log_file.close()

        self._setup_storage()

        for probe in self._probes:
            probe.switch_storage()

    def cleanup(self):
        """
        Ensure proper resource release (close files, ...) before deleting
        this program object.
        """
        if self._event_log_file:
            self._event_log_file.close()

        for probe in self._probes:
            probe.cleanup()

    def restore(self):
        """
        Restore previously cleaned resources
        """
        self._setup_storage(mode='a')

        for probe in self._probes:
            probe.restore()

    def get_storage_path(self, name):
        """
        Return the full path for a filename stored in the program’s storage
        folder.
        """
        return join(self._host.get_storage_path(self._name), name)

    def attributes(self):
        """
        Returns a tuple containing the full program name (host.program), the
        host and program identifiers, a list of probe attributes, and a list
        of event attributes.
        """
        return ("%s.%s" % (self._host.name, self._name),
                self._host.ident, self._ident,
                [probe.attributes() for probe in self._probes],
                [event.attributes() for event in self._events])

    def write_event(self, text):
        """
        Write an event to the log.
        """
        self._event_log_file.write(text)

    def _setup_storage(self, mode='w'):
        """
        Creates the storage folder for the program, and the events log file.
        """
        path = self._host.get_storage_path(self._name)
        if not isdir(path):
            LOGGER.debug("Creating program folder %s", path)
            mkdir(path)

        path = join(path, "event_log.txt")
        if mode != 'a':
            LOGGER.debug("Creating event log file %s", path)
        else:
            LOGGER.debug("Opening event log file %s", path)
        try:
            self._event_log_file = open(path, mode)
        except (IOError, OSError), err:
            LOGGER.error("cannot open event log file: %s", err)

    @property
    def name(self):
        """
        Get the program name
        """
        return self._name

    @property
    def ident(self):
        """
        Get the program ident
        """
        return self._ident

    @property
    def probes(self):
        """
        Get the probes
        """
        return self._probes

    @property
    def events(self):
        """
        Get the events
        """
        return self._events

    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Program %s: %r, %r>" % (self._name, self._probes, self._events)


class Host(object):
    """
    Represents an host running a daemon.
    """

    def __init__(self, manager, ident, name, address):
        self._manager = manager
        self._ident = ident
        self._name = name
        self._address = address
        self._ref_count = 1
        self._programs = {}
        self._create_host_folder()

    def add_program(self, ident, name, probe_list, event_list):
        """
        Add a program with a specified probe/event list to the list of
        programs running on the host.
        """
        if ident in self._programs:
            LOGGER.error("Tried to add program with ID %d already on host %s",
                         ident, self)

        prog = Program(self, ident, name, probe_list, event_list)
        self._programs[ident] = prog
        LOGGER.info("Program %s was added to host %s. Probes: %r, events: %r",
                    name, self, prog.probes, prog.events)

        return prog

    def remove_program(self, ident):
        """
        Remove the specified program from the host.
        """
        if ident not in self._programs:
            LOGGER.error("Tried to remove program with ID %d not on host %s",
                         ident, self)
            return

        name = self._programs[ident].name
        self._programs[ident].cleanup()
        del self._programs[ident]
        LOGGER.info("Program %s was removed from host %s", name, self)

    def switch_storage(self):
        """
        Handle the switch of storage folder.
        """
        self._create_host_folder()

        for program in self._programs.itervalues():
            program.switch_storage()

    def cleanup(self):
        """
        Ensure proper resource release (close files, ...) before deleting
        this host.
        """
        for program in self._programs.itervalues():
            program.cleanup()

    def restore(self):
        """
        Restore previously cleaned resources
        """
        for program in self._programs.itervalues():
            program.restore()

    def get_storage_path(self, name):
        """
        Returns the full path for a filename stored in the host’s storage
        folder.
        """
        return join(self._manager.get_storage_path(self._name), name)

    def get_program(self, ident):
        """
        Get the specified program on this host.
        """
        return self._programs[ident]

    def all_programs(self):
        """
        Return an iterator of all programs on this host
        """
        return self._programs.itervalues()

    @property
    def ref_count(self):
        """
        Get ref count value
        """
        return self._ref_count

    @ref_count.setter
    def ref_count(self, value):
        """
        set ref count value
        """
        self._ref_count = int(value)

    def _create_host_folder(self):
        """
        Create the storage folder for the host.
        """
        path = self._manager.get_storage_path(self._name)
        if not isdir(path):
            LOGGER.debug("Creating host folder %s", path)
            mkdir(path)

    @property
    def name(self):
        """
        Get the host name
        """
        return self._name

    @property
    def ident(self):
        """
        Get the host ident
        """
        return self._ident

    @property
    def address(self):
        """
        Get the host address
        """
        return self._address

    @address.setter
    def address(self, value):
        """
        Set host address
        """
        self._address = value

    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Host %r: %r>" % (self._name, self._programs)


class HostManager(object):
    """
    Manages the hosts identified by Avahi messaging.
    """

    def __init__(self):
        self._host_by_name = {}
        self._host_by_addr = {}
        self._host_by_id = {}
        self._removed_hosts = []
        self._storage_folder = tempfile.mkdtemp("_opensand_collector")
        LOGGER.debug("Initialized storage folder at %s", self._storage_folder)

    def get_storage_path(self, name):
        """
        Returns the full path for a filename stored in the storage folder.
        """
        return join(self._storage_folder, name)

    def switch_storage(self):
        """
        Creates a new storage folder, asks the components to switch to this
        folder, and returns the initial folder path.
        """
        # clean remove hosts as the simulation was stopped
        self._removed_hosts = []

        previous_folder = self._storage_folder
        self._storage_folder = tempfile.mkdtemp("opensand_collector")
        LOGGER.debug("Initialized new storage folder at %s",
            self._storage_folder)

        for host in self._host_by_name.itervalues():
            host.switch_storage()

        return previous_folder

    def add_host(self, name, address):
        """
        Adds a host with the specified address (IP, port tuple)
        """
        if name in self._host_by_name:
            LOGGER.error("Host name %s is already registered, ignoring.", name)
            return

        if address in self._host_by_addr:
            LOGGER.error("Host address %s is already registered, ignoring.",
                         address)
            return

        found = False
        for host in self._removed_hosts:
            if name == host.name:
                new_host = host
                new_host.restore()
                # update address
                host.address = address
                ident = host.ident
                found = True
                self._removed_hosts.remove(host)
                break

        if not found:
            ident = self._new_ident()

            if ident == 255:
                LOGGER.error("Unable to add host %s: no more available IDs", name)
                return

            new_host = Host(self, ident, name, address)

        self._host_by_name[name] = new_host
        self._host_by_addr[address] = new_host
        self._host_by_id[ident] = new_host

        LOGGER.info("Host %s (%s:%d) registered.", name, *address)

    def add_host_addr(self, name, addr):
        """
        Registers an extra IP address for an already registered host.
        """
        host = self._host_by_name.get(name)

        if not host:
            LOGGER.error("Host name %s is not registered, ignoring.", name)
            return

        host.ref_count += 1
        self._host_by_addr[addr] = host

        LOGGER.info("Host %s has an extra address %s:%d.", name, *addr)

    def remove_host(self, name):
        """
        Removes a host identified by an address.
        """
        host = self._host_by_name.get(name)
        if not host:
            LOGGER.error("Host name %s is not registered, ignoring.", name)
            return

        host.ref_count -= 1

        if host.ref_count > 0:
            LOGGER.info("Host %s is unregistering.", name)
            return

        self._removed_hosts.append(host)
        del self._host_by_name[name]
        del self._host_by_addr[host.address]
        del self._host_by_id[host.ident]
        host.cleanup()
        LOGGER.info("Host %s is unregistered.", name)

    def get_host(self, address):
        """
        Returns the host corresponding to a given address
        """
        return self._host_by_addr[address]

    def set_probe_status(self, host_id, program_id, probe_id, new_enabled,
                         new_displayed):
        """
        Sets the status of a given probe.
        Returns the host object corresponding to the ID if an update to the
        enabled state of the probe is needed.
        """
        try:
            host = self._host_by_id[host_id]
        except KeyError:
            LOGGER.error("Host with ID %d not found in set_probe_status",
                         host_id)
            return None

        try:
            program = host.get_program(program_id)
        except KeyError:
            LOGGER.error("Program with ID %d not found in set_probe_status",
                         program_id)
            return None

        try:
            probe = program.get_probe(probe_id)
        except IndexError:
            LOGGER.error("Probe with id %d not found in set_probe_status",
                         probe_id)
            return None

        enabled_changed = (new_enabled != probe.enabled)

        probe.enabled = new_enabled
        probe.displayed = new_displayed

        if enabled_changed:
            return host

        return None

    def cleanup(self):
        """
        Removes the storage folder.
        """
        shutil.rmtree(self._storage_folder)

    def all_programs(self):
        """
        Return an iterator of all programs on all hosts
        """
        return (p for h in self._host_by_name.itervalues()
                for p in h.all_programs())

    def _new_ident(self):
        """
        Returns an unused identifier which will be used for communication
        with the manager.
        """
        for i in xrange(0, 255):
            if i not in self._host_by_id and \
               i not in map(lambda x: x.ident, self._removed_hosts):
                return i

        return 255

    def __repr__(self):
        return "<HostManager: %r>" % self._host_by_name.values()


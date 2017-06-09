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

LOGGER = logging.getLogger('sand-collector')


class Probe(object):
    """
    Class representing a probe
    """

    def __init__(self, program, probe_id, name, unit, storage_type, enabled):
        self._program = program
        self._id = probe_id
        self._name = name.strip()
        self._unit = unit.strip()
        self._storage_type = storage_type
        self._enabled = enabled
        self._displayed = False
        self._save_file = None
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
        self.cleanup()  # Close the opened file
        self._create_file()

    def cleanup(self):
        """
        Close the file (should be used when the probe object is to be
        released)
        """
        if self._save_file:
            self._save_file.close()
        self._save_file = None

    def restore(self):
        """
        Restore previously cleaned resources
        """
        self._create_file(mode='a')

    def save_value(self, time, value):
        """
        Writes the specified value to the file.
        """
        self._save_file.write("%d %s\n" % (time, value))

    def attributes(self):
        """
        Returns the probe’s properties as a tuple.
        """
        return (self._id, self._name, self._unit, self._storage_type,
                self._enabled, self._displayed)

    def _create_file(self, mode='w'):
        """
        Create the probe file in the program’s storage folder.
        """
        path = self._program.get_storage_path(self._name + ".csv")
        try:
            self._save_file = open(path, mode)
        except (IOError, OSError), err:
            LOGGER.error("cannot open probe file: %s", err)
        if mode != 'a':
            LOGGER.debug("Creating probe file %s", path)
            self._save_file.write("%s\n" % self._unit)
        else:
            LOGGER.debug("Opening probe file %s", path)

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
        return "<Probe %s (%s): storage = %d, enabled = %s>" % (self, self._id,
                                                            self._storage_type,
                                                            self._enabled)


class Log(object):
    """
    Class representing an log
    """

    def __init__(self, program, log_id, ident, display_level):
        self._program = program
        self._id = log_id
        self._ident = ident.strip()
        self._display_level = int(display_level)
        self._save_file = None

    def save(self, time, level, text):
        """
        Writes the specified value to the file.
        """
        # TODO LEVEL !!!
        self._program.write_log("%.6f %s %s\n" % (time, self._ident, text))

    @property
    def display_level(self):
        """
        Get the log display level
        """
        return self._display_level

    @display_level.setter
    def display_level(self, value):
        """
        Set the display level value
        """
        self._display_level = value

    def attributes(self):
        """
        Returns the log’s properties as a tuple.
        """
        return (self._id, self._ident, self._display_level)

    def __str__(self):
        return self._ident

    def __repr__(self):
        return "<Log %s (%s): display level = %d>" % (self, self._id,
                                                      self._display_level)

class Program(object):
    """
    Class representing a program
    """

    def __init__(self, host, ident, name, probe_list, log_list):
        self._host = host
        self._ident = ident
        self._name = name

        self._log_file = None
        self._setup_storage()
        self._probes = {}
        self._logs = {}
        self._initialized = False

        self._reg_probes = []
        self._reg_logs = []

        self.add_probe(probe_list)
        self.add_log(log_list)

    def get_probe(self, probe_id):
        """
        Get the specified probe
        """
        return self._probes[probe_id]

    def get_log(self, log_id):
        """
        Get the specified log
        """
        return self._logs[log_id]

    def switch_storage(self):
        """
        Handle the switch of storage folder.
        """
        if self._log_file:
            self._log_file.close()

        self._setup_storage()

        for probe in self._probes.values():
            probe.switch_storage()

    def cleanup(self):
        """
        Ensure proper resource release (close files, ...) before deleting
        this program object.
        """
        if self._log_file:
            self._log_file.close()

        for probe in self._probes.values():
            probe.cleanup()

    def restore(self):
        """
        Restore previously cleaned resources
        """
        self._setup_storage(mode='a')

        for probe in self._probes.values():
            probe.restore()

    def get_storage_path(self, name):
        """
        Return the full path for a filename stored in the program’s storage
        folder.
        """
        name = name.replace("/", "_")
        name = name.replace(" ", "_")
        return join(self._host.get_storage_path(self._name), name)

    def attributes(self):
        """
        Returns a tuple containing the full program name (host.program), the
        host and program identifiers, a list of probe attributes, and a list
        of log attributes.
        """
        return ("%s.%s" % (self._host.name, self._name),
                self._host.ident, self._ident,
                [probe.attributes() for probe in self._probes.values()],
                [log.attributes() for log in self._logs.values()])

    def unreg_att(self):
        """
        return the same as attributes with only unregisterd logs and probes
        """
        probes = []
        for probe_id in self._probes:
            if not probe_id in self._reg_probes:
                probes.append(self._probes[probe_id])
        logs = []
        for log_id in self._logs:
            if not log_id in self._reg_logs:
                logs.append(self._logs[log_id])
        return ("%s.%s" % (self._host.name, self._name),
                self._host.ident, self._ident,
                [probe.attributes() for probe in probes],
                [log.attributes() for log in logs])

    def set_registered(self, probes=[], logs=[]):
        """ set probes and logs as registered """
        self._reg_probes += (probes)
        self._reg_logs += (logs)

    def reset_registered(self):
        """ clear the registered probes and logs """
        self._reg_probes = []
        self._reg_logs = []

    def write_log(self, text):
        """
        Write an log to the file.
        """
        self._log_file.write(text)

    def add_probe(self, probe_list):
        """
        Add a new probe in the list
        """
        for (probe_id, p_name, unit, storage_type, enabled) in probe_list:
            if not probe_id in self._probes:
                LOGGER.info("Add probe %s with id %s in %s" % (p_name, probe_id,
                                                               self._name))
                probe = Probe(self, probe_id, p_name, unit, storage_type, enabled)
                self._probes[probe_id] = probe


    def add_log(self, log_list):
        """
        Add a new log in the list
        """
        for (log_id, name, level) in log_list:
            if not log_id in self._logs:
                LOGGER.info("Add log %s with id %s", name, log_id)
                log = Log(self, log_id, name, level)
                self._logs[log_id] = log

    def _setup_storage(self, mode='w'):
        """
        Creates the storage folder for the program, and the logs file.
        """
        path = self._host.get_storage_path(self._name)
        if not isdir(path):
            LOGGER.info("Creating program folder %s", path)
            mkdir(path)

        # TODO create a file par logging facility (error.log, warning.log, ...)
        path = join(path, "log.txt")
        if mode != 'a':
            LOGGER.debug("Creating log file %s", path)
        else:
            LOGGER.debug("Opening log file %s", path)
        try:
            self._log_file = open(path, mode)
        except (IOError, OSError), err:
            LOGGER.error("cannot open log file: %s", err)

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
        return self._probes.values()

    @property
    def logs(self):
        """
        Get the logs
        """
        return self._logs.values()

    @property
    def initialized(self):
        """ check if a program is initialized """
        return self._initialized

    @initialized.setter
    def initialized(self, val):
        """ set initialized value """
        self._initialized = val


    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Program %s: %r, %r>" % (self._name, self._probes, self._logs)


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

    def add_program(self, ident, name, probe_list, log_list):
        """
        Add a program with a specified probe/log list to the list of
        programs running on the host.
        """
        if ident in self._programs:
            # this is a new probe
            prog = self._programs[ident]
            prog.add_probe(probe_list)
            prog.add_log(log_list)
            LOGGER.info("Added %s probe(s) and %s log(s) to program %s "
                        "for host %s", len(probe_list), len(log_list),
                        name, self)
            return prog

        prog = Program(self, ident, name, probe_list, log_list)
        self._programs[ident] = prog
        LOGGER.info("Program %s was added to host %s. Probes: %r, logs: %r",
                    name, self, prog.probes, prog.logs)

        return prog

    def remove_program(self, ident):
        """
        Remove the specified program from the host.
        """
        if ident not in self._programs:
            LOGGER.error("Tried to remove program with ID %d not on host %s",
                         ident, self)
            return False

        name = self._programs[ident].name
        self._programs[ident].cleanup()
        del self._programs[ident]
        LOGGER.info("Program %s was removed from host %s", name, self)
        return True

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
            LOGGER.info("Creating host folder %s", path)
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
        LOGGER.info("Initialized storage folder at %s", self._storage_folder)

    def get_storage_path(self, name):
        """
        Returns the full path for a filename stored in the storage folder.
        """
        name = name.replace("/", "_")
        name = name.replace(" ", "_")
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
        LOGGER.info("Initialized new storage folder at %s",
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

    def get_host_address(self, host_id):
        """ get the host address according to its id """
        try:
            host = self._host_by_id[host_id]
        except KeyError:
            LOGGER.error("Host with ID %d not found in get_host_address",
                         host_id)
            return None
        return host.address

    def get_host(self, address):
        """
        Returns the host corresponding to a given address
        """
        return self._host_by_addr[address]

    def unreg_manager(self):
        """ The manager was unregistered """
        for host in self._host_by_name.itervalues():
            for program in host.all_programs():
                program.reset_registered()

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
        except KeyError:
            LOGGER.error("Probe with id %d not found in set_probe_status",
                         probe_id)
            return None

        enabled_changed = (new_enabled != probe.enabled)

        probe.enabled = new_enabled
        probe.displayed = new_displayed

        if enabled_changed:
            return host

        return None

    def set_log_display_level(self, host_id, program_id, log_id, display_level):
        """
        Sets the display level of a given log.
        Returns the host object corresponding to the ID if an update to the
        display level of log is needed.
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
            log = program.get_log(log_id)
        except KeyError:
            LOGGER.error("Log with id %d not found in set_log_display_level",
                         log_id)
            return None

        level_changed = (int(display_level) != log.display_level)

        log.display_level = display_level

        if level_changed:
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

    def is_initialized(self, host_id, program_id):
        """ check if a program is initialized """
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

        return program.initialized


    def __repr__(self):
        return "<HostManager: %r>" % self._host_by_name.values()


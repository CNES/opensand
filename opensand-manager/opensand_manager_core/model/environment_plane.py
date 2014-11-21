#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
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

# Author: Vincent DUVERT / <vduvert@toulouse.viveris.com>


"""
environment_plane.py - Model for environment plane elements
"""

from opensand_manager_core.model.host import InitStatus

import os
import struct



class Program(object):
    """
    Represents a running program.
    """

    def __init__(self, controller, ident, name, probes, logs, host_model=None):
        self._ident = ident
        host_name, prog_name = name.split(".", 1)
        self._controller = controller
        if host_name.startswith(prog_name):
            name = host_name

        self._name = name
        self._probes = {}
        self.add_probes(probes)
        self._logs = {}
        self.add_logs(logs)
        self._host_model = host_model
        self._syslog_enabled = True
        self._logs_enabled = True

    def handle_critical_log(self):
        """
        critical log received for this host, set host status
        """
        if self._host_model is None:
            return
        self._host_model.set_init_status(InitStatus.FAIL)

    def get_probes(self):
        """
        Returns a list of the probe objects associated with this program
        """
        return self._probes.values()

    def get_probe(self, ident):
        """
        Returns the probe identified by ident
        """
        return self._probes[ident]

    def get_logs(self):
        """
        Returns a list of the log objects associated with this program
        """
        return self._logs.values()

    def get_log(self, ident):
        """
        Returns the log identified by ident as a (name, level) tuple
        """
        return self._logs[ident]

    def add_probes(self, probes):
        """
        Add probes in the list
        """
        for (probe_id, p_name, unit, storage_type, enabled, disp) in probes:
#            if not probe_id in self._probes:
            probe = Probe(self._controller, self, probe_id, p_name, unit,
                          storage_type, enabled, disp)
            self._probes[probe_id] = probe

    def add_logs(self, logs):
        """
        Add logs in the list
        """
        for (log_id, name, level) in logs:
#            if not log_id in self._logs:
            log = Log(self._controller, self, log_id, name, level)
            self._logs[log_id] = log

    def enable_syslog(self, value):
        """ Enable/disable syslog """
        if self._controller is None:
            return
        self._controller.enable_syslog(value, self)
        self._syslog_enabled = value

    def enable_logs(self, value):
        """ Enable/disable logging """
        if self._controller is None:
            return
        self._controller.enable_logs(value, self)
        self._logs_enabled = value

    def syslog_enabled(self):
        """ Check if syslog is enabled or not """
        return self._syslog_enabled

    def logs_enabled(self):
        """ Check if logging is enabled or not """
        return self._logs_enabled

    def is_running(self):
        """ Check if progam is started """
        return self._host_model.get_state()

    def get_host_model(self):
        """ Get the host model """
        if self._host_model.get_name().lower().startswith(self._name.lower()):
        # only return host model for main host program
            return self._host_model
        # TODO return the associated tool if possible and handle it
        return None


    @property
    def name(self):
        """
        Get the program name
        """
        return self._name

    @property
    def host_name(self):
        """
        Get the program related host name
        """
        return self._host_model.get_name().lower()


    @property
    def ident(self):
        """
        Get the program ident
        """
        return self._ident


    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Program: %s [%d]>" % (self._name, self._ident)


class Probe(object):
    """
    Represents a probe
    """
    def __init__(self, controller, program, ident, name, unit, storage_type,
                 enabled, displayed):
        self._controller = controller
        self._program = program
        self._ident = ident
        self._name = name
        self._unit = unit
        self._storage_type = storage_type
        self._enabled = enabled
        self._displayed = displayed

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

    @property
    def ident(self):
        """
        Get the probe ident
        """
        return self._ident

    @property
    def name(self):
        """
        Get the probe name
        """
        return self._name

    @property
    def enabled(self):
        """
        Indicates if the probe is enabled
        """
        return self._enabled

    @property
    def program(self):
        """
        The program associated to the probe
        """
        return self._program

    @property
    def unit(self):
        """
        The probe unit
        """
        return self._unit

    @enabled.setter
    def enabled(self, value):
        """
        Enables or disables the probe
        """
        value = bool(value)

        if value == self._enabled:
            return

        if not value:
            self._displayed = False

        self._enabled = value
        self._controller.update_probe_status(self)

    @property
    def displayed(self):
        """
        Indicates if the probe is displayed
        """
        return self._displayed

    @displayed.setter
    def displayed(self, value):
        """
        Displays or hides the probes
        """
        value = bool(value)

        if value == self._displayed:
            return

        if value and not self._enabled:
            raise ValueError("Cannot display a disabled probe")

        self._displayed = value
        self._controller.update_probe_status(self)

    @property
    def global_ident(self):
        return self._ident | (self._program.ident << 8)

    @property
    def full_name(self):
        return "%s.%s" % (self._program.name, self._name)

    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Probe: %s [%d]>" % (self._name, self._ident)


class Log(object):
    """
    Represents a log
    """
    def __init__(self, controller, program, ident, name, level):
        self._controller = controller
        self._program = program
        self._ident = ident
        self._name = name
        self._display_level = level

    @property
    def ident(self):
        """
        Get the log ident
        """
        return self._ident

    @property
    def name(self):
        """
        Get the log name
        """
        return self._name

    @property
    def display_level(self):
        """
        Indicates the log level
        """
        return self._display_level

    @property
    def program(self):
        """
        The program associated to the log
        """
        return self._program

    @display_level.setter
    def display_level(self, value):
        """
        Set the log level
        """
        if value == self._display_level:
            return

        self._display_level = value
        self._controller.update_log_status(self)

    @property
    def global_ident(self):
        return self._ident | (self._program.ident << 8)

    @property
    def full_name(self):
        return "%s.%s" % (self._program.name, self._name)

    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Log: %s [%d]>" % (self._name, self._ident)

class SavedProbeLoader(object):
    """
    This objects reconstructs a program/probe hierarchy from a saved run
    """
    def __init__(self, run_path):
        self._data = {}
        self._programs = {}

        try:
            self._load(run_path)
        except EnvironmentError, msg:
            raise ValueError(str(msg))

    def _load(self, run_path):
        prog_id = 0

        for host_name in os.listdir(run_path):
            host_path = os.path.join(run_path, host_name)

            if not os.path.isdir(host_path):
                continue

            for prog_name in os.listdir(host_path):
                prog_path = os.path.join(host_path, prog_name)
                prog_id += 1

                if not os.path.isdir(prog_path):
                    continue

                probes = []
                if host_name.startswith(prog_name):
                    prog_full_name = host_name
                else:
                    prog_full_name = "%s.%s" % (host_name, prog_name)

                probe_id = 0
                for probe_name in os.listdir(prog_path):
                    probe_path = os.path.join(prog_path, probe_name)
                    probe_name, ext = os.path.splitext(probe_name)
                    probe_full_name = "%s.%s" % (prog_full_name, probe_name)

                    if not os.path.isfile(probe_path) or ext not in ['.csv',
                                                                     '.log']:
                        continue

                    probe_times = []
                    probe_values = []

                    with open(probe_path, 'r') as probe_file:
                        unit = probe_file.readline().strip()
                        for line in probe_file:
                            time, value = line.split(" ", 1)
                            time = int(time)
                            value = float(value)
                            probe_times.append(time)
                            probe_values.append(value)

                    self._data[probe_full_name] = (probe_times, probe_values)
                    probes.append((probe_id, probe_name, unit, None, True, False))
                    probe_id += 1

                full_prog_name = "%s.%s" % (host_name, prog_name)
                self._programs[prog_id] = Program(self, prog_id, full_prog_name,
                                                  probes, [])

    def get_programs(self):
        """
        Return the program list.
        """
        return self._programs

    def get_data(self):
        """
        Return the probe data.
        """
        return self._data

    def update_probe_status(self, probe):
        # Nothing to do
        pass


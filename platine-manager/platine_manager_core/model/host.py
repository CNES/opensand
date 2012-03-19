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
host.py - Host model for Platine manager
"""

import threading

#TODO we could modify privates methods from toto to _toto_
from platine_manager_core.model.tool import ToolModel
from platine_manager_core.my_exceptions import ModelException
from platine_manager_core.model.host_advanced import AdvancedHostModel

class HostModel:
    """ host model """
    def __init__(self, name, instance, network_config, state_port,
                 command_port, tools, scenario, manager_log):
        self._log = manager_log
        self._name = name
        self._instance = instance
        if self._name.startswith('st'):
            self._component = 'st'
        elif self._name.startswith('ws'):
            self._component = 'ws'
        else:
            self._component = self._name

        self._ifaces = network_config
        self._state_port = state_port
        self._command_port = command_port

        self._enabled = True
        self._advanced = None
        self._state = None

        if self._component != 'ws':
            try:
                self._advanced = AdvancedHostModel(self._name, self._instance,
                                                   self._ifaces, scenario)
            except ModelException, error:
                self._log.warning("%s: %s" % (self._name.upper(), error))

        self._lock = threading.Lock()

        # the tools dictionary [name: ToolModel]
        self._tools = {}
        for tool_name in tools:
            self._log.info(self._name.upper() + ": add " + tool_name)
            try:
                new_tool = ToolModel(tool_name, self._name, self._component)
                new_tool.load(scenario)
            except ModelException as error:
                self._log.warning("%s: %s" % (self._name.upper(), error))
            finally:
                self._tools[tool_name] = new_tool

    def reload_all(self, scenario):
        """ reload host to update the scenario path """
        self.reload_conf(scenario)
        self.reload_tools(scenario)


    def reload_conf(self, scenario):
        """ reload the host configuration """
        try:
            self._advanced.load(self._name, self._instance,
                                self._ifaces, scenario)
        except ModelException as error:
            self._log.warning("%s: %s" % (self._name.upper(), error))

    def reload_tools(self, scenario):
        """ update the scenario path for tools configuration """
        for tool_name in self._tools:
            try:
                self._tools[tool_name].update(scenario)
            except ModelException as error:
                self._log.warning("%s: %s" % (self._name.upper(), error))

    def get_advanced_conf(self):
        """ get the advanced configuration """
        return self._advanced

    def get_component(self):
        """ return the component type """
        return self._component

    def get_instance(self):
        """ get the component instance """
        return self._instance

    def get_name(self):
        """ get the name of a host """
        return self._name

    def get_state(self):
        """ get the host state """
        self._lock.acquire()
        state = self._state
        self._lock.release()
        return state

    def set_started(self, started_list):
        """ set the specified hosts states to True """
        self._lock.acquire()
        if started_list == None:
            self._state = None
            self._lock.release()
            return

        # set all states to False
        for key in self._tools:
            self._tools[key].set_state(False)
        self._state = False

        if len(started_list) == 0:
            self._lock.release()
            return

        # check if a program is started twice
        if len(started_list) > len(set(started_list)):
            self._log.warning(self._name + ": " \
                              "some components are started twice")
        # check that each specified program is specified in the tools list
        # or corresponds to the main program and set their status to True
        for key in started_list:
            if key in self._tools:
                self._tools[key].set_state(True)
            elif key == self._component:
                self._state = True
            else:
                self._log.warning(self._name + ": component '" +
                                  key + "' does not belong to model")
        self._lock.release()

    def get_ip_address(self):
        """ get the host IP address """
        return self._ifaces["discovered"]

    def get_state_port(self):
        """ get the state server port """
        return int(self._state_port)

    def get_command_port(self):
        """ get the command server port """
        return int(self._command_port)

    def enable(self, val):
        """ enable host """
        if self._advanced is None:
            return
        if val:
            self._advanced.enable()
        else:
            self._advanced.disable()

    def is_enabled(self):
        """ check if host is enabled """
        if self._advanced is None:
            return True
        return self._advanced.is_enabled()

    def get_tools(self):
        """ get the host tools """
        return self._tools.values()

    def get_tool(self, tool_name):
        """ get the host tools """
        if tool_name in self._tools:
            return self._tools[tool_name]
        return None

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

class HostModel:
    """ host model """
    def __init__(self, name, instance, ipaddr, state_port,
                 command_port, tools, scenario, manager_log):
        self._log = manager_log
        self._name = name
        self._instance  = instance
        if self._name.startswith('st'):
            self._component = 'st'
        elif self._name.startswith('ws'):
            self._component = 'ws'
        else:
            self._component = self._name

        self._ip_address = ipaddr
        self._state_port = state_port
        self._command_port = command_port

        self._enabled = True

        self._state = None

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

    def reload_tools(self, scenario):
        """ update the scenario path for tools configuration """
        for tool_name in self._tools.keys():
            try:
                self._tools[tool_name].update(scenario)
            except ModelException as error:
                self._log.warning("%s: %s" % (self._name.upper(), error))

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
        for key in self._tools.keys():
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
            if key in self._tools.keys():
                self._tools[key].set_state(True)
            elif key == self._component:
                self._state = True
            else:
                self._log.warning(self._name + ": component '" +
                                  key + "' does not belong to model")
        self._lock.release()

    def get_ip_address(self):
        """ get the host IP address """
        return self._ip_address

    def get_state_port(self):
        """ get the state server port """
        return int(self._state_port)

    def get_command_port(self):
        """ get the command server port """
        return int(self._command_port)

    def enable(self):
        """ enable host """
        self._enabled = True

    def disable(self):
        """ disable host """
        self._enabled = False

    def is_enabled(self):
        """ check if host is enabled """
        return self._enabled

    def get_tools(self):
        """ get the host tools """
        return self._tools.values()

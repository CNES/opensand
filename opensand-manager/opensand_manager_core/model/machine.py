#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 TAS
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
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>

"""
machine.py - Machine model for OpenSAND manager
"""

import threading

from opensand_manager_core.model.tool import ToolModel
from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_core.model.host_advanced import AdvancedHostModel
from opensand_manager_core.module import load_modules
from opensand_manager_core.utils import GW, WS, ST, SAT, GW_types

class InitStatus:
    """ status of host initialization """
    NONE = 0     # no host process info or no environment plane
    SUCCESS = 1  # host process successfully initialized
    PENDING = 2  # host process is still initializing
    FAIL = 3     # host process failed to initialize


class MachineModel:
    """ machine model """
    def __init__(self, name, instance, network_config, state_port,
                 command_port, tools, modules, scenario, manager_log,
                 collector_functional, host_model,  spot_id = "", gw_id = ""):
        self._log = manager_log
        self._name = name
        self._instance = instance
        self._gw_id = gw_id
        self._spot_id = spot_id
        self._host_model = host_model
        if self._name.startswith(ST):
            self._component = ST
        elif self._name.startswith(WS):
            self._component = WS
        elif self._name.startswith(GW):
            self._component = GW
            for gw_type in GW_types:
                if self._name.startswith(gw_type):
                    self._component = gw_type
                    break
        else:
            self._component = self._name

        self._ifaces = network_config
        self._state_port = state_port
        self._command_port = command_port

        self._enabled = True
        self._state = None

        self._init_status = InitStatus.NONE
        self._collector_functional = collector_functional

        self._lock = threading.Lock()

        # the tools dictionary {name: ToolModel}
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

        # the modules list
        self._modules = load_modules(self._component)

        # a list of modules that where not detected by the host
        self._missing_modules = []
        for module in self._modules:
            if module.get_name().upper() not in modules:
                self._log.warning("%s: plugin %s may be missing" %
                                  (name.upper(), module.get_name()))
                self._missing_modules.append(module)
        self.reload_modules(scenario)

    def reload_all(self, scenario):
        """ reload host to update the scenario path """
        self.reload_conf(scenario)
        self.reload_tools(scenario)
        self.reload_modules(scenario)

    def reload_conf(self, scenario):
        """ reload the host configuration """
        #TODO: this should not be called!!
        try:
            if not self.get_advanced_conf():
                self.set_advanced_conf(scenario)
            else:
                self.get_advanced_conf().load(scenario)
        except ModelException as error:
            self._log.warning("%s: %s" % (self._name.upper(), error))

    def reload_tools(self, scenario):
        """ update the scenario path for tools configuration """
        for tool_name in self._tools:
            try:
                self._tools[tool_name].update(scenario)
            except ModelException as error:
                self._log.warning("%s: %s" % (self._name.upper(), error))

    def reload_modules(self, scenario):
        """ update the scenario path for modules configuration """
        for module in self._modules:
            try:
                module.update(scenario, self._component, self._name)
            except ModelException as error:
                self._log.warning("%s: %s" % (self._name.upper(), error))

    def get_modules(self):
        """get the modules """
        return self._modules

    def get_module(self, name):
        """ get a module according to its name """
        for module in self._modules:
            if name == module.get_name():
                return module

    def get_lan_adapt_modules(self):
        """ get the lan adaptation modules {name: module} """
        modules = {}
        for module in self._modules:
            if module.get_type() == "lan_adaptation":
                modules[module.get_name()] = module
        return modules

    def get_missing_modules(self):
        """ get the missing modules """
        return self._missing_modules

    def update_files(self, changed):
        """ update the source files according to user configuration """
        if self.get_advanced_conf() is not None:
            self.get_advanced_conf().get_files().update(changed)
        for tool in self._tools.values():
            files = tool.get_files()
            if files is not None:
                files.update(changed)
        for module in self._modules:
            files = module.get_files()
            if files is not None:
                files.update(changed)
        if len(changed) > 0:
            for filename in changed:
                self._log.warning("%s: the file %s has not been updated" %
                                  (self._name.upper(), filename))

    def get_deploy_files(self):
        """ get the files to deploy (modified files) """
        deploy_files = []
        if self.get_advanced_conf() is not None:
            deploy_files += self.get_advanced_conf().get_files().get_modified()
        for tool in self._tools.values():
            files = tool.get_files()
            if files is not None:
                deploy_files += files.get_modified()
        for module in self._modules:
            files = module.get_files()
            if files is not None:
                deploy_files += files.get_modified()
        return deploy_files

    def set_deployed(self):
        """ the files were correctly deployed """
        if self.get_advanced_conf() is not None:
            self.get_advanced_conf().get_files().set_modified()
        for tool in self._tools.values():
            files = tool.get_files()
            if files is not None:
                files.set_modified()
        for module in self._modules:
            files = module.get_files()
            if files is not None:
                files.set_modified()

    def first_deploy(self):
        """ check if this is the first deploy """
        if self.get_advanced_conf() is not None:
            return self.get_advanced_conf().get_files().is_first()
        else:
            # WS case, we may have tools files to deploy
            for tool in self._tools.values():
                files = tool.get_files()
                if files is not None:
                    return files.is_first()
            return False

    def get_advanced_conf(self):
        """ get the advanced configuration """
        return self._host_model.get_advanced_conf()

    def set_advanced_conf(self, scenario):
        self._host_model.reload_conf(scenario)

    def get_lan_adaptation(self):
        """ get the lan adaptation_schemes values """
        return self.get_advanced_conf().get_stack("lan_adaptation_schemes", 'proto')

    def get_debug(self):
        """ get the debug section content """
        return self.get_advanced_conf().get_debug()

    def get_component(self):
        """ return the component type """
        return self._component

    def get_host_component(self):
        return self._host_model.get_component()

    def get_instance(self):
        """ get the component instance """
        return self._host_model.get_instance()

    def get_name(self):
        """ get the name of a host """
        return self._name

    def get_host_name(self):
        """ get the short name of a host """
        return self._host_model.get_name()

    def get_state(self):
        """ get the host state """
        self._lock.acquire()
        state = self._state
        self._lock.release()
        return state

    def get_init_status(self):
        """ get the host initialisation state """
        self._lock.acquire()
        status = self._init_status
        self._lock.release()
        return status

    def set_init_status(self, status):
        """ set the host initialisation state """
        self._lock.acquire()
        self._init_status = status
        self._lock.release()

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
            self._init_status = InitStatus.NONE
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
                # if the collector is registerd and the host status was not
                # updated set it to pending 
                if not self._collector_functional and \
                   self._init_status != InitStatus.FAIL:
                    self._init_status = InitStatus.SUCCESS
                if self._collector_functional and \
                   self._init_status == InitStatus.NONE:
                    self._init_status = InitStatus.PENDING
                self._state = True
            else:
                # TODO notice
                self._log.info(self._name + ": component '" +
                               key + "' does not belong to model")
        self._lock.release()

    def get_ip_address(self):
        """ get the host IP address """
        return self._ifaces["discovered"]

    def get_emulation_address(self):
        """ get the host emulation IPv4 address """
        try:
            return self._ifaces["emu_ipv4"].split('/')[0]
        except KeyError:
            self._log.error("cannot retrieve IPv4 emulation address, mandatory "
                            "for component starting")
            return ""

    def get_emulation_interface(self):
        """ get the host emulation interface """
        try:
            return self._ifaces["emu_iface"].split('/')[0]
        except KeyError:
            self._log.error("cannot retrieve IPv4 emulation interface name, "
                            "mandatory for component starting")
            return ""

    def get_lan_interface(self):
        """ get the host LAN interface """
        try:
            return self._ifaces["lan_iface"].split('/')[0]
        except KeyError:
            self._log.error("cannot retrieve IPv4 LAN interface name, "
                            "mandatory for component starting")
            return ""

    def get_lan_address(self):
        """ get the host LAN IPv4 address """
        try:
            return self._ifaces["lan_ipv4"].split('/')[0]
        except KeyError:
            self._log.error("cannot retrieve IPv4 LAN address, mandatory "
                            "for component starting")
            return ""

    def get_lan_address_v6(self):
        """ get the host LAN IPv6 address """
        try:
            return self._ifaces["lan_ipv6"].split('/')[0]
        except KeyError:
            return ""

    def get_lan_mac(self):
        """ get the host LAN MAC address """
        try:
            return self._ifaces["mac"].split('/')[0]
        except KeyError:
            return ""

    def get_iface(self, key):
        """ get the key from ifaces """
        try:
            return self._ifaces[key]
        except KeyError:
            return ""
    
    def get_upward_port(self):
        """ get the upward port """
        try:
            return self._ifaces['upward_port']
        except KeyError:
            return "10001"
    
    def get_downward_port(self):
        """ get the upward port """
        try:
            return self._ifaces['downward_port']
        except KeyError:
            return "10002"
    
    def get_remote_ip_addr(self):
        """ get the upward port """
        try:
            return self._ifaces['remote_ip']
        except KeyError:
            return ""

    def get_state_port(self):
        """ get the state server port """
        return int(self._state_port)

    def get_command_port(self):
        """ get the command server port """
        return int(self._command_port)

    def enable(self, val):
        """ enable host """
        if self.get_advanced_conf() is None:
            return
        if val:
            self.get_advanced_conf().enable()
        else:
            self.get_advanced_conf().disable()

    def is_enabled(self):
        """ check if host is enabled """
        return self._host_model.is_enabled()

    def get_tools(self):
        """ get the host tools """
        return self._tools.values()

    def get_tool(self, tool_name):
        """ get the host tools """
        if tool_name in self._tools:
            return self._tools[tool_name]
        return None

    def set_collector_functional(self, status):
        """ the collector responds to manager registration """
        self._collector_functional = status

    def is_collector_functional(self):
        """ does the collector responds to manager registration """
        return self._host_model.is_collector_functional()

    def set_lan_adaptation(self, stack):
        """ set the lan_adaptation_schemes values """
        lan_adapt = stack
        self.get_advanced_conf().set_stack('lan_adaptation_schemes',
                                 lan_adapt, 'proto')

    def get_interface_type(self):
        """ get the type of interface according to the stack """
        if self._component not in [SAT, WS]:
            lan_adapt = self.get_advanced_conf().get_stack('lan_adaptation_schemes',
                                                      'proto')
            try:
                name = lan_adapt['0']
                modules = self.get_lan_adapt_modules()
                return modules[name].get_interface_type()
            except KeyError:
                raise ModelException("cannot find first Lan Adaptation scheme")
        return ''

    def get_gw_id(self):
        return self._gw_id

    def set_gw_id(self, gw_id):
        self._gw_id = gw_id
    
    def get_spot_id(self):
        return self._spot_id
    
    def set_spot_id(self, spot_id):
        self._spot_id = spot_id

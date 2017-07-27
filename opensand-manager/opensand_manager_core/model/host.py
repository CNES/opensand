#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2017 TAS
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
host.py - Host model for OpenSAND manager
"""

import threading

from opensand_manager_core.model.machine import InitStatus
from opensand_manager_core.model.tool import ToolModel
from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_core.model.host_advanced import AdvancedHostModel
from opensand_manager_core.module import load_modules
from opensand_manager_core.utils import GW, WS, ST, SAT, GW_types, HOST_TEMPLATES

class HostModel:
    """ host model """
    def __init__(self, name, instance, network_config, state_port,
                 command_port, tools, modules, scenario, manager_log,
                 collector_functional, spot_id = "", gw_id = ""):
        self._log = manager_log
        self._name = name
        self._instance = instance
        self._gw_id = gw_id
        self._spot_id = spot_id
        self._machines = {}
        if self._name.startswith(ST):
            self._component = ST
        elif self._name.startswith(WS):
            self._component = WS
        elif self._name.startswith(GW):
            self._component = GW
        else:
            self._component = self._name

        self._ifaces = network_config
        self._state_port = state_port
        self._command_port = command_port

        self._enabled = True
        self._advanced = None
        self._state = None

        self._init_status = InitStatus.NONE
        self._collector_functional = collector_functional

        if self._component != WS:
            try:
                self._advanced = AdvancedHostModel(self._name, scenario)
            except ModelException, error:
                self._log.error("%s: %s" % (self._name.upper(), error))
                raise

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
        # TODO: modules should be loaded only by machine model
        self._modules = load_modules(self._component)

        self.reload_modules(scenario)

    def reload_all(self, scenario):
        """ reload host to update the scenario path """
        self.reload_conf(scenario)
        self.reload_tools(scenario)
        self.reload_modules(scenario)

    def reload_conf(self, scenario):
        """ reload the host configuration """
        # TODO:
        try:
            if not self._advanced:
                self._advanced = AdvancedHostModel(self._name, scenario)
            else:
                self._advanced.load(scenario)
        except ModelException as error:
            self._log.warning("%s: %s" % (self._name.upper(), error))

    def reload_tools(self, scenario):
        """ update the scenario path for tools configuration """
        for machine in [x for x in self._machines if self._machines[x]]:
            self._machines[machine].reload_tools(scenario)

    def reload_modules(self, scenario):
        """ update the scenario path for modules configuration """
        for machine in [x for x in self._machines if self._machines[x]]:
            self._machines[machine].reload_modules(scenario)

    def get_modules(self):
        """get the modules """
        # TODO: self._modules should store minimum set
        for machine in [x for x in self._machines if self._machines[x]]:
            return self._machines[machine].get_modules()

    def get_module(self, name):
        """ get a module according to its name """
        # TODO: self._modules should store minimum set
        for machine in [x for x in self._machines if self._machines[x]]:
            return self._machines[machine].get_module(name)

    def get_lan_adapt_modules(self):
        """ get the lan adaptation modules {name: module} """
        # TODO: self._modules should store minimum set
        for machine in [x for x in self._machines if self._machines[x]]:
            return self._machines[machine].get_lan_adapt_modules()

    def get_missing_modules(self):
        """ get the missing modules """
        # TODO: self._modules should store maximum set
        for machine in [x for x in self._machines if self._machines[x]]:
            return self._machines[machine].get_missing_modules()

    def update_files(self, changed):
        """ update the source files according to user configuration """
        # TODO: shouldnt be used. should be safe to remove
        for machine in [x for x in self._machines if self._machines[x]]:
            self._machines[machine].update_files(changed)

    def get_deploy_files(self):
        """ get the files to deploy (modified files) """
        # TODO: shouldnt be used. should be safe to remove
        deploy_files = []
        if self._advanced is not None:
            deploy_files += self._advanced.get_files().get_modified()
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
        # TODO: shouldnt be used. should be safe to remove
        if self._advanced is not None:
            self._advanced.get_files().set_modified()
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
        # TODO: shouldnt be used. should be safe to remove
        if self._advanced is not None:
            return self._advanced.get_files().is_first()
        else:
            # WS case, we may have tools files to deploy
            for tool in self._tools.values():
                files = tool.get_files()
                if files is not None:
                    return files.is_first()
            return False
    
    def get_advanced_conf(self):
        """ get the advanced configuration """
        return self._advanced

    def get_lan_adaptation(self):
        """ get the lan adaptation_schemes values """
        return self._advanced.get_stack("lan_adaptation_schemes", 'proto')

    def get_debug(self):
        """ get the debug section content """
        return self._advanced.get_debug()

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
        state = True
        for machine in [x for x in self._machines if self._machines[x]]:
            state = state and self._machines[machine].get_state()
        return state

    def get_init_status(self):
        """ get the host initialisation state """
        status = InitStatus.NONE
        for machine in [x for x in self._machines if self._machines[x]]:
            substatus = self._machines[machine].get_init_status()
            if substatus > status:
                status = substatus
        return status

    def set_init_status(self, status):
        """ set the host initialisation state """
        # TODO: this should never be used
        self._log.error("this shouldnt be used")
        self._lock.acquire()
        self._init_status = status
        self._lock.release()

    def set_started(self, started_list):
        """ set the specified hosts states to True """
        for machine in [x for x in self._machines if self._machines[x]]:
            self._machines[machine].set_started(started_list)
    
    def enable(self, val):
        """ enable host """
        if self._advanced is None:
            return
        self._lock.acquire()
        if val:
            self._advanced.enable()
        else:
            self._advanced.disable()
        self._lock.release()

    def is_enabled(self):
        """ check if host is enabled """
        if self._advanced is None:
            return True
        return self._advanced.is_enabled()

    def get_tools(self):
        """ get the host tools """
        # TODO: shouldnt be used
        for machine in [x for x in self._machines if self._machines[x]]:
            return self._machines[machine].get_tools()

    def get_tool(self, tool_name):
        """ get the host tools """
        for machine in [x for x in self._machines if self._machines[x]]:
            return self._machines[machine].get_tool(tool_name)
    
    def set_collector_functional(self, status):
        """ the collector responds to manager registration """
        self._collector_functional = status

    def is_collector_functional(self):
        """ does the collector respond to manager registration """
        return self._collector_functional

    def set_lan_adaptation(self, stack):
        """ set the lan_adaptation_schemes values """
        lan_adapt = stack
        self._advanced.set_stack('lan_adaptation_schemes',
                                 lan_adapt, 'proto')

    def get_interface_type(self):
        """ get the type of interface according to the stack """
        if self._component not in [SAT, WS]:
            lan_adapt = self._advanced.get_stack('lan_adaptation_schemes',
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

    def add_machine(self, machine):
        """ add sub component """
        # TODO should verify that it is the same instance number and stuff
        component = machine.get_component()
        try:
            if self._machines[component]:
                self._log.error("component %s already in %s" % 
                        (component, self._name))
                return False
        except KeyError:
            pass
        self._machines[component] = machine
        return True
    
    def del_machine(self, name):
        """ removes machine from list """
        for key, machine in self._machines.iteritems():
            if machine.get_name() == name:
                break
        try:
            del self._machines[key]
        except:
            return False
        return True

    def get_machines(self):
        """ gets machine models """
        return self._machines

    def is_complete(self):
        """ get is_complete """
        for template in HOST_TEMPLATES[self._component]:
            found_all = True
            for machine in template:
                if machine not in self._machines:
                    found_all = False
            if found_all:
                return True
        return False

    def is_empty(self):
        return (len(self._machines) == 0)

    def get_net_config(self):
        """ gets network config """
        if not self.is_complete():
            return None
        net_config = {}
        for m in [x for x in self._machines if self._machines[x]]:
            if (m in {ST, SAT, GW} or m.endswith('phy')):
                net_config['emu_iface'] = self._machines[m].get_iface('emu_iface')
                net_config['emu_ipv4'] = self._machines[m].get_iface('emu_ipv4')
            if (m in {ST, GW} or m.endswith('net-acc')):
                net_config['lan_iface'] = self._machines[m].get_iface('lan_iface')
                net_config['lan_ipv4'] = self._machines[m].get_iface('lan_ipv4')
                net_config['lan_ipv6'] = self._machines[m].get_iface('lan_ipv6')
                net_config['mac'] = self._machines[m].get_iface('mac')
        # no 'discovered' because there are more than one
        return net_config


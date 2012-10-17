#!/usr/bin/env python
# -*- coding: utf-8 -*-

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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
opensand_model.py - OpenSAND manager model
"""

import ConfigParser
import os
import shutil

from opensand_manager_core.model.event_manager import EventManager
from opensand_manager_core.model.host import HostModel
from opensand_manager_core.model.global_config import GlobalConfig
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_core.loggers.manager_log import ManagerLog
from opensand_manager_core.opensand_xml_parser import XmlParser
from opensand_manager_core.modules import *
from opensand_manager_core.encap_module import encap_methods

MAX_RECENT = 5

TOPOLOGY_CONF = "topology.conf"
TOPOLOGY_XSD = "topology.xsd"

class Model:
    """ Model for OpenSAND """
    def __init__(self, manager_log, scenario = ''):
        # initialized in load
        self._inifile = None
        self._log = manager_log

        # start event managers
        self._event_manager = EventManager("manager")
        self._event_manager_response = EventManager("response")

        self._modules = {}
        self._missing_modules = {}
        self._scenario_path = scenario
        self._is_default = False
        self._modified = False
        self._run_id = "default"

        # Running in dev mode ?
        self._is_dev_mode = False

        self._hosts = []
        self._ws = []
        self._collector_known = False

        self._config = None
        self._topology = None

        # load modules
        self.load_modules()
        if len(self._modules) == 0:
            raise ModelException("You need encapsulation modules to use your "
                                 "platform")

        try:
            self.load()
        except ModelException:
            raise

    def load(self):
        """ load the model scenario """
        # load the scenario
        self._is_default = False
        if not 'HOME' in os.environ:
            raise ModelException("cannot get HOME environment variable")

        # no scenario to load use the default path
        if self._scenario_path == "":
            self._scenario_path = os.path.join(os.environ['HOME'],
                                               ".opensand/default")
            self.clean_default()
            self._is_default = True
        else:
            recents = []
            filename = os.path.join(os.environ['HOME'], ".opensand/recent")
            if os.path.exists(filename):
                with open(filename, 'r') as recent_file:
                    for line in recent_file:
                        recents.append(line.strip())

            # remove current path as it will be append at the end
            if os.path.realpath(self._scenario_path) in recents:
                recents.remove(os.path.realpath(self._scenario_path))

            # remove older folders
            while len(recents) > MAX_RECENT - 1:
                recents.pop(len(recents) - 1)

            recents.insert(0, os.path.realpath(self._scenario_path))

            with open(filename, 'w') as recent_file:
                for recent in recents:
                    recent_file.write(recent + '\n')

        # create the scenario path if necessary
        if not os.path.exists(self._scenario_path):
            try:
                os.makedirs(self._scenario_path, 0755)
            except OSError, (errno, strerror):
                raise ModelException("cannot create directory '%s': %s" %
                                     (self._scenario_path, strerror))

        # load the topology configuration
        self.load_topology()

        # actualize the tools scenario path
        for host in self._hosts:
            host.reload_all(self._scenario_path)

        # load modules configuration
        self.reload_modules()

        # read configuration file
        try:
            self._config = GlobalConfig(self._scenario_path)
        except ModelException:
            raise
        
    def load_modules(self):
        """ load the modules """
        # add  modules in tree
        for name in encap_methods.keys():
            module = encap_methods[name]()
            self._modules[name] = module

    def load_topology(self):
        """ load or reload the topology configuration """
        topo_conf = os.path.join(self._scenario_path, TOPOLOGY_CONF)
        topo_xsd = os.path.join('/usr/share/opensand', TOPOLOGY_XSD)
        try:
            if self._topology is not None:
                # copy the previous topology in the new file
                self._topology.write(topo_conf)
            else:
                # get the default topology file
                default_topo = os.path.join('/usr/share/opensand', TOPOLOGY_CONF)
                shutil.copy(default_topo, topo_conf)

            self._topology = XmlParser(topo_conf, topo_xsd)
        except IOError, (errno, strerror):
            raise ModelException("cannot load topology configuration: %s " %
                                 strerror)
            
    def reload_modules(self):
        """ load or reload the modules configuration """
        for name in self._modules:
            module = self._modules[name]
            # handle the module configuration
            xml = module.get_xml()
            xsd = module.get_xsd()
            if xml is None:
                continue

            plugins_path = os.path.join(self._scenario_path, 'plugins')
            xml_path = os.path.join(plugins_path, xml)
            xsd_path = os.path.join('/usr/share/opensand/plugins', xsd)
            # create the plugins path if necessary
            if not os.path.exists(plugins_path):
                try:
                    os.makedirs(plugins_path, 0755)
                except OSError, (errno, strerror):
                    raise ModelException("cannot create directory '%s': %s" %
                                         (plugins_path, strerror))
            # create the configuration file if necessary
            if not os.path.exists(xml_path):
                try:
                    default_path = os.path.join('/usr/share/opensand/plugins', xml)
                    shutil.copy(default_path, xml_path)
                except IOError, (errno, strerror):
                    raise ModelException("cannot copy %s plugin configuration "
                                         "from '%s' to '%s': %s" % (name,
                                         default_path, xml_path, strerror))

            try:
                config_parser = XmlParser(xml_path, xsd_path)
            except IOError, msg:
                raise ModelException("cannot load module %s configuration:"
                                     "\n\t%s" % (name, msg))
            except XmlException, msg:
                raise ModelException("failed to parse module %s configuration file:"
                                     "\n\t%s" % (name, msg))
            module.set_config_parser(config_parser)

    def add_topology(self, name, instance, net_config):
        """ Add a new host in the topology configuration file """
        try:
            if name == 'sat':
                att_path = '/configuration/sat_carrier/carriers/carrier' \
                           '[@up="true" and @ip_multicast="false"]'
                self._topology.set_values(net_config['emu_ipv4'].split('/')[0],
                                          att_path, 'ip_address')
            elif name == 'gw':
                att_path = '/configuration/sat_carrier/carriers/carrier' \
                           '[@up="false" and @ip_multicast="false"]'
                self._topology.set_values(net_config['emu_ipv4'].split('/')[0],
                                          att_path, 'ip_address')

            if name != "sat":
                addr = net_config['lan_ipv4'].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind('.') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        'tal_id': instance,
                       }
                xpath = '/configuration/ip_dedicated_v4/terminals'
                self._topology.create_line(line, 'terminal_v4', xpath)
                addr = net_config['lan_ipv6'].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind(':') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        'tal_id': instance,
                       }
                xpath = '/configuration/ip_dedicated_v6/terminals'
                self._topology.create_line(line, 'terminal_v6', xpath)

            self._topology.write()
        except XmlException, msg:
            self._log.error("failed to add topology for %s: %s" % (name,
                                                                   str(msg)))
        except KeyError, msg:
            self._log.error("cannot find network keys %s for topology updating "
                            "on %s" % (msg, name))
        except Exception, msg:
            self._log.error("unknown exception when trying to add topology for "
                            "%s: %s" % (name, str(msg)))

    def remove_topology(self, instance):
        """ remove a host from topology configuration file """
        try:
            xpath = "/configuration/ip_dedicated_v4/terminals/terminal_v4" \
                    "[@tal_id='%s']" % instance
            self._topology.del_element(xpath)
            xpath = "/configuration/ip_dedicated_v6/terminals/terminal_v6" \
                    "[@tal_id='%s']" % instance
            self._topology.del_element(xpath)
            self._topology.write()
        except XmlException, msg:
            self._log.error("failed to remove host with id %s in topology: %s" %
                            (instance, str(msg)))
        except Exception, msg:
            self._log.error("unknown exception when trying to remove topology "
                            "for host with instance %s: %s" % (instance,
                                                               str(msg)))

    def close(self):
        """ release the model """
        self._log.debug("Model: close")
        self._event_manager.set('quit')
        self._event_manager_response.set('quit')
        self._log.debug("Model: closed")

    def get_hosts_list(self):
        """ return the hosts list """
        return self._hosts

    def get_host(self, name):
        """ return the host according to its name """
        for host in self.get_all():
            if name == host.get_name():
                return host
        if name == 'global':
            return self._config
        return None

    def get_workstations_list(self):
        """ return the workstations list """
        return self._ws

    def get_all(self):
        """ return the hosts and workstation list """
        return self._hosts + self._ws

    def del_host(self, name):
        """ remove an host """
        idx = 0
        for host in self._hosts:
            if name == host.get_name():
                self._log.debug("remove host: '" + name + "'")
                if not name == 'sat':
                    self.remove_topology(self._hosts[idx].get_instance())
                del self._hosts[idx]
            idx += 1

        idx = 0
        for host in self._ws:
            if name == host.get_name():
                self._log.debug("remove host: '" + name + "'")
                del self._ws[idx]
            idx += 1

        for module in self._missing_modules:
            if name in self._missing_modules[module]:
                self._missing_modules[module].remove(name)

    def add_host(self, name, instance, network_config,
                 state_port, command_port, tools, host_modules):
        """ add an host in the host list """
        # remove instance for ST and WS
        if name.startswith('st'):
            component = 'st'
        elif name.startswith('ws'):
            component = 'ws'
        else:
            component = name

        # check if we have all the correct information
        checked = True
        if (component == 'st' or component == 'ws') and instance == '':
            self._log.warning(name + ": "
                              "service received with no instance information")
            checked = False
        if not str(state_port).isdigit() or not str(command_port).isdigit():
            self._log.warning(name + ": "
                              "service received with no state or command port")
            checked = False

        ip_addr = network_config['discovered']
        # find if the component already exists
        for host in self._hosts:
            if host.get_name() == name:
                self._log.warning("%s : duplicated service received at "
                                  "address %s" % (name, ip_addr))
                raise ModelException
        for host in self._ws:
            if host.get_name() == name:
                self._log.warning("%s : duplicated service received at "
                                  "address %s" % (name, ip_addr))
                raise ModelException


        self._log.debug("add host '%s'" % name)
        # report a warning if a module is not supported by the host
        for module in [mod for mod in self._modules
                           if mod.upper() not in host_modules]:
            if component != 'ws':
                self._log.warning("%s does not support %s plugin" %
                                  (name.upper(), module))
                if not module in self._missing_modules:
                    self._missing_modules[module] = [name]
                else:
                    self._missing_modules[module].append(name)
        # the component does not exist so create it
        host = HostModel(name, instance, network_config, state_port,
                         command_port, tools, self._scenario_path, self._log)
        if component == 'sat':
            self._hosts.insert(0, host)
        elif component == 'gw':
            self._hosts.insert(1, host)
        elif component != 'ws':
            self._hosts.append(host)
        else:
            self._ws.append(host)

        if component != "ws":
            self.add_topology(name, instance, network_config)

        if not checked:
            raise ModelException
        else:
            return host

    def is_running(self):
        """ check if at least one host or controller is running """
        ret = False
        for host in self._hosts:
            if host.get_state() is not None:
                ret = ret or host.get_state()

        if ret:
            self._modified = True
        return ret

    def set_dev_mode(self, dev_mode=False):
        """ Set the dev mode to `dev_mode` """
        self._log.debug("Switch to dev mode %s" % dev_mode)
        self._is_dev_mode = dev_mode

    def get_dev_mode(self):
        """get the dev mode """
        return self._is_dev_mode

    def set_scenario(self, val):
        """ set the scenario id """
        self._modified = True
        self._scenario_path = val
        self.load()

    def get_scenario(self):
        """ get the scenario id """
        return self._scenario_path

    def set_run(self, val):
        """ set the scenario id """
        self._modified = True
        self._run_id = val
        if self._run_id == "":
            self._run_id = "default"


    def get_run(self):
        """ get the scenario id """
        return self._run_id

    def get_event_manager(self):
        """ get the event manager """
        return self._event_manager

    def get_event_manager_response(self):
        """ get the event manager response """
        return self._event_manager_response

    def main_hosts_found(self):
        """ check if OpenSAND main hosts were found in the platform """
        # check that we have at least sat, gw and one st
        sat = False
        gw = False
        st = False

        for host in self._hosts:
            if host.get_component() == 'sat' and host.get_state() != None:
                sat = True
            if host.get_component() == 'gw' and host.get_state() != None:
                gw = True
            if host.get_component() == 'st' and host.get_state() != None:
                st = True

        return sat and gw and st
    
    def is_collector_known(self):
        """ indicates if the environment plane collector service has been
        found. """
        
        return self._collector_known
    
    def set_collector_known(self, collector_known):
        """ called by the service listener when the collector service is found
        or lost. """
        
        self._collector_known = collector_known

    def clean_default(self):
        """ clean the $HOME/.opensand directory from default files """
        if os.path.exists(self._scenario_path):
            try:
                shutil.rmtree(self._scenario_path)
            except (OSError, os.error), msg:
                self._log.warning("Cannot clean default scenario: %s" %
                                  str(msg))

    def is_default_modif(self):
        """ check if we work on the default path and if we modified it """
        return self._is_default and self._modified

    def is_default(self):
        """ check if we work on default path """
        return self._is_default

    def get_conf(self):
        """ get the global configuration """
        return self._config
    
    def get_modules(self):
        """ get the module list """
        return self._modules

    def get_missing(self):
        """ get the missing module list """
        return self._missing_modules


##### TEST #####
if __name__ == "__main__":
    import sys

    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)
    try:
        CONFIG = MODEL.get_conf()
        LOGGER.debug("payload type: " + CONFIG.get_payload_type())
        LOGGER.debug("emission standard: " + CONFIG.get_emission_std())
        LOGGER.debug("uplink encapsulation protocol: " +
                     CONFIG.get_up_return_encap())
        LOGGER.debug("downlink encapsulation protocol: " +
                     CONFIG.get_down_forward_encap())
        LOGGER.debug("terminal type: " + CONFIG.get_terminal_type())
        LOGGER.debug("frame duration: " + str(CONFIG.get_frame_duration()))

        MODEL.add_host('st1', '1', '127.0.0.1', 1111, 2222, {}, {})
        MODEL.add_host('st3', '3', '127.0.0.1', 1111, 2222, {}, {})
        NAMES = ''
        for HOST in MODEL.get_hosts_list():
            NAMES = NAMES + HOST.get_name() + ", "
        LOGGER.debug("hosts: " + NAMES[:len(NAMES)-2])

        MODEL.del_host('st1')
        NAMES = ''
        for HOST in MODEL.get_hosts_list():
            NAMES = NAMES + HOST.get_name() + ", "
        LOGGER.debug("hosts: " + NAMES[:len(NAMES)-2])
    except ModelException:
        LOGGER.error("model error")
        sys.exit(1)
    except ConfigParser.Error:
        LOGGER.error("error when reading configuration")
        sys.exit(1)
    finally:
        MODEL.close()

    sys.exit(0)

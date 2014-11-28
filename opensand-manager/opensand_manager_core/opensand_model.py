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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
opensand_model.py - OpenSAND manager model
"""

import ConfigParser
import os
import shutil


from opensand_manager_core.utils import OPENSAND_PATH
from opensand_manager_core.model.environment_plane import SavedProbeLoader
from opensand_manager_core.model.event_manager import EventManager
from opensand_manager_core.model.host import HostModel
from opensand_manager_core.model.global_config import GlobalConfig
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_core.loggers.manager_log import ManagerLog
from opensand_manager_core.opensand_xml_parser import XmlParser
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.module import load_modules

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

        # a list of modules that where not detected by some hosts
        self._missing_modules = {}
        self._scenario_path = scenario
        self._is_default = False
        self._modified = False
        self._run_id = "default"

        # Running in dev mode ?
        self._is_dev_mode = False
        # Running in adv mode ?
        self._is_adv_mode = False

        self._hosts = []
        self._ws = []
        self._collector_known = False
        self._collector_functional = False

        # the global config
        self._config = None
        self._topology = None

        # load the global modules
        self._modules = load_modules('global')

        # {host: {sim file xpath: new_source_file}}
        self._changed_sim_files = {}

        # check if there is already a running simulation
        filename = os.path.join(os.environ['HOME'], ".opensand/current")
        if os.path.exists(filename):
            with open(filename, 'r') as current:
                self._scenario_path = current.readline().strip()
                self._run_id = current.readline().strip()
                self._log.info("Manager was stopped while playing scenario %s "
                               "run %s" % (self._scenario_path, self._run_id))
            os.remove(filename)
        try:
            self.load(True)
        except ModelException:
            raise

    def load(self, first=False):
        """ load the model scenario """
        # load the scenario
        self._is_default = True
        if not 'HOME' in os.environ:
            raise ModelException("cannot get HOME environment variable")

        default = os.path.join(os.environ['HOME'], ".opensand/default")
        # no scenario to load use the default path
        if self._scenario_path == "":
            self._scenario_path = default
            self.clean_default()
        elif self._scenario_path != default:
            self._is_default = False
            # we check that this is not the defautl scenario path because
            # in the case where the default scenario was running when starting
            # we do not clean the path but we don't want to keep it in recents
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
            except OSError, (_, strerror):
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
            if self._config is None:
                self._config = GlobalConfig(self._scenario_path)
            else:
                self._config.load(self._scenario_path)
        except ModelException:
            raise

        if not first:
            # deploy the simulation files when loading a new scenario
            self._event_manager.set('deploy_files')

    def load_topology(self):
        """ load or reload the topology configuration """
        topo_conf = os.path.join(self._scenario_path, TOPOLOGY_CONF)
        topo_xsd = os.path.join(OPENSAND_PATH, TOPOLOGY_XSD)
        try:
            if self._topology is not None:
                # copy the previous topology in the new file
                self._topology.write(topo_conf)
            else:
                # get the default topology file
                default_topo = os.path.join(OPENSAND_PATH,
                                            TOPOLOGY_CONF)
                shutil.copy(default_topo, topo_conf)

            self._topology = XmlParser(topo_conf, topo_xsd)
        except IOError, (_, strerror):
            raise ModelException("cannot load topology configuration: %s " %
                                 strerror)

    def reload_modules(self):
        """ load or reload the modules configuration """
        for module in self._modules:
            module.update(self._scenario_path, 'global')

    def add_topology(self, name, instance, net_config):
        """ Add a new host in the topology configuration file """
        try:
            if name.startswith('ws') and '_' in instance:
                instance = instance.split('_')[0]
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
                # IPv4 SARP
                addr = net_config['lan_ipv4'].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind('.') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        'tal_id': instance,
                       }
                xpath = '/configuration/sarp/ipv4'
                self._topology.create_line(line, 'terminal_v4', xpath)
                # IPv6 SARP
                addr = net_config['lan_ipv6'].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind(':') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        'tal_id': instance,
                       }
                xpath = '/configuration/sarp/ipv6'
                self._topology.create_line(line, 'terminal_v6', xpath)
                # Ethernet SARP
                if 'mac' in net_config:
                    mac = net_config['mac']
                    # we can have several MAC addresses separated by space
                    macs = mac.split(' ')
                    for mac in macs:
                        line = {'mac': mac,
                                'tal_id': instance,
                               }
                        xpath = '/configuration/sarp/ethernet'
                        self._topology.create_line(line, 'terminal_eth', xpath)

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
            xpath = "/configuration/sarp/ipv4/terminal_v4" \
                    "[@tal_id='%s']" % instance
            self._topology.del_element(xpath)
            xpath = "/configuration/sarp/ipv6/terminal_v6" \
                    "[@tal_id='%s']" % instance
            self._topology.del_element(xpath)
            xpath = "/configuration/sarp/ethernet/terminal_eth" \
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
        # keep current scenario if running
        filename = os.path.join(os.environ['HOME'], ".opensand/current")
        if self.is_running():
            with open(filename, 'w') as current:
                current.write("%s\n%s" % (self._scenario_path, self._run_id))
                self._log.debug("Platform is running, keep current scenario "
                                "and run")
        else:
            if os.path.exists(filename):
                os.remove(filename)
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
            if name.lower() == host.get_name().lower():
                return host
        if name == 'global':
            return self
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
        for module in self._modules:
            if module.get_name().upper() not in host_modules:
                if component != 'ws':
                    self._log.warning("%s: plugin %s may be missing" %
                                      (name.upper(), module.get_name()))
                    if not module in self._missing_modules:
                        self._missing_modules[module] = [name]
                    else:
                        self._missing_modules[module].append(name)
        # the component does not exist so create it
        host = HostModel(name, instance, network_config, state_port,
                         command_port, tools, host_modules, self._scenario_path,
                         self._log, self._collector_functional)
        if component == 'sat':
            self._hosts.insert(0, host)
        elif component == 'gw':
            self._hosts.insert(1, host)
        elif component != 'ws':
            self._hosts.append(host)
        else:
            self._ws.append(host)

        self.add_topology(name, instance, network_config)

        if not checked:
            raise ModelException
        else:
            return host

    def host_ready(self, host):
        """ a host was correcly contacted by the controller """
        self._log.debug("%s is ready" % host.get_name())
        if host.get_state() == False:
            self._event_manager.set('deploy_files')

    def is_running(self):
        """ check if at least one host or controller is running """
        ret = False
        for host in self._hosts:
            if host.get_state() is not None:
                ret = ret or host.get_state()

        if ret:
            self._modified = True
        return ret

    def all_running(self):
        """ check if all components are running """
        alive = False
        for host in self.get_hosts_list():
            alive = True 
            if not host.get_state():
                alive = False
        return alive


    def running_list(self):
        """ get the name of the components that are still running """
        running = []
        for host in self.get_hosts_list():
            if host.get_state():
                running.append(str(host.get_name()).upper())
        return running

    def set_dev_mode(self, dev_mode=False):
        """ Set the dev mode to `dev_mode` """
        self._log.debug("Switch to dev mode %s" % dev_mode)
        self._is_dev_mode = dev_mode

    def get_dev_mode(self):
        """get the dev mode """
        return self._is_dev_mode

    def set_adv_mode(self, adv_mode=False):
        """ Set the adv mode to `adv_mode` """
        self._log.debug("Switch to adv mode %s" % adv_mode)
        self._is_adv_mode = adv_mode

    def get_adv_mode(self):
        """get the adv mode """
        # dev mode => adv_mode
        return self._is_adv_mode or self._is_dev_mode

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

    def is_collector_functional(self):
        """ indicates if the environment plane collector has responded to
            the manager registration. """
        return self._collector_functional

    def set_collector_known(self, collector_known):
        """ called by the service listener when the collector service is found
            or lost. """
        self._collector_known = collector_known
        if not collector_known:
            self._collector_functional = False

    def set_collector_functional(self, collector_functional):
        """ called by the probes controller when the collector responds to
            manager registration. """
        self._collector_functional = collector_functional
        for host in self.get_all():
            host.set_collector_functional(collector_functional)

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

    def get_module(self, name):
        """ get a module according to its name """
        for module in self._modules:
            if name == module.get_name():
                return module

    def get_encap_modules(self):
        """ get the encapsulation modules {name: module} """
        modules = {}
        for module in self._modules:
            if module.get_type() == "encap":
                modules[module.get_name()] = module
        return modules

    def get_global_lan_adaptation_modules(self):
        """ get the global lan adaptation modules {name: module}
            (i.e. header modification modules) """
        modules = {}
        for module in self._modules:
            if module.get_type() == "lan_adaptation":
                modules[module.get_name()] = module
        return modules

    def get_missing(self):
        """ get the missing module list """
        return self._missing_modules

    def get_saved_probes(self, run_id=None):
        """ get a SavedProbeLoader object with the saved probes objects """
        if run_id is None:
            run_id = self.get_run()
        if run_id == "default":
            # default is the equivalent of empty run_id
            return

        run_path = os.path.join(self.get_scenario(), run_id)

        try:
            return SavedProbeLoader(run_path)
        except ValueError, msg:
            error_popup("cannot parse saved probes files",
                        str(msg))
            return None

    def handle_file_changed(self, file_chooser, host_name, xpath):
        """ a source for a file from configuration has been updated """
        host = self.get_host(host_name)
        if not host in self._changed_sim_files:
            self._changed_sim_files[host] = {}
        self._changed_sim_files[host][xpath] = file_chooser.get_filename()


    def conf_apply(self):
        """ the advanced configuration has been applied, the file shoud be
            modified """
        for host in self._changed_sim_files:
            try:
                host.update_files(self._changed_sim_files[host], self._scenario_path)
            except IOError, (_, strerror):
                error_popup("Cannot update files on %s" % host.get_name(), strerror)

        self._changed_sim_files = {}
        # deploy the simulation files that were modified
        self._event_manager.set('deploy_files')

    def conf_undo(self):
        """ the advanced configuration hase no been applied, revert the source
            files """
        self._changed_sim_files = {}

    # functions for global host
    def update_files(self, changed, scenario):
        """ update the source files according to user configuration """
        self._config.get_files().update(changed, scenario)
        for module in self._modules:
            files = module.get_files()
            if files is not None:
                files.update(changed, scenario)
        if len(changed) > 0:
            for filename in changed:
                self._log.warning("The file %s has not been updated" %
                                  (filename))

    def get_deploy_files(self):
        """ get the files to deploy (modified files) """
        deploy_files = []
        deploy_files += self._config.get_files().get_modified(self._scenario_path)
        for module in self._modules:
            files = module.get_files()
            if files is not None:
                deploy_files += files.get_modified(self._scenario_path)
        return deploy_files

    def get_all_files(self):
        """ get the files to deploy (modified files) """
        deploy_files = []
        deploy_files += self._config.get_files().get_all(self._scenario_path)
        for module in self._modules:
            files = module.get_files()
            if files is not None:
                deploy_files += files.get_all(self._scenario_path)
        return deploy_files

    def set_deployed(self):
        """ the files were correctly deployed """
        self._config.get_files().set_modified(self._scenario_path)
        for module in self._modules:
            files = module.get_files()
            if files is not None:
                files.set_modified(self._scenario_path)

    def get_name(self):
        """ for compatibility with advanced dialog host calls """
        return 'global'

    def get_component(self):
        """ for compatibility with advanced dialog host calls """
        return 'global'

    def get_advanced_conf(self):
        """ for compatibility with advanced dialog host calls """
        return self._config

    def enable(self, val=True):
        """ for compatibility with advanced dialog host calls """
        pass


##### TEST #####
if __name__ == "__main__":
    import sys

    LOGGER = ManagerLog(7, True, True, True)
    MODEL = Model(LOGGER)
    try:
        CONFIG = MODEL.get_conf()
        LOGGER.debug("payload type: " + CONFIG.get_payload_type())
        LOGGER.debug("emission standard: " + CONFIG.get_emission_std())
        LOGGER.debug("uplink encapsulation protocol: " +
                     str(CONFIG.get_return_up_encap()))
        LOGGER.debug("downlink encapsulation protocol: " +
                     str(CONFIG.get_forward_down_encap()))
        net_config = {'discovered': '127.0.0.1',
                      'emu_iface': 'eth0',
                      'emu_ipv4': '127.0.0.1',
                      'lan_iface': 'eth1',
                      'lan_ipv4': '127.0.0.1',
                      'lan_ipv6': '::1',
                      'mac': '01:23:45:67:89'}

        MODEL.add_host('st1', '1', net_config, 1111, 2222, {}, {})
        MODEL.add_host('st3', '3', net_config, 1111, 2222, {}, {})
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

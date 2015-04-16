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

# Author: Bénédicte Motto / <bmotto@toulouse.viveris.com>

"""
topology_configuration.py - the topology configuration description
"""

import os
import shutil
from lxml import etree

from opensand_manager_core.utils import *
from opensand_manager_core.model.host_advanced import AdvancedHostModel
from opensand_manager_core.model.files import Files
from opensand_manager_core.my_exceptions import XmlException, ModelException
from opensand_manager_core.opensand_xml_parser import XmlParser

DEFAULT_CONF = OPENSAND_PATH + "topology.conf"
TOPOLOGY_XSD = OPENSAND_PATH + "topology.xsd"
CONF_NAME = "topology.conf"

class TopologyConfig(AdvancedHostModel):
    """ OpenSAND Topology configuration, displayed in configuration tab """
    def __init__(self, scenario, manager_log):
        self._list_host = {}
        AdvancedHostModel.__init__(self, 'topology', scenario)
        self._modules = []
        self._log = manager_log

    def load(self, scenario):
        """ load the global configuration """
        # create the host configuration directory
        conf_path = scenario
        if not os.path.isdir(conf_path):
            try:
                os.makedirs(conf_path, 0755)
            except OSError, (_, strerror):
                raise ModelException("failed to create directory '%s': %s" %
                                     (conf_path, strerror))

        self._conf_file = os.path.join(conf_path, CONF_NAME)
        # copy the configuration template in the destination directory
        # if it does not exist
        if not os.path.exists(self._conf_file):
            try:
                shutil.copy(DEFAULT_CONF, self._conf_file)
            except IOError, msg:
                raise ModelException("failed to copy %s configuration file in "
                                     "'%s': %s" % (self._name, self._conf_file,
                                                   msg))

        self._xsd = TOPOLOGY_XSD

        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
            if self._files is None:
                self._files = Files(self._name, self._configuration, scenario)
            else:
                self._files.load(scenario, self._configuration)
        except IOError, msg:
            raise ModelException("cannot load configuration: %s" % msg)
        except XmlException, msg:
            raise ModelException("failed to parse configuration: %s"
                                 % msg)

        #update topology adding host
        for host in self._list_host:
            instance = 0
            if host.startswith(GW) or host.startswith(ST) or host.startswith(WS):
                instance = host[2::]
            self.add(host, instance, self._list_host[host])
    
    def write(self, conf):
        self._configuration.write(conf)


    def save(self):
        """ save the configuration """
        try:
            self._configuration.write()
        except XmlException:
            raise

    def get_param(self, name):
        """ get a parameter in the XML configuration file """
        val = None
        key = self._configuration.get("//" + name)
        if key is not None:
            try:
                val = self._configuration.get_value(key)
            except:
                pass
        return val

    def update_files(self, changed, scenario):
        """ update the source files according to user configuration """
        self.get_files().update(changed, scenario)
        if len(changed) > 0:
            for filename in changed:
                self._log.warning("The file %s has not been updated" %
                                  (filename))

    def get_deploy_files(self):
        """ get the files to deploy (modified files) """
        deploy_files = []
        deploy_files += self.get_files().get_modified(self._scenario_path)
        return deploy_files

    def get_all_files(self):
        """ get the files to deploy (modified files) """
        deploy_files = []
        deploy_files += self.get_files().get_all(self._scenario_path)
        return deploy_files

    def set_deployed(self):
        """ the files were correctly deployed """
        self.get_files().set_modified(self._scenario_path)

    def get_name(self):
        """ for compatibility with advanced dialog host calls """
        return TOPOLOGY

    def get_component(self):
        """ for compatibility with advanced dialog host calls """
        return TOPOLOGY

    def get_advanced_conf(self):
        """ for compatibility with advanced dialog host calls """
        return self

    def enable(self, val=True):
        """ for compatibility with advanced dialog host calls """
        pass
    
    def get_modules(self):
        """ get the module list """
        return self._modules

    def get_module(self, name):
        """ get a module according to its name """
        return self._modules[name]

    def get_conf(self):
        return self._configuration

    def add(self, name, instance, net_config):
        """ Add a new host in the topology configuration file """
        self._list_host[name] = net_config
       
        try:
            if name.startswith(WS) and '_' in instance:
                instance = instance.split('_')[0]
            if name == SAT:
                att_path = '/configuration/sat_carrier/spot/carriers/carrier' \
                           '[(@' + CARRIER_TYPE + '="' + CTRL_IN + '" or@' + \
                           CARRIER_TYPE + '="' + LOGON_IN + '" or' \
                           '@' + CARRIER_TYPE + '="' + DATA_IN_GW + '" or' \
                           '@' + CARRIER_TYPE + '="' + DATA_IN_ST + '")' \
                           'and @' + IP_MULTICAST + '="false"]'
                self._configuration.set_values(net_config['emu_ipv4'].split('/')[0],
                                          att_path, IP_ADDRESS)
            elif name.startswith(GW):
                exist =  False
                for section in self._configuration.get_sections():
                    for child in section.iterchildren():
                        if (child.tag == GW and child.get(ID) == instance) or \
                           (child.tag == SPOT and child.get(GW) == instance):
                            exist =  True
                            continue
                        if child.tag ==  SPOT or child.tag == GW:
                            self._configuration.add_gw("//"+section.tag, instance) 

                if not exist:
                    self.update(gw_id = instance)

                gw_id = name[len(GW)::]
                att_path = '/configuration/sat_carrier/spot[@' + GW + '="' + gw_id + \
                           '"]/carriers/carrier[(@' + CARRIER_TYPE + '="' + LOGON_OUT + \
                           '" or @' + CARRIER_TYPE + '="'+ DATA_OUT_GW + '")' \
                           'and @' + IP_MULTICAST + '="false"]'
                self._configuration.set_values(net_config['emu_ipv4'].split('/')[0],
                                          att_path, IP_ADDRESS)

            if name != SAT:
                # IPv4 SARP
                addr = net_config['lan_ipv4'].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind('.') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        TAL_ID: instance,
                       }
                xpath = '/configuration/sarp/ipv4'
                
                table = self._configuration.get(xpath)
                new = etree.Element('terminal_v4', line)
                find = False
                for elm in table.getchildren():
                    if etree.tostring(new) == etree.tostring(elm):
                        find =  True
                        break

                if not find:
                    self._configuration.create_line(line, 'terminal_v4', xpath)

                # IPv6 SARP
                addr = net_config['lan_ipv6'].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind(':') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        TAL_ID: instance,
                       }
                xpath = '/configuration/sarp/ipv6'

                table = self._configuration.get(xpath)
                new = etree.Element('terminal_v6', line)
                find = False
                for elm in table.getchildren():
                    if etree.tostring(new) == etree.tostring(elm):
                        find =  True
                        break

                if not find:
                    self._configuration.create_line(line, 'terminal_v6', xpath)
                
                # Ethernet SARP
                if 'mac' in net_config:
                    mac = net_config['mac']
                    # we can have several MAC addresses separated by space
                    macs = mac.split(' ')
                    for mac in macs:
                        line = {'mac': mac,
                                TAL_ID: instance,
                               }
                        xpath = '/configuration/sarp/ethernet'

                        table = self._configuration.get(xpath)
                        new = etree.Element('terminal_eth', line)
                        find = False
                        for elm in table.getchildren():
                            if etree.tostring(new) == etree.tostring(elm):
                                find =  True
                                break

                        if not find:
                            self._configuration.create_line(line, 'terminal_eth', xpath)

            self.save()
        except XmlException, msg:
            self._log.error("failed to add topology for %s: %s" % (name,
                                                                   str(msg)))
        except KeyError, msg:
            self._log.error("cannot find network keys %s for topology updating "
                            "on %s" % (msg, name))
        except Exception, msg:
            self._log.error("unknown exception when trying to add topology for "
                            "%s: %s" % (name, str(msg)))

    def remove(self, name, instance):
        """ remove a host from topology configuration file """
        del self._list_host[name]
        
        try:
            xpath = "/configuration/sarp/ipv4/terminal_v4" \
                    "[@" + TAL_ID + "='%s']" % instance
            self._configuration.del_element(xpath)
            xpath = "/configuration/sarp/ipv6/terminal_v6" \
                    "[@" + TAL_ID + "='%s']" % instance
            self._configuration.del_element(xpath)
            xpath = "/configuration/sarp/ethernet/terminal_eth" \
                    "[@" + TAL_ID + "='%s']" % instance
            self._configuration.del_element(xpath)

            if name.startswith(GW):
                for section in self._configuration.get_sections():
                    for child in section.getchildren():
                        if child.tag ==  SPOT or child.tag == GW:
                            self._configuration.remove_gw("//"+section.tag, instance) 
                            break

            self.save()
        except XmlException, msg:
            self._log.error("failed to remove host with id %s in topology: %s" %
                            (instance, str(msg)))
        except Exception, msg:
            self._log.error("unknown exception when trying to remove topology "
                            "for host with instance %s: %s" % (instance,
                                                               str(msg)))



    def update(self, spot_id = "", gw_id = ""):
        sections = self._configuration.get_sections()

        tab_tal_id_spot = ["1","2","3","4","5","6"]
        tab_tal_id_gw = ["1","2","3","4","5","6"]
        tab_multicast = ["239.137.194.221",
                         "239.137.194.222",
                         "239.137.194.223",
                         "239.137.194.224",
                         "239.137.194.225",
                         "239.137.194.226",
                         "239.137.194.227",
                         "239.137.194.228",
                         "239.137.194.229",
                         "239.137.194.230",
                         "239.137.194.231",
                         "239.137.194.232"]
        tab_multicast_used = []

        spot_base = ""
        gw_base = ""
        for section in sections:
            for child in section.getchildren():
                if child.tag == SPOT:
                    # get base spot id
                    if spot_base == "":
                        spot_base = child.get(ID)
                    if gw_base == "" and child.get(GW) is not None:
                        gw_base = child.get(GW)

                    for key in self._configuration.get_keys(child):
                        for element in self._configuration.get_table_elements(key):
                            #remove used tal_id
                            if key.tag == TERMINALS:
                                if element.get(ID) in tab_tal_id_spot:
                                    tab_tal_id_spot.remove(element.get(ID))
                                    continue
                            for att in element.keys():
                                if att == IP_ADDRESS:
                                    #remove used multicast address
                                    if element.get(att) in tab_multicast:
                                        tab_multicast.remove(element.get(att))
                                        tab_multicast_used.append(element.get(att))
                                    continue
                if child.tag == GW:
                    # get base gw id
                    if gw_base == "":
                        gw_base = child.get(ID)

                    for key in self._configuration.get_keys(child):
                        for element in self._configuration.get_table_elements(key):
                            #remove used tal_id
                            if key.tag == TERMINALS:
                                if element.get(ID) in tab_tal_id_gw:
                                    tab_tal_id_gw.remove(element.get(ID))
                                    continue



        # update topology carrier value according to spor value
        for section in sections:
            for child in section.getchildren():
                gw = gw_id
                spot = spot_id
                if child.tag ==  SPOT and (spot_id == child.get(ID) or \
                   gw_id == child.get(GW)):
                    if gw == "" :
                        if child.get(GW) is not None:
                            gw = child.get(GW)
                        else:
                            gw = "0"
                    if spot == "" and child.get(ID) is not None:
                        spot = child.get(ID)
                    
                    for key in self._configuration.get_keys(child):
                        if self._configuration.is_table(key) :
                            s_id = (int(spot)-1)*10 - (int(spot_base)-1)*10 + \
                                   (int(gw))*10 - (int(gw_base))*10
                            for element in self._configuration.get_table_elements(key):
                                for att in element.keys():
                                    #update multicast address
                                    if att == IP_ADDRESS and \
                                       element.get(att) in tab_multicast_used:
                                        if len(tab_multicast) > 0:
                                            element.set(att,tab_multicast[0])
                                            tab_multicast.remove(tab_multicast[0])
                                        continue

                                    #update rrier id and port id
                                    try:
                                        if key.tag == CARRIERS:
                                            val = int(element.get(att))+s_id
                                        elif key.tag == TERMINALS and \
                                             len(tab_tal_id_spot) > 0:
                                            val = tab_tal_id_spot[0]
                                            tab_tal_id_spot.remove(tab_tal_id_spot[0])
                                    except ValueError:
                                        val = element.get(att)
                                    element.set(att,str(val))
                elif child.tag == GW and child.get(ID) == gw_id:
                    for key in self._configuration.get_keys(child):
                        for element in self._configuration.get_table_elements(key):
                            #remove used tal_id
                            if key.tag == TERMINALS  and \
                               len(tab_tal_id_gw) > 0:
                                val = tab_tal_id_gw[0]
                                tab_tal_id_gw.remove(tab_tal_id_gw[0])
                                element.set(ID,str(val))



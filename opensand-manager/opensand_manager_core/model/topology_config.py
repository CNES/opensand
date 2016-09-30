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
        self._log = manager_log
        AdvancedHostModel.__init__(self, TOPOLOGY, scenario)

    def load(self, scenario):
        """ load the global configuration """
        
        # create the host configuration directory
        conf_path = scenario
        self._scenario = scenario
        if not os.path.isdir(conf_path):
            try:
                os.makedirs(conf_path, 0755)
            except OSError, (_, strerror):
                raise ModelException("failed to create directory '%s': %s" %
                                     (conf_path, strerror))

        self._conf_file = os.path.join(conf_path, CONF_NAME)
        self._log.debug("topology load scenario %s" % self._conf_file)
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
            self.new_host(host, instance, self._list_host[host])
   
    
    def cancel(self):
        self.load(self._scenario)


    def save(self):
        """ save the configuration """
        try:
            self._configuration.write()
        except XmlException:
            raise


    def get_name(self):
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
        return []

    def get_module(self, name):
        """ get a module according to its name """
        return None

    def get_conf(self):
        return self._configuration


    def new_host(self, name, instance, net_config):
        """ Add a new host in the topology configuration file """
        # if the host is already in the topology don't add it
        self._list_host[name] = net_config
        try:
            if name.startswith(WS) and '_' in instance:
                instance = instance.split('_')[0]
            elif name.startswith(GW):
                exist = False
                for section in self._configuration.get_sections():
                    for child in section.iterchildren():
                        if (child.tag == GW and child.get(ID) == instance) or \
                           (child.tag == SPOT and child.get(GW) == instance):
                            exist =  True
                            continue
                        if child.tag ==  SPOT or child.tag == GW:
                            self._configuration.add_gw("//"+section.tag, instance) 
                            self._log.debug("topology add section %s" % section.tag)

                if not exist:
                    self.update_conf(gw_id=instance)
            self.update_host_address(name, net_config)

            if name != SAT:
                # IPv4 SARP
                addr = net_config[LAN_IPV4].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind('.') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        TAL_ID: instance,
                       }
                xpath = PATH_IPV4
                
                table = self._configuration.get(xpath)
                new = etree.Element(TERMINAL_V4, line)
                find = False
                for elm in table.getchildren():
                    if etree.tostring(new) == etree.tostring(elm):
                        find =  True
                        break

                if not find:
                    self._configuration.create_line(line, TERMINAL_V4, xpath)
                    self._log.debug("topology create line %s" % line)

                # IPv6 SARP
                addr = net_config[LAN_IPV6].split('/')
                ip = addr[0]
                net = ip[0:ip.rfind(':') + 1] + "0"
                mask = addr[1]
                line = {'addr': net,
                        'mask': mask,
                        TAL_ID: instance,
                       }
                xpath = PATH_IPV6

                table = self._configuration.get(xpath)
                new = etree.Element(TERMINAL_V6, line)
                find = False
                for elm in table.getchildren():
                    if etree.tostring(new) == etree.tostring(elm):
                        find =  True
                        break

                if not find:
                    self._configuration.create_line(line, TERMINAL_V6, xpath)
                    self._log.debug("topology create line %s" % line)
                
                # Ethernet SARP
                if 'mac' in net_config:
                    mac = net_config['mac']
                    # we can have several MAC addresses separated by space
                    macs = mac.split(' ')
                    for mac in macs:
                        line = {'mac': mac,
                                TAL_ID: instance,
                               }
                        xpath = PATH_ETERNET

                        table = self._configuration.get(xpath)
                        new = etree.Element(TERMINAL_ETH, line)
                        find = False
                        for elm in table.getchildren():
                            if etree.tostring(new) == etree.tostring(elm):
                                find =  True
                                break

                        if not find:
                            self._configuration.create_line(line, TERMINAL_ETH, xpath)
                            self._log.debug("topology create line %s" % line)

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

    def remove_host(self, name, instance):
        """ remove a host from topology configuration file """
        del self._list_host[name]
        
        try:
            xpath = PATH_TERM_V4 + "[@" + TAL_ID + "='%s']" % instance
            self._configuration.del_element(xpath)
            self._log.debug("topology del %s" % xpath)
            xpath = PATH_TERM_V6 + "[@" + TAL_ID + "='%s']" % instance
            self._configuration.del_element(xpath)
            self._log.debug("topology del %s" % xpath)
            xpath = PATH_TERM_ETH + "[@" + TAL_ID + "='%s']" % instance
            self._configuration.del_element(xpath)
            self._log.debug("topology del %s" % xpath)

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



    def update_hosts_address(self):
        """ update the hosts IP addresses in carriers
            (used for tests) """
        for host in self._list_host:
            self.update_host_address(host, self._list_host[host])
        self.save()

    def update_host_address(self, name, net_config):
        """ update an host IP address in carriers """
        try:
            if name == SAT:
                att_path = '/configuration/sat_carrier/spot/carriers/carrier' \
                           '[(@' + CARRIER_TYPE + '="' + CTRL_IN + '" or@' + \
                           CARRIER_TYPE + '="' + LOGON_IN + '" or' \
                           '@' + CARRIER_TYPE + '="' + DATA_IN_GW + '" or' \
                           '@' + CARRIER_TYPE + '="' + DATA_IN_ST + '")' \
                           'and @' + IP_MULTICAST + '="false"]'
                self._configuration.set_values(net_config['emu_ipv4'].split('/')[0],
                                               att_path, IP_ADDRESS)
                self._log.debug("topology sat ipv4 %s" % IP_ADDRESS)

            elif name.startswith(GW):
                gw_id = name[len(GW)::]
                att_path = '/configuration/sat_carrier/spot[@' + GW + '="' + gw_id + \
                           '"]/carriers/carrier[(@' + CARRIER_TYPE + '="' + LOGON_OUT + \
                           '" or @' + CARRIER_TYPE + '="'+ DATA_OUT_GW + '")' \
                           'and @' + IP_MULTICAST + '="false"]'
                self._configuration.set_values(net_config['emu_ipv4'].split('/')[0],
                                               att_path, IP_ADDRESS)
                self._log.debug("topology gw ipv4 %s" % IP_ADDRESS)
        except XmlException, msg:
            self._log.error("failed to update carriers for %s: %s" % (name,
                                                                   str(msg)))


    def update_conf(self, spot_id="", gw_id=""):
        """ update the spot and/or gw content when
            adding a new spot and/or gw """
        sections = self._configuration.get_sections()

        tab_tal_id_spot = []
        tab_tal_id_gw =  []
        tab_multicast = []
        for count in xrange(NB_MAX_TAL):
            tab_tal_id_spot.append(count)
            tab_tal_id_gw.append(count)
            add = 220 + count
            tab_multicast.append("239.137.194." + str(add))

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
                    if int(child.get(ID)) in tab_tal_id_gw:
                        tab_tal_id_gw.remove(int(child.get(ID)))

                    for key in self._configuration.get_keys(child):
                        for element in self._configuration.get_table_elements(key):
                            #remove used tal_id
                            if key.tag == TERMINALS:
                                if int(element.get(ID)) in tab_tal_id_gw:
                                    tab_tal_id_gw.remove(int(element.get(ID)))
                                    continue


        # update topology carrier value according to spot value
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

                                    #update carrier id and port id
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

        self.save()




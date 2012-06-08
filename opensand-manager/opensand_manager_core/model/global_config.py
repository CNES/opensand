#!/usr/bin/env python 
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
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
global_configuration.py - the global configuration description
"""

import os
import shutil

from opensand_manager_core.model.host_advanced import AdvancedHostModel
from opensand_manager_core.my_exceptions import XmlException, ModelException, \
                                               ConfException
from opensand_manager_core.opensand_xml_parser import XmlParser

DEFAULT_CONF = "/usr/share/opensand/core_global.conf"
GLOBAL_XSD = "/usr/share/opensand/core_global.xsd"
CONF_NAME = "core_global.conf"

class GlobalConfig(AdvancedHostModel):
    """ Global OpenSAND configuration, displayed in configuration tab """
    def __init__(self, scenario):
        AdvancedHostModel.__init__(self, 'global', 0, {}, scenario)
        self._payload_type = ''
        self._emission_std = ''
        self._dama = ''
        self._terminal_type = ''
        self._ip_options = []
        self._down_forward = {}
        self._up_return = {}
        self._frame_duration = ''

    def load(self, name, instance, ifaces, scenario):
        """ load the global configuration """
        # create the host configuration directory
        conf_path = scenario
        if not os.path.isdir(conf_path):
            try:
                os.makedirs(conf_path, 0755)
            except OSError, (errno, strerror):
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
                                     "'%s': %s" % (name, self._conf_file, msg))

        self._xsd = GLOBAL_XSD

        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
        except IOError, msg:
            raise ModelException("cannot load configuration: %s" % msg)
        except XmlException, msg:
            raise ModelException("failed to parse configuration: %s"
                                 % msg)

    def get_name(self):
        """ for compatibility with advanced dialog calls """
        return 'global'

    def get_advanced_conf(self):
        """ for compatibility with advanced dialog calls """
        return self

        
    def save(self):
        """ save the configuration """
        try:
            self._configuration.set_value(self._payload_type,
                                          "//satellite_type")
            self._configuration.set_value(self._dama, "//dama_algorithm")
            self.set_options('ip_options', self._ip_options)
            self.set_encap('down_forward_encap_schemes',
                           self._down_forward)
            self.set_encap('up_return_encap_schemes',
                           self._up_return)
            self._configuration.set_value(self._terminal_type, "//dvb_scenario")
            self._configuration.set_value(self._frame_duration,
                                          "//frame_duration")
            self._configuration.write()
        except XmlException:
            raise

    def set_payload_type(self, val):
        """ set the payload_type value """
        self._payload_type = val

    def get_payload_type(self):
        """ get the payload_type value """
        return self.get_param('satellite_type')

    def set_emission_std(self, val):
        """ set the payload_type value """
        self._emission_std = val

    def get_emission_std(self):
        """ get the payload_type value """
        if self.get_dama() != 'MF2-TDMA':
            return "DVB-RCS"
        else:
            return "DVB-S2"

    def set_dama(self, val):
        """ set the dama value """
        self._dama = val

    def get_dama(self):
        """ get the dama value """
        return self.get_param("dama_algorithm")

    def set_ip_options(self, options):
        """ set the ip_options values """
        self._ip_options = options

    def get_ip_options(self):
        """ get the ip_optionss values """
        options = self.get_options("ip_options")
        return options

    def set_up_return_encap(self, stack):
        """ set the up_return_encap_schemes values """
        self._up_return = stack

    def get_up_return_encap(self):
        """ get the up_return_encap_schemes values """
        encap = self.get_encap("up_return_encap_schemes")
        return encap

    def set_down_forward_encap(self, stack):
        """ set the down_forward_encap_schemes values """
        self._down_forward = stack 

    def get_down_forward_encap(self):
        """ get the down_forward_encap_schemes values """
        encap = self.get_encap("down_forward_encap_schemes")
        return encap

    def get_options(self, name):
        """ get the IP options names """
        table = self.get_table(name)
        if table is None:
            raise ConfException("cannot parse IP options")
        options = []
        for elt in table:
            try:
                # remove the NONE element
                if elt['name'].upper() != "NONE":
                    options.append(elt['name'])
            except KeyError, msg:
                raise ConfException("cannot get IP options: %s" % msg)
        return options

    def get_encap(self, name):
        """ get the encapsulation scheme stack """
        table = self.get_table(name)
        if table is None:
            raise ConfException("cannot parse encapsulation scheme")
        encap = {}
        for elt in table:
            try:
                encap[elt['pos']] = elt['encap']
            except KeyError, msg:
                raise ConfException("cannot get encapsulation scheme: %s" % msg)
        return encap

    def set_options(self, table_path, options):
        """ set the IP options list """
        table = self._configuration.get("//" + table_path)
        if(len(options) == 0):
            options.append("NONE")
        while len(table) < len(options):
            self._configuration.add_line(self._configuration.get_path(table))
            table = self._configuration.get("//" + table_path)
        while len(table) > len(options):
            self._configuration.remove_line(self._configuration.get_path(table))
            table = self._configuration.get("//" + table_path)

        lines = self._configuration.get_table_elements(table)
        idx = 0
        for name in options:
            path = self._configuration.get_path(lines[idx])
            self._configuration.set_value(name, path, 'name')
            idx += 1

    def set_encap(self, table_path, stack):
        """ set the encapsulation scheme table """
        table = self._configuration.get("//" + table_path)
        if(len(stack) < 1):
            raise ConfException("empty encapsulation stack received")
        while len(table) < len(stack):
            self._configuration.add_line(self._configuration.get_path(table))
            table = self._configuration.get("//" + table_path)
        while len(table) > len(stack):
            self._configuration.remove_line(self._configuration.get_path(table))
            table = self._configuration.get("//" + table_path)

        lines = self._configuration.get_table_elements(table)
        idx = 0
        for pos in sorted(stack):
            path = self._configuration.get_path(lines[idx])
            self._configuration.set_value(pos, path, 'pos')
            self._configuration.set_value(stack[pos], path, 'encap')
            idx += 1

    def set_terminal_type(self, val):
        """ set the terminal_type value """
        self._terminal_type = val

    def get_terminal_type(self):
        """ get the terminal_type value """
        return self.get_param("dvb_scenario")

    def set_frame_duration(self, val):
        """ set the frame_duration value """
        self._frame_duration = val

    def get_frame_duration(self):
        """ get the frame_duration value """
        return self.get_param("frame_duration")

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

    def enable(self, val=True):
        """ for compatibility with advanced dialog calls """
        self._enabled = val
        
    def get_table(self, name):
        """ get a list of dictionnary containing the lines content
            of the table """
        content = []
        table = self._configuration.get("//" + name)
        if table is None or not self._configuration.is_table(table):
            return None
        lines = self._configuration.get_table_elements(table)
        for line in lines:
            elt = self._configuration.get_element_content(line)
            content.append(elt)     
        return content
        

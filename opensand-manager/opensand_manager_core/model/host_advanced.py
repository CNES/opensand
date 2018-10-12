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

"""
host_advanced.py - advanced model for hosts
"""

import os
import shutil


from opensand_manager_core.utils import OPENSAND_PATH, GW, ST, GW_PHY, \
                                        GW_NET_ACC, INTERCONNECT, \
                                        UPPER_IP_ADDRESS, LOWER_IP_ADDRESS
from opensand_manager_core.model.files import Files
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_core.opensand_xml_parser import XmlParser

class AdvancedHostModel:
    """ Advanced host model"""
    def __init__(self, name, scenario):
        self._conf_file = ''
        self._xsd = ''
        self._config_view = None
        self._configuration = None
        self._enabled = True
        self._files = None
        self._name = name
        self._scenario = scenario

        self.load(self._scenario)
        self.update_conf()

    def load(self, scenario):
        """ load the advanced configuration """
        # create the host configuration directory
        self._scenario = scenario
        conf_path = os.path.join(scenario, self._name)
        if not os.path.isdir(conf_path):
            try:
                os.makedirs(conf_path, 0755)
            except OSError, (_, strerror):
                raise ModelException("failed to create directory '%s': %s" %
                                     (conf_path, strerror))

        self._conf_file = os.path.join(conf_path, 'core.conf')
        if self._name.startswith(ST):
            component = ST
        elif self._name.startswith(GW):
            component = GW
        else:
            component = self._name
        # copy the configuration template in the destination directory
        # if it does not exist
        if not os.path.exists(self._conf_file):
            try:
                shutil.copy(os.path.join(OPENSAND_PATH, "%s/core.conf" %
                                         component),
                            self._conf_file)
            except IOError, msg:
                raise ModelException("failed to copy %s configuration file in "
                                     "'%s': %s" % (self._name, self._conf_file,
                                                   msg))

        self._xsd = os.path.join(OPENSAND_PATH, "%s/core.xsd" % component)

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


    def reload_conf(self, scenario):
        """ reload the configuration file """
        self._scenario = scenario
        self._config_view = None
        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
            self._files.load(scenario, self._configuration)
        except IOError, msg:
            raise ModelException("cannot load configuration: %s" % msg)
        except XmlException, msg:
            raise ModelException("failed to parse configuration %s"
                                 % msg)

    def get_name(self):
        return self._name

    def get_configuration(self):
        """ get the configuration parser """
        return self._configuration

    def get_conf_view(self):
        """ get the configuration view """
        return self._config_view

    def set_conf_view(self, view):
        """ set the configuration view """
        self._config_view = view

    def is_enabled(self):
        """ check if host is enabled """
        return self._enabled

    def enable(self):
        """ enable host """
        self._enabled = True

    def disable(self):
        """ disable host """
        self._enabled = False

    def get_params(self, name):
        """ get a parameter in the XML configuration file """
        val = []
        keys = self._configuration.get_all("//" + name)
        for key in keys:
            try:
                val.append(self._configuration.get_value(key))
            except:
                pass
        return val

    def get_stack(self, name, key):
        """ get a lan adaptation stack """
        table = self.get_table(name)
        if table is None:
            raise ModelException("cannot parse stack %s" % name)
        stack = {}
        for elt in table:
            try:
                stack[elt['pos']] = elt[key]
            except KeyError, msg:
                raise ModelException("cannot get scheme for %s: %s" % (name,
                                                                       msg))
        return stack

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

    def set_stack(self, table_path, stack, key):
        """ set the encapsulation scheme table """
        table = self._configuration.get("//" + table_path)
        if(len(stack) < 1):
            raise ModelException("empty stack received for %s" % table_path)
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
            self._configuration.set_value(stack[pos], path, key)
            idx += 1
        self._configuration.write()

    def set_interconnect(self, component, ip_address):
        """ set the corresponding interconnect configurations """
        key = LOWER_IP_ADDRESS if component == GW_PHY else UPPER_IP_ADDRESS
        path = "//" + INTERCONNECT + "/" + key
        self._configuration.set_value(ip_address, path)
        self._configuration.write()

    def get_files(self):
        """ get the files """
        return self._files

    def get_debug(self):
        """ get the debug configuration """
        return self._configuration.get("//debug")


    def update_conf(self, spot_id="", gw_id=""):
        """ update the spot and/or gw content when
            adding a new spot and/or gw and the lines in files """
        # get elements of type file
        nodes = self._configuration.get_file_elements("element")
        for node in nodes:
            elems = self._configuration.get_all("//%s" % node)
            for elem in elems:
                path = self._configuration.get_path(elem)
                filename = self._configuration.get_value(elem)
                filename = self._configuration.adapt_filename(filename, elem)
                self._configuration.set_value(filename, path)

        # get attributes of type file
        nodes = self._configuration.get_file_elements("attribute")
        for node in nodes:
            elems = self._configuration.get_all("//*[@%s]" % node)
            for elem in elems:
                filenames = self._configuration.get_element_content(elem)
                filename = filenames[node]
                path = self._configuration.get_path(elem)
                # get line ID in path
                pos = path.rfind('[')
                line = path[pos:].strip('[]')
                filename = self._configuration.adapt_filename(filename, elem, line)
                self._configuration.set_value(filename, path, node)

        try:
            self._configuration.write()
            self._files.load(self._scenario, self._configuration)
        except XmlException:
            raise


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

from opensand_manager_core.utils import OPENSAND_PATH, TOPOLOGY
from opensand_manager_core.model.host_advanced import AdvancedHostModel
from opensand_manager_core.model.files import Files
from opensand_manager_core.my_exceptions import XmlException, ModelException
from opensand_manager_core.opensand_xml_parser import XmlParser

DEFAULT_CONF = OPENSAND_PATH + "topology.conf"
TOPOLOGY_XSD = OPENSAND_PATH + "topology.xsd"
CONF_NAME = "topology.conf"

class TopologyConfig(AdvancedHostModel):
    """ OpenSAND Topology configuration, displayed in configuration tab """
    def __init__(self, scenario):
        AdvancedHostModel.__init__(self, 'topology', scenario)
        self._modules = []

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

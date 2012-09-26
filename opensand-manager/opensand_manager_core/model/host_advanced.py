#!/usr/bin/env python2
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
host_advanced.py - advanced model for hosts
"""

import os
import shutil

from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_core.opensand_xml_parser import XmlParser

class AdvancedHostModel:
    """ Advanced host model"""
    def __init__(self, name, instance, ifaces, scenario):
        self._conf_file = ''
        self._xsd = ''
        self._config_view = None
        self._configuration = None
        self._enabled = True
        self.load(name, instance, ifaces, scenario)

    def load(self, name, instance, ifaces, scenario):
        """ load the advanced configuration """
        # create the host configuration directory
        conf_path = os.path.join(scenario, name)
        if not os.path.isdir(conf_path):
            try:
                os.makedirs(conf_path, 0755)
            except OSError, (errno, strerror):
                raise ModelException("failed to create directory '%s': %s" %
                                     (conf_path, strerror))

        self._conf_file = os.path.join(conf_path, 'core.conf')
        if name.startswith('st'):
            component = 'st'
        else:
            component = name
        # copy the configuration template in the destination directory
        # if it does not exist
        if not os.path.exists(self._conf_file):
            try:
                shutil.copy("/usr/share/opensand/%s/core.conf" % component,
                            self._conf_file)
            except IOError, msg:
                raise ModelException("failed to copy %s configuration file in "
                                     "'%s': %s" % (name, self._conf_file, msg))

        self._xsd = "/usr/share/opensand/%s/core.xsd" % component

        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
        except IOError, msg:
            raise ModelException("cannot load configuration: %s" % msg)
        except XmlException, msg:
            raise ModelException("failed to parse configuration: %s"
                                 % msg)

    def reload_conf(self):
        """ reload the configuration file """
        self._config_view = None
        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
        except IOError, msg:
            raise ModelException("cannot load configuration: %s" % msg)
        except XmlException, msg:
            raise ModelException("failed to parse configuration %s"
                                 % msg)

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



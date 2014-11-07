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
tool.py - Tool model for OpenSAND manager
"""

import os
import glob
import shutil


from opensand_manager_core.utils import OPENSAND_PATH
from opensand_manager_core.model.files import Files
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_core.opensand_xml_parser import XmlParser

class ToolModel:
    """ tool model """
    def __init__(self, name, host_name, compo):
        self._name = name
        self._host = host_name
        self._compo = compo
        self._state = None
        self._selected = None

        self._bin_path = ''
        self._conf_file = ''
        self._dest_conf_path = ''
        self._xsd = ''

        self._bin = None
        self._command = None
        self._descr = None
        self._configuration = None

        self._config_view = None
        self._files = None

    def load(self, scenario):
        """ load the tools elements from files """
        # get description
        desc_path = os.path.join(OPENSAND_PATH, "/tools/%s/description" %
                                 self._name)
        try:
            size = os.path.getsize(desc_path)
            with open(desc_path) as desc_file:
                self._descr = desc_file.read(size)
        except (OSError, IOError), (_, strerror):
            raise ModelException("cannot read %s description file %s (%s)" %
                                 (self._name, desc_path, strerror))

        # parse binary
        self._bin_path = os.path.join(OPENSAND_PATH, "/tools/%s/%s/binary"
                                      % (self._name, self._compo))
        try:
            with open(self._bin_path) as bin_file:
                self._command = bin_file.readline()
                self._bin = self._command.split()[0]
        except (OSError, IOError), (_, strerror):
            raise ModelException("cannot read %s binary file %s (%s)" %
                                 (self._name, self._bin_path, strerror))

        self.update(scenario)

        # everything went fine
        self._state = False
        self._selected = False

    def update(self, scenario):
        """ update the tools path """
        # get configuration
        self._conf_file = os.path.join(scenario, "tools/%s/%s/" %
                                       (self._name, self._host))
        # create the directory if it does not exist
        if not os.path.isdir(self._conf_file):
            try:
                os.makedirs(self._conf_file, 0755)
            except OSError, (_, strerror):
                raise ModelException("cannot create directory '%s': %s" %
                                     (self._conf_file, strerror))
        self._conf_file = os.path.join(self._conf_file, 'config')

        if not os.path.exists(self._conf_file):
            try:
                default_path = os.path.join(OPENSAND_PATH, "/tools/%s/%s/config"
                                            % (self._name, self._compo))
                shutil.copy(default_path, self._conf_file)
            except IOError, (_, strerror):
                raise ModelException("cannot copy %s configuration from "
                                     "'%s' to '%s': %s" % (self._name,
                                     default_path, self._conf_file, strerror))

        self._xsd = os.path.join(OPENSAND_PATH, "/tools/%s/%s/config.xsd" %
                                 (self._name, self._compo))

        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
            if self._files is None:
                self._files = Files(self._host, self._configuration, scenario)
            else:
                self._files.load(scenario, self._configuration)
        except IOError, msg:
            raise ModelException("cannot load %s configuration:\n\t%s" %
                                 (msg, self._name))
        except XmlException, msg:
            raise ModelException("failed to parse %s configuration file:\n\t%s"
                                 % (self._name, msg))

    def get_host(self):
        """ get the host to which this tool belong """
        return self._host

    def reload_conf(self, scenario):
        """ reload the configuration file """
        self._config_view = None
        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
            self._files.load(scenario, self._configuration)
        except IOError, msg:
            self._configuration = None
            self._state = None
            self._selected = None
            self._files = None
            raise ModelException("cannot load %s configuration:\n\t%s" %
                                 (msg, self._name))
        except XmlException, msg:
            self._configuration = None
            self._state = None
            self._selected = None
            self._files = None
            raise ModelException("failed to parse %s configuration file:\n\t%s"
                                 % (self._name, msg))

    def get_name(self):
        """ get the tool name """
        return self._name

    def get_command(self):
        """ get the command line to start tools """
        return self._command

    def get_binary(self):
        """ get the binary path """
        return self._bin

    def get_description(self):
        """ get the tool description """
        return self._descr

    def get_config_parser(self):
        """ get the configuration parser """
        return self._configuration

    def get_state(self):
        """ get the tool state """
        return self._state

    def is_selected(self):
        """ get the tool selected status """
        return self._selected

    def set_state(self, state):
        """ set the tool state """
        self._state = state

    def set_selected(self, status=True):
        """ set the selected status """
        self._selected = status

    def get_conf_src(self):
        """ get the configuration file """
        return self._conf_file

    def get_binary_path(self):
        """ get the binary path """
        return self._bin_path

    def get_conf_dst(self):
        """ get the configuration destination """
        return "/etc/opensand/tools/%s.conf" % self._name

    def get_conf_view(self):
        """ get the configuration view """
        return self._config_view

    def set_conf_view(self, view):
        """ set the configuration view """
        self._config_view = view

    def get_conf_files(self):
        """ get the configuration files """
        conf_directory = os.path.join(OPENSAND_PATH, "/tools/%s/%s/" %
                                      (self._name, self._compo))
        conf_files = {}
        for extension in ["*.xml", "*.conf", "*.ini"]:
            for conf_file in glob.glob(conf_directory + extension):
                conf_files[conf_file] = "/etc/opensand/tools/" + \
                                        os.path.basename(conf_file)

        return conf_files

    def get_files(self):
        """ get the files """
        return self._files



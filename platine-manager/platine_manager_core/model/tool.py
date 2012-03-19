#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
#
#
# This file is part of the Platine testbed.
#
#
# Platine is free software : you can redistribute it and/or modify it under the
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
tool.py - Tool model for Platine manager
"""

import ConfigParser
import os
import glob
import shutil

from platine_manager_core.my_exceptions import ModelException

class ToolModel:
    """ tool model """
    def __init__(self, name, host_name, compo):
        self._name = name
        self._host = host_name
        self._compo = compo
        self._state = None
        self._selected = None

        self._bin_path = ''
        self._conf_path = ''
        self._dest_conf_path = ''

        self._bin = None
        self._command = None
        self._descr = None
        self._conf = None

        self._config_view = None

    def load(self, scenario):
        """ load the tools elements from files """
        # get description
        desc_path = "/usr/share/platine/tools/%s/description" % self._name
        try:
            size = os.path.getsize(desc_path)
            with open(desc_path) as desc_file:
                self._descr = desc_file.read(size)
        except (OSError, IOError), (errno, strerror):
            raise ModelException("cannot read %s description file %s (%s)" %
                                 (self._name, desc_path, strerror))

        # parse binary
        self._bin_path = "/usr/share/platine/tools/%s/%s/binary" % \
                         (self._name, self._compo)
        try:
            with open(self._bin_path) as bin_file:
                self._command = bin_file.readline()
                self._bin = self._command.split()[0]
        except (OSError, IOError), (errno, strerror):
            raise ModelException("cannot read %s binary file %s (%s)" %
                                 (self._name, self._bin_path, strerror))

        self.update(scenario)

        # everything went fine
        self._state = False
        self._selected = False

    def update(self, scenario):
        """ update the tools path """
        # get configuration
        self._conf_path = os.path.join(scenario, "tools/%s/%s/" %
                                       (self._name, self._host))
        # create the directory if it does not exist
        if not os.path.isdir(self._conf_path):
            try:
                os.makedirs(self._conf_path, 0755)
            except OSError, (errno, strerror):
                raise ModelException("cannot create directory '%s': %s" %
                                     (self._conf_path, strerror))
        self._conf_path = os.path.join(self._conf_path, 'config')

        if not os.path.exists(self._conf_path):
            try:
                default_path = "/usr/share/platine/tools/%s/%s/config" % \
                               (self._name, self._compo)
                shutil.copy(default_path, self._conf_path)
            except IOError, (errno, strerror):
                raise ModelException("cannot copy %s configuration from "
                                     "'%s' to '%s': %s" % (self._name,
                                     default_path, self._conf_path, strerror))

        try:
            self._conf = ConfigParser.SafeConfigParser()
            if len(self._conf.read(self._conf_path)) == 0:
                raise ModelException("cannot read %s configuration file %s" %
                                     (self._name, self._conf_path))
        except ConfigParser.Error, msg:
            raise ModelException("cannot read %s configuration file %s: %s" %
                                 (self._name, self._conf_path, msg))

    def reload_conf(self):
        """ reload the configuration file """
        self._config_view = None
        read = 0
        try:
            self._conf = ConfigParser.SafeConfigParser()
            read = len(self._conf.read(self._conf_path))
        except ConfigParser.Error:
            pass
        finally:
            if read == 0:
                self._conf = None
                self._state = None
                self._selected = None

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
        return self._conf

    def save_config(self):
        """ save the configuration stored in config_parser """
        try:
            with open(self._conf_path, 'w') as conf_file:
                self._conf.write(conf_file)
        except IOError:
            raise

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
        return self._conf_path

    def get_binary_path(self):
        """ get the binary path """
        return self._bin_path

    def get_conf_dst(self):
        """ get the configuration destination """
        return "/etc/platine/tools/%s.conf" % self._name

    def get_conf_view(self):
        """ get the configuration view """
        return self._config_view

    def set_conf_view(self, view):
        """ set the configuration view """
        self._config_view = view

    def get_conf_files(self):
        """get the configuration files."""
        conf_directory = "/usr/share/platine/tools/%s/%s/" % (self._name, self._compo)
        conf_files = {}
        for extension in ["*.xml", "*.conf", "*.ini"]:
            for conf_file in glob.glob(conf_directory + extension):
                conf_files[conf_file] = "/etc/platine/tools/" + \
                                        os.path.basename(conf_file)

        return conf_files

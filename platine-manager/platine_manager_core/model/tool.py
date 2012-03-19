#!/usr/bin/env python
# -*- coding: utf-8 -*-
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
        self._deploy = "/usr/share/platine/tools/%s/%s/deploy.ini" % \
                       (name, compo)
        self._files = "/usr/share/platine/tools/%s/%s/files" % (name, compo)

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

        # check if deploy.ini and files are present
        if not os.path.exists(self._files):
            self._files = None
        if not os.path.exists(self._deploy):
            self._deploy = None

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

    def get_deploy_file(self):
        """ get the deploy file """
        return self._deploy

    def get_files_file(self):
        """ get the file containing the files to copy on startup """
        return self._files

    def get_conf_files(self):
        """get the configuration files."""
        conf_directory = "/usr/share/platine/tools/%s/%s/" % (self._name, self._compo)
        conf_files = {}
        for extension in ["*.xml", "*.conf", "*.ini"]:
            for conf_file in glob.glob(conf_directory + extension):
                conf_files[conf_file] = "/etc/platine/tools/" + \
                                        os.path.basename(conf_file)

        return conf_files

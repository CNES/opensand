##!/usr/bin/env python2
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
files.py - Specific elements for file handling in configurations
"""

import os
import shutil
import hashlib

from opensand_manager_core.utils import OPENSAND_PATH, GLOBAL
from opensand_manager_core.my_exceptions import ModelException

def get_md5(filename):
    """ get the md5sum on a file """
    with open(filename, 'r')  as filecontent:
        md5 = hashlib.md5()
        md5.update(filecontent.read())
        return md5.digest()


# About file elements:
# the configuration contains the destination file that will be read by
# the program, this cannot be modified from manager
# the xsd file contains two elements, default and source:
# - default is the default path for this file (relative to
#   OPENSAND_FOLDER), it is used to copy the file when creating a
#   scenario
#  - source is the path relative to the scenario folder that will be
#    used when deploying the file on host
# Both these values cannot be modified as well, but the source file can
# be edited or changed in the advance configuration. When the source is
# changed, the source parameter is not modified but we simply overwritte
# the file with the one given by the user
class Files(object):
    """ files """
    def __init__(self, host_name, config, scenario):
        self._host_name = host_name.lower()
        self._configuration = config

        # [(default, source)]
        self._file_paths = []
        # {sim file xpath: source_file}
        self._file_sources = {}
        # {sim file xpath: md5sum}
        self._md5 = {}
        self._first = True

        self.load(scenario)
        for xpath in self._file_sources:
            # force initial copy
            self._md5[xpath] = 0


    def load(self, scenario, configuration=None):
        """ load the files for current scenario """
        if configuration is not None:
            self._configuration = configuration
        if self._host_name != GLOBAL:
            scenario = os.path.join(scenario, self._host_name)

        # handle files elements
        self._file_paths = self._configuration.get_file_paths()
        self._file_sources = self._configuration.get_file_sources()
        for (default, source) in self._file_paths:
            if default is None or source is None:
                raise ModelException("Missing default and source parameters "
                                     "for some files in %s XSD" %
                                     self._host_name)
            abs_default = os.path.join(OPENSAND_PATH, default)
            abs_source = os.path.join(scenario, source)
            if not os.path.exists(os.path.dirname(abs_source)):
                os.makedirs(os.path.dirname(abs_source))
            if not os.path.exists(abs_source):
                shutil.copy(abs_default, abs_source)

    def update(self, changed, scenario):
        """ update changed files """
        if self._host_name != GLOBAL:
            scenario = os.path.join(scenario, self._host_name)

        # copy the new file into the source
        copied = []
        for xpath in changed:
            new_source = changed[xpath]
            try:
                source = os.path.join(scenario,
                                      self._file_sources[xpath])
            except KeyError:
                continue
            if new_source != source:
                shutil.copy(new_source, source)
            copied.append(xpath)
        for xpath in copied:
            del changed[xpath]

    def get_modified(self, scenario):
        """
        get the tuples source, destination of the files that were modified
        """
        if self._host_name != GLOBAL:
            scenario = os.path.join(scenario, self._host_name)

        deploy = []
        print scenario
        for xpath in self._file_sources:
            if not xpath in self._md5:
                self._md5[xpath] = 0
                
            old_hash = self._md5[xpath]
            new_hash = get_md5(os.path.join(scenario,
                                            self._file_sources[xpath]))
            if old_hash != new_hash:
                src = self._file_sources[xpath]
                src = os.path.join(scenario, src)
                if not "@" in xpath:
                    xpath += '/text()'
                dest = self._configuration.get(xpath)
                deploy.append((src, dest))
        return deploy

    def get_all(self, scenario):
        """
        get all the tuples source, destination
        """
        if self._host_name != GLOBAL:
            scenario = os.path.join(scenario, self._host_name)

        deploy = []
        for xpath in self._file_sources:
            src = self._file_sources[xpath]
            src = os.path.join(scenario, src)
            if not "@" in xpath:
                xpath += '/text()'
            dest = self._configuration.get(xpath)
            deploy.append((src, dest))
        return deploy

    def set_modified(self, scenario):
        """
        the files were modified, update the md5sums
        """
        if self._host_name != GLOBAL:
            scenario = os.path.join(scenario, self._host_name)

        for xpath in self._file_sources:
            self._md5[xpath]= get_md5(os.path.join(scenario,
                                                   self._file_sources[xpath]))
        self._first = False

    def is_first(self):
        """
        check if this is the first deployment
        """
        return self._first

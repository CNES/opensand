#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 TAS
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
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>

"""
host.py - controller that configure, install, start, stop
          and get status of OpenSAND processes on a specific host
"""

import threading
import socket
import thread
import os
import ConfigParser
import tempfile
import time

from opensand_manager_core.my_exceptions import CommandException
from opensand_manager_core.controller.stream import Stream
from opensand_manager_core.utils import ST, SAT, GW, GW_types
from opensand_manager_core.controller.machine import MachineController

CONF_DESTINATION_PATH = '/etc/opensand/'
START_DESTINATION_PATH = '/var/cache/sand-daemon/start.ini'
START_INI = 'start.ini'
DATA_END = 'DATA_END\n'

#TODO factorize
class HostController:
    """ controller which implements the client that connects in order to get
        program states on distant host and the client that sends command to
        the distant host """
    def __init__(self, host_model, manager_log,
                 cache_dir='/var/cache/sand-daemon/'):
        self._host_model = host_model
        self._log = manager_log
        self._machines = {} # we want them ordered

    def close(self):
        """ close the host connections """
        for m in self._machines:
            self._machines[m].close()

    def get_name(self):
        """ get the name of host """
        return self._host_model.get_name().upper()
    
    def start_stop(self, command):
        """ send the start or stop command to host server """
        for m in self._machines:
            self._machines[m].start_stop(command)
    
    def disable(self):
        """ disable the host """
        self._host_model.enable(False)

    def get_interface_type(self):
        """ get the type of interface according to the stack """
        return self._host_model.get_interface_type()

    def get_deploy_files(self):
        """ get the files to deploy """
        return self._host_model.get_deploy_files()

    def first_deploy(self):
        """ check if this is the first deploy """
        return self._host_model.first_deploy()
    
    def new_machine(self, cache):
        """ adds new machine from model """
        # there should be only one new machine, otherwise, we'll
        # use the wrong 'cache' path
        host_model_machines = self._host_model.get_machines()
        for m in host_model_machines:
            if m not in self._machines:
                self._machines[m] = MachineController(
                                            host_model_machines[m],
                                            self._log, cache)

    def del_machine_by_component(self, machine):
        """ removes and closes machine """
        if machine in self._machines:
            self._machines[machine].close()
            del self._machines[machine]

    def del_machine_by_name(self, machine):
        """ removes and closes machine """
        for name, mach in self._machines.iteritems():
            if mach._machine_model.get_name().lower() == machine:
                mach.close()
                del self._machines[name]
                break

    def deploy(self, conf):
        """ close the host connections """
        for m in self._machines:
            try:
                self._machines[m].deploy(conf)
            except Exception as ex:
                raise Exception("Unexpected error when deploying machine %s: %s"
                                % (m, str(ex)) )

    def has_machine_by_component(self, machine):
        return (machine in self._machines)

    def has_machine_by_name(self, machine):
        found = False
        for m in self._machines:
            if self._machines[m]._machine_model.get_name().lower() == machine:
                found = True
                break
        return found

    def get_machines(self):
        """ return machines """
        return self._machines

    def get_ordered_machines(self):
        """ return list with machines in order """
        ordered_list = []
        for machine in self._machines:
            if 'lan' in self._machines[machine].get_name():
                ordered_list = [self._machines[machine]] + ordered_list
            else:
                ordered_list = ordered_list + [self._machines[machine]]
        return ordered_list

#!/usr/bin/env python2
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
probe_controller.py - the probe controller
"""

import threading
import socket
import struct

from opensand_manager_core.my_exceptions import ProbeException
from opensand_manager_gui.view.popup.infos import error_popup


class ProbeController(threading.Thread):
    """ Thread which get the probes from probe controller """
    def __init__(self, model, manager_log):
        threading.Thread.__init__(self)
        self._stop = threading.Event()

        self._probes = model
        self._log = manager_log

        # create the probe list
        self._component_list = ['GW', 'SAT', 'ST']
        self._cmpt = 0 # used for test
        self._test = False

        #TODO use socket server ?
        # initialize the socket
        try :
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # bind the socket to a public host and a well-known port
#TODO read port from conf
            self._socket.bind(('', 2613))
            self._socket.settimeout(1)
        except socket.error , strerror :
            self._log.error("error on probe socket: " + str(strerror))
            self._socket = None

    def close(self):
        """ terminate the thread """
        self._log.debug("Probe Controller: close")
        self._stop.set()
        if self._socket != None:
            self._socket.close()
        self._log.debug("Probe Controller: closed")

    def run(self):
        """ run the thread loop """
        while not self._stop.is_set():
            if self._test == True:
                pass
#               # used for testing: no data is collected from the probe server
#               # but is generated directly for only one stat
#               i = self.find_stat('GW', 'Carrier_composition', 'DRA_scheme_1')
#               if (self._cmpt < 10):
#                   i.add(self._cmpt, (1.0 + self._cmpt)*pylab.sin(self._cmpt))
#               else:
#                   i.add(self._cmpt, (40.0 - self._cmpt)*pylab.sin(self._cmpt))
#
#               self._cmpt=self._cmpt+0.1
            else:
                # get probe value from the socket
                try:
                    self.get_probe_value()
                except ProbeException as error:
                    self._stop.set()
                    error_popup(error.value)

    def get_probe_value(self):
        """ get the probes value """
        if self._socket == None:
            raise ProbeException("cannot access probe socket")

        try:
            # accept connections from outside
            data = self._socket.recv(32)
        except socket.timeout:
            pass
        except socket.error, strerror:
            raise ProbeException("error reading on probe socket: " +
                                 str(strerror))
        else:
            if len(data) < 16:
                raise ProbeException("received %d bytes (at least 16 expected)"
                                     % len(data))

            val = struct.unpack("!BBHiif", data)
            component = val[0] >> 4
            instance = val[0] & 15
            if (val[2] == 1):
                val = struct.unpack("!BBHiff", data)

            # add the value in the corresponding stat
            cmpt = self._component_list[component]
            if cmpt == 'ST':
                cmpt = cmpt + str(instance)

            self._probes.add_value(cmpt, val[1], val[3], val[4], val[5])

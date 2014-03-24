#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
tcp_server.py - server that get OpenSAND commands
"""

import SocketServer
import select
import logging
import threading

from opensand_daemon.my_exceptions import Timeout
from opensand_daemon.stream import TIMEOUT

#macros
LOGGER = logging.getLogger('sand-daemon')

class Plop(SocketServer.TCPServer):
    """ The TCP socket server with reuse address to true """

    allow_reuse_address = True

    def __init__(self, *args):
        SocketServer.TCPServer.__init__(self, *args)

    def run(self):
        """ run the TCP server """
        try:
            self.serve_forever()
        except Exception:
            raise

    def stop(self):
        """ stop the TCP server """
        MyTcpHandler._stop.set()
        self.shutdown()


class MyTcpHandler(SocketServer.StreamRequestHandler):
    """ The RequestHandler class for the TCP server """

    # line buffered
    rbufsize = 1
    wbufsize = 0
    # the stop event is handled by the state server, if the state server is not
    # started there is no reason to get another connection with the manager so
    # for each thread in a MyTcpHandler class you should watch the _stop event
    _stop = threading.Event()

    def setup(self):
        """ the function called when MyTcpHandler is created """
        self.connection = self.request
        self.rfile = self.connection.makefile('rb', self.rbufsize)
        self.wfile = self.connection.makefile('wb', self.wbufsize)
        self._data = ''

    def finish(self):
        """ the function called when handle returns """
        LOGGER.info("close connection")
        if not self.wfile.closed:
            self.wfile.flush()
        self.wfile.close()
        self.rfile.close()

    # TODO use a pipe as for output_handler to break the select to avoid TIMEOUT
    # and loop
    def read_data(self, timeout=True):
        """ read data on socket.
            Can raise Timeout and EOFError exceptions """
        if timeout:
            inputready, out, err = select.select([self.rfile], [], [], TIMEOUT)
            if(len(inputready) == 0):
                LOGGER.warning("timeout when trying to read")
                raise Timeout
        else:
            # do not block on socket polling
            while not MyTcpHandler._stop.is_set():
                inputready, out, err = select.select([self.rfile], [], [], 1)
                if(len(inputready) == 0):
                    MyTcpHandler._stop.wait(1.0)
                else:
                    break

        if not MyTcpHandler._stop.is_set():
            self._data = self.rfile.readline().strip()
            if (self._data == ''):
                LOGGER.info("distant socket is closed")
                raise EOFError

# -*- coding: utf8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 CNES
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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
Main file for the OpenSAND collector.
"""

from messages_handler import MessagesHandler
from optparse import OptionParser
from probes_manager import HostManager
from service_handler import ServiceHandler
from transfer_server import TransferServer
import gobject
import logging
import socket
import sys


class OpenSandCollector(object):
    """
    This class serves as the entry point for the collector daemon.
    """

    def __init__(self):
        self.sock = None
        self.host_manager = HostManager()

    def run(self):
        """
        Start the collector
        """

        parser = OptionParser()
        parser.set_defaults(debug=False)
        parser.add_option("-t", "--service_type", dest="service_type",
            help="OpenSAND service type (default: _opensand._tcp)")
        parser.add_option("-d", action="store_true", dest="debug",
            help="Show debug messages")
        (options, args) = parser.parse_args()

        service_type = options.service_type or "_opensand._tcp"
        level = logging.DEBUG if options.debug else logging.INFO

        logging.basicConfig(level=level)

        gobject.threads_init()  # Necessary for the transfer_server thread
        main_loop = gobject.MainLoop()

        try:
            with MessagesHandler(self) as msg_handler:
                port = msg_handler.get_port()
                with TransferServer(self.host_manager) as transfer_server:
                    trsfer_port = transfer_server.get_port()
                    with ServiceHandler(self, port, trsfer_port, service_type):
                        main_loop.run()
        finally:
            self.host_manager.cleanup()

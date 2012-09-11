# -*- coding: utf8 -*-

"""
Main file for the OpenSAND collector.
"""

from messages_handler import MessagesHandler
from probes_manager import HostManager
from service_handler import ServiceHandler
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

        logging.basicConfig(level=logging.DEBUG)

        main_loop = gobject.MainLoop()

        with MessagesHandler(self) as msg_handler:
            with ServiceHandler(self, msg_handler.get_port()):
                main_loop.run()


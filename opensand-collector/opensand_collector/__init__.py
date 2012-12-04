#!/usr/bin/env python2
# -*- coding: utf8 -*-

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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
__init__.py -  Main file for the OpenSAND collector.
"""

from opensand_collector.messages_handler import MessagesHandler
from opensand_collector.manager import HostManager
from opensand_collector.service_handler import ServiceHandler
from opensand_collector.transfer_server import TransferServer
from opensand_collector.syslog_handler import SysLogHandler, syslog
from optparse import OptionParser
import errno
import fcntl
import logging
import os
import signal
import sys


LOGGER = logging.getLogger('sand-collector')


def fail(message, *args):
    """
    Report a startup error and exits.
    """
    LOGGER.error(message % args)
    sys.exit(1)


def read_pid_file(path):
    """
    Returns the current content of the PID file, or 0 if it does not exist.
    """
    try:
        with open(path) as pid_file:
            try:
                return int(pid_file.read())
            except ValueError:
                return 0

    except IOError:
        return 0
    
def remove_pid(path):
    """
    Remove the pid file
    """
    try:
        os.remove(path)
    except OSError, msg:
        LOGGER.warning("cannot remove pid file")
        pass


class OpenSandCollector(object):
    """
    This class serves as the entry point for the collector daemon.
    """

    def __init__(self):
        self._host_manager = HostManager()

    def run(self):
        """
        Start the collector
        """
        parser = OptionParser()
        parser.set_defaults(debug=False, background=False, kill=False)
        parser.add_option("-t", "--service_type", dest="service_type",
                          default='_opensand._tcp', action="store",
                          help="OpenSAND service type (default: _opensand._tcp)")
        parser.add_option("-i", "--iface", dest="iface",
                          default='', action="store",
                          help="Interface for service publishing (default: all)")
        parser.add_option("-v", "--verbose", action="store_true",
                          dest="verbose", default=False,
                          help = "Print more informations")
        parser.add_option("-d", "--debug", action="store_true", dest="debug",
                          default=False, help="Show debug messages")
        parser.add_option("-q", "--quiet", action="store_true", dest="quiet",
                          default=False, help="Stop printing logs in console")
        parser.add_option("-b", "--background", action="store_true",
                          dest="background",
                          help="Run in background as opensand user")
        parser.add_option("-p", "--pid", dest="pid",
                          default='/var/run/sand-collector/pid',
                          action="store",
                          help="Specify the file to save sand-collector PID")
        parser.add_option("-k", "--kill", action="store_true", dest="kill",
                          help="Kill a background collector instance")
        (options, _args) = parser.parse_args()

        service_type = options.service_type
        iface = options.iface
        pid_path = options.pid
        
        # Logging configuration
        if options.background or options.quiet:
            log_handler = SysLogHandler('sand-collector', syslog.LOG_PID,
                                         syslog.LOG_DAEMON)
            LOGGER.addHandler(log_handler)

        # Print logs in terminal for debug
        if not options.quiet:
            log_handler = logging.StreamHandler(sys.stdout)
            formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)-5s "
                                          "- %(message)-50s [%(filename)s:%(lineno)d]")
            log_handler.setFormatter(formatter)
            LOGGER.addHandler(log_handler)

        LOGGER.setLevel(logging.WARNING)
        if options.debug:
            LOGGER.setLevel(logging.DEBUG)
        elif options.verbose:
            LOGGER.setLevel(logging.INFO)

        LOGGER.error("ICI")
        if options.kill:
            pid = read_pid_file(pid_path)
            if pid == 0:
                fail("The collector does not seem to be running (no PID).")

            try:
                os.kill(pid, signal.SIGTERM)
            except OSError, msg:
                remove_pid(pid_path)
                fail("Cannot kill sand-collector: " + str(msg))

            os._exit(0)

        if options.background:
            try:
                if os.path.exists(pid_path):
                    fail("pid already exists")
                bg_fd = os.open(pid_path, os.O_WRONLY | os.O_CREAT)
                fcntl.flock(bg_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except OSError, err:
                fail(str(err))
            except IOError, err:
                if err.errno == errno.EACCES or err.errno == errno.EAGAIN:
                    fail("The collector seem to be already running in the "
                         "background.")
                else:
                    fail(str(err))

            pid = os.fork()
            if pid:
                os.write(bg_fd, str(pid))
                os._exit(0)

            null = open(os.path.devnull, 'r+')
            sys.stdin = sys.stdout = sys.stderr = null

        try:
            with MessagesHandler(self._host_manager) as msg_handler:
                port = msg_handler.get_port()
                with TransferServer(self._host_manager) as transfer_server:
                    trsfer_port = transfer_server.get_port()
                    with ServiceHandler(self._host_manager, port, trsfer_port,
                                        service_type, iface) as service:
                        def handler(_sig, _frame):
                            """
                            SIGTERM handler
                            """
                            logging.info("SIGTERM caught, quitting.")
                            service.stop()
                        signal.signal(signal.SIGTERM, handler)
                        service.run()
        finally:
            self._host_manager.cleanup()
            if options.background:
                remove_pid(pid_path)

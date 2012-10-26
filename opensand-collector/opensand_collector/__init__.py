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

from opensand_collector.messages_handler import MessagesHandler
from opensand_collector.probes_manager import HostManager
from opensand_collector.service_handler import ServiceHandler
from opensand_collector.transfer_server import TransferServer
from optparse import OptionParser
import errno
import fcntl
import gobject
import logging
import os
import pwd
import signal
import socket
import sys


PID_PATH = "/var/run/opensand-collector.pid"
LOG_PATH = "/var/log/opensand/collector.log"


def fail(message, *args):
    """
    Report a startup error and exits.
    """
    
    sys.exit(message % args)


def read_pid_file():
    """
    Returns the current content of the PID file, or 0 if it does not exist.
    """
    
    try:
        with open(PID_PATH) as pid_file:
            try:
                return int(pid_file.read())
            except ValueError:
                return 0
            
    except IOError:
        return 0


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
        parser.set_defaults(debug=False, background=False, kill=False)
        parser.add_option("-t", "--service_type", dest="service_type",
            help="OpenSAND service type (default: _opensand._tcp)")
        parser.add_option("-d", "--debug", action="store_true", dest="debug",
            help="Show debug messages")
        parser.add_option("-b", "--background", action="store_true",
            dest="background", help="Run in background as opensand user")
        parser.add_option("-k", "--kill", action="store_true", dest="kill",
            help="Kill a background collector instance")
        (options, _args) = parser.parse_args()

        service_type = options.service_type or "_opensand._tcp"
        level = logging.DEBUG if options.debug else logging.INFO

        gobject.threads_init()  # Necessary for the transfer_server thread
        main_loop = gobject.MainLoop()
        
        opensand_uid = pwd.getpwnam('opensand').pw_uid
        current_uid = os.getuid()
        
        if options.kill:
            if current_uid not in [0, opensand_uid]:
                fail("This program must be started as the root or opensand "
                    "user to kill a background collector instance.")
                
            pid = read_pid_file()
            if pid == 0:
                fail("The collector does not seem to be running (no PID).")
            
            os.kill(pid, signal.SIGTERM)
            
            return
                
        
        if options.background:
            logging.basicConfig(level=level, filename=LOG_PATH)
        
            if current_uid != 0:
                fail("The collector must be started as root to run in the "
                    "background.")
            
            try:
                bg_fd = os.open(PID_PATH, os.O_WRONLY | os.O_CREAT)
                fcntl.flock(bg_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except OSError, err:
                fail(str(err))
            except IOError, err:
                if err.errno == errno.EACCES or err.errno == errno.EAGAIN:
                    fail("The collector seem to be already running in the "
                        "background.")
                else:
                    fail(str(err))
            
            os.setuid(opensand_uid)
        else:
            logging.basicConfig(level=level)

        try:
            with MessagesHandler(self) as msg_handler:
                port = msg_handler.get_port()
                with TransferServer(self.host_manager) as transfer_server:
                    trsfer_port = transfer_server.get_port()
                    with ServiceHandler(self, port, trsfer_port, service_type):
                        if options.background:
                            pid = os.fork()
                            if pid:
                                os.ftruncate(bg_fd, 0)
                                os.write(bg_fd, str(pid))
                                os._exit(0)
                            
                            null = open(os.path.devnull)
                            sys.stdin = sys.stdout = sys.stderr = null
                            
                            def handler(_sig, _frame):
                                """
                                SIGTERM handler
                                """
                                
                                logging.info("SIGTERM caught, quitting.")
                                main_loop.quit()
                            
                            signal.signal(signal.SIGTERM, handler)
                    
                        main_loop.run()
        finally:
            self.host_manager.cleanup()
            if options.background:
                os.ftruncate(bg_fd, 0)

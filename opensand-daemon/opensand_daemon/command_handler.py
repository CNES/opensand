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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
command_handler.py - server that get OpenSAND commands
"""

import logging
import time
import pickle
import ConfigParser
import shlex
import subprocess
import stat
import os

from opensand_daemon.tcp_server import MyTcpHandler
from opensand_daemon.process_list import ProcessList
from opensand_daemon.my_exceptions import Timeout, InstructionError, XmlError
from opensand_daemon.stream import DirectoryHandler
from opensand_daemon.routes import OpenSandRoutes
from opensand_daemon.interfaces import OpenSandIfaces, TUN_NAME, BR_NAME
from opensand_daemon.nl_utils import NlError

#macros
LOGGER = logging.getLogger('sand-daemon')

class CommandHandler(MyTcpHandler):
    """ The RequestHandler class for the command server """

    # line buffered
    rbufsize = 1
    wbufsize = 0

    def setup(self):
        """ the function called when CommandHandler is created """
        MyTcpHandler.setup(self)
        self._process_list = ProcessList()
        self._routes = OpenSandRoutes()
        self._interfaces = OpenSandIfaces()
        # wait a little bit that component list is initialized
        nbr = 0
        while self._process_list.is_initialized() == False and nbr < 5:
            LOGGER.warning("process list is still not initialized")
            time.sleep(1)
            nbr = nbr + 1
        if not self._routes.is_initialized():
            LOGGER.warning("routes are not initialized")

    def handle(self):
        """ handle a OpenSAND manager request """
        LOGGER.info("command server connected to manager: " + \
                    self.client_address[0])

        # test if the process list was initialized
        if not self._process_list.is_initialized():
            LOGGER.error("process list won't initialize")
            self.wfile.write("ERROR process list is not initialized\n")
            return
        
        # wait for 'DEPLOY', 'START' or 'STOP'
        try:
            self.read_data()
        except Timeout:
            LOGGER.error("timeout exception on server!")
            return
        except EOFError:
            LOGGER.error("EOFError exception on server!")
            return
        except Exception:
            LOGGER.exception("exception in command server: ")
            return
        else:
            LOGGER.debug("received: '" + self._data + "'")

        try:
            #TODO add argument after instruction to enable starting,
            #     deploying or stopping specific elements
            if self._data == 'DEPLOY':
                self.handle_data_request()
            elif self._data == 'CONFIGURE':
                self.handle_data_request()
            elif self._data.startswith('START'):
                iface = None
                try:
                    (cmd, iface) = self._data.split(' ', 1)
                except ValueError:
                    # no iface
                    pass
                self.handle_start(iface)
            elif self._data == 'STOP':
                self.handle_stop()
            elif self._data == 'TEST':
                self.handle_test()
            else:
                LOGGER.error("unknown command '" + self._data + "'\n")
                self.wfile.write("ERROR unknown command '%s'\n" % self._data)
        except Exception, msg:
            LOGGER.exception("exception while handling manager request: ")
            self.wfile.write("ERROR exception received while handling manager "
                             "request %s" % self._data)


    def handle_data_request(self):
        """ handle a DEPLOY or CONFIGURE request """
        LOGGER.debug("send: 'OK'")
        self.wfile.write("OK\n")

        # create the stream handler
        stream_handler = DirectoryHandler(self.rfile)

        try:
            ret = True
            while ret:
                ret = stream_handler.receive_content()
                LOGGER.debug("send: 'OK'")
                self.wfile.write("OK\n")
        except Timeout:
            self.wfile.write("ERROR timeout when reading flow\n")
            raise
        except InstructionError:
            self.wfile.write("ERROR bad instruction\n")
            raise
        except XmlError, msg:
            self.wfile.write("ERROR %s\n" % msg)
            raise

    def handle_start(self, iface):
        """ handle a START request """
        try:
            if self._process_list.is_running():
                LOGGER.error("some process are already started")
                raise InstructionError("some process are already started")

            is_l2 = False
            if iface == "TUN":
                iface = TUN_NAME
            if iface == "TAP":
                is_l2 = True
                iface = BR_NAME
            # set interfaces before routes
            # TODO we can add interfaces in bridge here (see nl_link_enslave and
            # release)
            # and then remove the part that need interface name in cpp code
            # and then remove interface name from avahi data
            self._interfaces.setup_interfaces(is_l2)
            self._routes.setup_routes(iface)
            self.start_binaries()
        except InstructionError as error:
            self.wfile.write("ERROR %s\n" % error.value)
            raise
        except OSError:
            self.wfile.write("ERROR cannot create directory or file\n")
            raise
        except IOError:
            self.wfile.write("ERROR unable to start process list\n")
            raise
        except pickle.PickleError:
            self.wfile.write("ERROR unable to load or serialize process list\n")
            raise
        except ConfigParser.Error:
            self.wfile.write("ERROR cannot read binaries configuration\n")
            raise
        except NlError:
            self.wfile.write("ERROR cannot set correct addresses on " \
                             "interfaces\n")
        else:
            LOGGER.debug("send: 'OK'")
            self.wfile.write("OK\n")


    def start_binaries(self):
        """ start the binaries specified in the binary configuration file """
        try:
            self._process_list.start()
        except Exception:
            raise

    def handle_stop(self):
        """ handle a STOP request """
        self._process_list.stop()
        self._routes.remove_routes()
        self._interfaces.standby()

        LOGGER.debug("send: 'OK'")
        self.wfile.write("OK\n")

    def handle_test(self):
        """ handle a TEST request"""
        LOGGER.debug("send: 'OK'")
        self.wfile.write("OK\n")

        try:
            self.read_data()
            LOGGER.debug("received: '" + self._data + "'")
            (instr, command) = self._data.split(' ', 1)
        except Exception:
            raise

        if instr != "COMMAND":
            self.wfile.write("ERROR bad instruction %s" % instr)
            return
                
        cmd = shlex.split(command)
        LOGGER.info("set execution rights on %s" % cmd[0])
        os.chmod(cmd[0], stat.S_IRWXU)
        LOGGER.info("launch test command: %s" % command)
        name = os.path.basename(cmd[0])
        with open('/tmp/opensand_tests/result', 'a') as output:
            output.write("\n")
            process = subprocess.Popen(cmd, close_fds=True,
                                       stdout=output,
                                       stderr=subprocess.STDOUT,
                                       cwd=os.path.dirname(command))
            
            # Put the process in the process list            
            process.prog_name = name
            ProcessList._process_lock.acquire()
            ProcessList._process_list[name] = process
            ProcessList._process_lock.release()

        timeout = 0
        # we use timeout because if _stop is not set
        # we won't be able to force kill in test
        while not MyTcpHandler._stop.is_set() and \
              process.returncode is None\
              and timeout < 300:
            process.poll()
            timeout += 1
            MyTcpHandler._stop.wait(1)

        if process.returncode is None:
            LOGGER.error("kill test because it was not stopped")
            process.terminate()
            MyTcpHandler._stop.wait(1)
            process.poll()
            if process.returncode is None:
                process.kill()

        process.wait()
        
        ProcessList._process_lock.acquire()
        try:
            del ProcessList._process_list[name]
        except KeyError:
            pass
        ProcessList._process_lock.release()

        if not MyTcpHandler._stop.is_set():
            out, err = process.communicate()
            if out is not None:
                LOGGER.debug("test output:\n" + out)
            if err is not None:
                LOGGER.debug("test errors:\n" + err)

            ret = process.returncode

            LOGGER.debug("test returned %s" % ret)
            LOGGER.debug("send: '%s'" % ret)
            self.wfile.write("%s\n" % ret)
        else:
            LOGGER.warning("test did not stopped normally\n")


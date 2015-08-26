#!/usr/bin/env python2
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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
tcp_server.py - server that get OpenSAND commands
"""

import SocketServer
import select
import threading

from opensand_manager_core.my_exceptions import CommandException
from opensand_manager_core.utils import green, red

class Plop(SocketServer.TCPServer):
    """ The TCP socket server with reuse address to true """

    allow_reuse_address = True

    def __init__(self, server_address, RequestHandlerClass,
                 controller, model, log):
        SocketServer.TCPServer.__init__(self, server_address,
                                        RequestHandlerClass)
        self.controller = controller
        self.model = model
        self.log = log

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
    _stop = threading.Event()


    def setup(self):
        """ the function called when MyTcpHandler is created """
        self.connection = self.request
        self.rfile = self.connection.makefile('rb', self.rbufsize)
        self.wfile = self.connection.makefile('wb', self.wbufsize)
        self._data = ''
        self._controller = self.server.controller
        self._model = self.server.model
        self._log = self.server.log

    def finish(self):
        """ the function called when handle returns """
        if not self.wfile.closed:
            self.wfile.flush()
        self.wfile.close()
        self.rfile.close()

    def read_data(self, timeout=True):
        """ read data on socket """
        if timeout:
            inputready, _, _ = select.select([self.rfile], [], [], 1)
            if(len(inputready) == 0):
                raise CommandException("timeout")

        self._data = self.rfile.readline()
        if (self._data == ''):
            raise CommandException("distant socket is closed")
        self._data = self._data.strip()


class CommandServer(MyTcpHandler):
    """ A TCP server that handles user command """

    # A callback that allows killing the main process,
    # it depends on the frontend and should intialized by it
    _shutdown =  None

    def handle(self):
        """ function for TCPServer """
        self._log.info("command server connected to: %s" %
                       self.client_address[0])

        self.wfile.write(LOGO)

        while not MyTcpHandler._stop.is_set():
            try:
                self.read_data()
            except CommandException, msg:
                if str(msg) == "timeout":
                    MyTcpHandler._stop.wait(0.1)
                    continue
                self._log.error("Error on command server %s" % msg)
                return
            else:
                self._log.debug("received: '%s'" % self._data)

            data = self._data.split(' ', 1)
            instr = data[0]
            cmd = []
            if len(data) > 1:
                cmd = data[1:]

            try:
                if not self.process_instruction(instr.lower(), cmd):
                    break
            except Exception, msg:
                self._log.error("Error on command server %s" % msg)
                self.wfile.write("ERROR\n")


    def process_instruction(self, instruction, params):
        """ parse the commands """
        if instruction in ['help', '?', 'h']:
            self.wfile.write(HELP)
        elif instruction == "status":
            status = ""
            status += "{:<10}: {:}\n".format("SCENARIO",
                                             self._model.get_scenario())
            if self._model.is_running():
                status += "{:<10}: {:}\n".format("RUN", self._model.get_run())
            if self._model.is_collector_known():
                running = green("RUNNING")
                if not self._model.is_collector_functional():
                    running = red("UNCREACHABLE...")
            else:
                running = red("STOPPED")
            status += "{:<10}: {:}\n".format("COLLECTOR", running)
            status += "{:<10}:\n".format("HOSTS")
            for host in self._model.get_hosts_list():
                state = host.get_state()
                if state == False:
                    running = red("STOPPED")
                elif state == True:
                    running = green("RUNNING")
                else:
                    running = red("UNCREACHABLE...")
                status += "  {:<6}: {:}\n".format(host.get_name().upper(),
                                                  running)
            if self._model.get_dev_mode():
                status += "Developer mode enabled\n"
            if self._model.get_adv_mode():
                status += "Advanced mode enabled\n"
            self.wfile.write(status)
        elif instruction == "start":
            if len(params) > 0:
                self._model.set_run(params[0])
            ret = self._controller.start_platform()
            if ret:
                self.wfile.write(green("OK (scenario '%s' run '%s')\n" %
                                 (self._model.get_scenario(),
                                  self._model.get_run())))
            else:
                self.wfile.write(red("ERROR\n"))
        elif instruction == "stop":
            ret = self._controller.stop_platform()
            if ret:
                self.wfile.write(green("OK\n"))
            else:
                self.wfile.write(red("ERROR\n"))
        elif instruction == "scenario":
            if len(params) > 0:
                self._model.set_scenario(params[0])
                self.wfile.write(green("OK (Scenario '%s')\n" %
                                 self._model.get_scenario()))
            else:
                self.wfile.write(red("ERROR\n"))
        elif instruction in ['quit', 'exit', 'close']:
            self.wfile.write("Goodbye\n")
            return False
        elif instruction in ['shutdown']:
            if CommandServer._shutdown is None:
                self.wfile.write(red("ERROR No shutdown callback available\n"))
            else:
                self.wfile.write("Goodbye\n")
                CommandServer._shutdown()
            return False
        elif instruction != '':
            self.wfile.write(red("Wrong command\n"))
        return True


HELP="Welcome on the OpenSAND command interface.\n" \
     "Commands:\n" \
     "  - help: print the available commands\n" \
     "  - status: get the platform status\n" \
     "  - start [run]: start the platform with the specific run id\n" \
     "  - stop: stop the platform\n" \
     "  - scenario name: load a scenario with the specified name\n" \
     "  - exit: logout from this server\n" \
     "  - shutdown: close the manager\n"

LOGO="                   [0;37;5;40;100m8[0;36;5;40;100m  t[0;30;5;40;100mSSSSX[0;36;5;40;100mt [0;37;5;40;100m8[0;1;30;90;47mX[0m                  \n" \
"              [0;1;30;90;47m8[0;36;5;40;100mt[0;30;5;40;100mSSSSSSSSSSSSSSSSSS[0;36;5;40;100mt[0;37;5;40;100m@[0m              \n" \
"           [0;37;5;40;100m8[0;30;5;40;100mSSSSSSSSX88888888XSSSSSSS[0;36;5;40;100m%[0;37;5;40;100mX[0m           \n" \
"         [0;36;5;40;100m:[0;30;5;40;100mSSSSSX8[0;1;30;90;40mX[0;34;40mS%%%%%%%%%%%%S[0;1;30;90;40mX[0;30;5;40;100m8XSSSSS[0;36;5;40;100m:[0m         \n" \
"       [0;36;5;40;100m [0;30;5;40;100mSSSSS8[0;34;40mS%%%%%%%%%%%%%%%%%%%%S[0;30;5;40;100m8SSSSS[0;36;5;40;100m.[0m       \n" \
"     [0;1;30;90;47mS[0;36;5;40;100m%[0;30;5;40;100mSSSS[0;1;30;90;40m8[0;34;40m%%%%%%%%%%%%%[0;30;5;40;100m8[0;36;5;40;100m; [0;37;5;40;100mX@[0;36;5;40;100m :[0;34;40mX%%%%%[0;1;30;90;40m8[0;30;5;40;100mSSSSS[0;1;30;90;47m8[0m     \n" \
"    [0;36;5;40;100m [0;30;5;40;100mSSSS8[0;34;40m%%%%%%%[0;1;37;97;47mt [0;36;5;40;100m [0;34;40mX%%[0;36;5;40;100m [0;1;37;97;47m@t[0;1;30;90;47m%@@X[0;1;37;97;47m:[0;37;5;47;107m@t[0;1;37;97;47mt[0;37;5;40;100mX[0;34;40mX%%%%[0;30;5;40;100m8SSSS[0;36;5;40;100m [0m    \n" \
"   [0;37;5;40;100mS[0;30;5;40;100mSSSS[0;1;30;90;40m@[0;34;40m%%%%%%%[0;36;5;40;100m:[0;37;5;47;107m::[0;1;30;90;47m:[0;1;37;97;47m@[0;1;30;90;47m;[0;34;5;40;100mS[0;1;30;90;40m@[0;34;40m%%%%%%%%[0;34;5;40;100m8[0;1;30;90;47m%[0;37;5;47;107mS8[0;36;5;40;100m [0;34;40m%%%%[0;1;30;90;40m@[0;30;5;40;100mSSSS[0;36;5;40;100m [0m   \n" \
"  [0;37;5;40;100m@[0;30;5;40;100mSSSS[0;34;40mX%%%%%%%%[0;1;30;90;47m@[0;37;5;47;107m::[0;34;40m@%[0;36;5;40;100m [0;37;5;47;107m8[0;1;30;90;47m.[0;34;5;40;100m8[0;34;40m%%%%%[0;34;5;40;100m88[0;34;40m%%8[0;1;37;97;47m%[0;37;5;47;107mt[0;37;5;40;100mS[0;34;40m%%%%X[0;30;5;40;100mSSSS[0;36;5;40;100m [0m  \n" \
"  [0;30;5;40;100mSSSS[0;1;30;90;40mX[0;34;40m%%%%%%%%%[0;1;30;90;47mX[0;37;5;47;107m::[0;1;30;90;47m%[0;34;40m%%S[0;1;30;90;47m8[0;37;5;47;107m%[0;36;5;40;100m [0;34;40m%%%[0;1;37;97;47m%[0;37;5;47;107m::[0;1;37;97;47m [0;34;40m%%X[0;1;37;97;47m8[0;37;5;47;107m8[0;34;40mX%%%%[0;1;30;90;40mX[0;30;5;40;100mSSSS[0m  \n" \
" [0;36;5;40;100m.[0;30;5;40;100mSSS8[0;34;40m%%%%%%%%%%[0;1;30;90;47m8[0;37;5;47;107m:::[0;36;5;40;100m [0;34;40m%%%[0;34;5;40;100m%[0;37;5;47;107mt[0;1;37;97;47m:[0;1;30;90;47m;[0;37;5;47;107m@8[0;1;30;90;47m8[0;37;5;40;100m8[0;34;5;40;100m8[0;34;40m%%%[0;36;5;40;100m.[0;37;5;47;107mS[0;36;5;40;100m [0;34;40m%%%%%[0;30;5;40;100m8SSS[0;36;5;40;100mt[0m \n" \
" [0;30;5;40;100mSSSS[0;34;40mS%%%%%%%%%%[0;34;5;40;100m8[0;37;5;47;107m:::;[0;36;5;40;100m [0;34;5;40;100m8[0;1;30;90;47m8[0;1;37;97;47m@[0;37;5;47;107m:::[0;1;30;90;47m%[0;34;40mS%%%%%%[0;34;5;40;100m8[0;37;5;47;107m%[0;36;5;40;100m [0;34;40m%%%%%S[0;30;5;40;100mSSSS[0m \n" \
"[0;36;5;40;100m [0;30;5;40;100mSSS8[0;34;40m%%%%%%%%%%%%[0;1;37;97;47mt[0;37;5;47;107m:::::::;[0;1;30;90;47m;[0;1;37;97;47m8[0;37;5;47;107m8[0;34;40m@%%%%%%[0;37;5;40;100m%[0;37;5;47;107m8[0;34;5;40;100m8[0;34;40m%%%%%%[0;30;5;40;100m8SSS[0;37;5;40;100mX[0m\n" \
"[0;30;5;40;100mXSSS[0;1;30;90;40m8[0;34;40m%%%%%%%%%%%%[0;30;5;40;100m8[0;37;5;47;107mS:::::[0;1;37;97;47m;[0;34;5;40;100m8[0;34;40m%@[0;1;37;97;47m8X[0;34;40m%%%%%[0;30;5;40;100m8[0;37;5;47;107mX[0;1;30;90;47m8[0;34;40m%%%%%%%[0;1;30;90;40m8[0;30;5;40;100mSSS[0;36;5;40;100m [0m\n" \
"[0;30;5;40;100mSSSS[0;1;30;90;40m8[0;34;40m%%%%%%%%%%%%%[0;34;5;40;100m8[0;37;5;47;107m%::::[0;1;37;97;47m%[0;34;40m8%%@[0;37;5;47;107mX[0;37;5;40;100mX[0;34;40m%%%[0;36;5;40;100mt[0;1;37;97;47m8[0;1;30;90;47m@[0;34;40m%%%%%%%%[0;1;30;90;40m8[0;30;5;40;100mSSS[0;34;5;40;100m%[0m\n" \
"[0;30;5;40;100mSSSS[0;1;30;90;40m8[0;34;40m%%%%%%%%%%%%%[0;1;30;90;40m8[0;37;5;47;107mt:::::S[0;37;5;40;100mX[0;34;40mS%[0;37;5;40;100m@[0;37;5;47;107mX[0;34;40mS%[0;36;5;40;100m;[0;1;30;90;47m;[0;36;5;40;100m:[0;34;40m%%%%%%%%%[0;1;30;90;40m8[0;30;5;40;100mSSS[0;36;5;40;100mt[0m\n" \
"[0;36;5;40;100m [0;30;5;40;100mSSS8[0;34;40m%%%%%%%%%%%%%[0;1;37;97;47mt[0;37;5;47;107m::[0;1;37;97;47m:X[0;37;5;47;107m::::8[0;37;5;40;100mX[0;1;30;90;47m;[0;37;5;47;107m:[0;36;5;40;100m [0;34;40m%%%%%%%%%%%%%[0;30;5;40;100m8SSS[0;37;5;40;100mS[0m\n" \
" [0;30;5;40;100mSSSS[0;34;40m%%%%%%%%%%%%[0;36;5;40;100m:[0;37;5;47;107m:::X[0;34;40m@[0;34;5;40;100mS[0;1;30;90;47m8[0;1;37;97;47mS[0;37;5;47;107mt::::[0;1;30;90;47m8[0;34;40m%%%%%%%%%%%%%[0;30;5;40;100mSSSS[0m \n" \
" [0;36;5;40;100m;[0;30;5;40;100mSSS8[0;34;40m%%%%%%%%%%%[0;1;37;97;47m@[0;37;5;47;107m::::[0;1;30;90;47mX[0;34;40m%%%%[0;30;5;40;100m8[0;34;5;40;100m8SX[0;34;40m8%%%%%%%%%%%%[0;30;5;40;100m8SSSS[0m \n" \
" [0;1;30;90;47mS[0;30;5;40;100mSSSS[0;34;40mX%%%%%%%%%[0;36;5;40;100m [0;37;5;47;107m:::::t[0;30;5;40;100m8[0;34;40m%%%%%%%%%%%%%%%%%%%X[0;30;5;40;100mSSSS[0;1;30;90;47m@[0m \n" \
"  [0;36;5;40;100m [0;30;5;40;100mSSSX[0;34;40mS%%%%%%%%[0;37;5;40;100m8[0;37;5;47;107m;::::S[0;34;5;40;100mS[0;34;40m%%%%%%%%%%%%%%%%%%S[0;30;5;40;100mXSSS[0;36;5;40;100m [0m  \n" \
"   [0;36;5;40;100m [0;30;5;40;100mSSSS[0;34;40mX%%%[0;37;5;40;100m%[0;1;30;90;47mSSSS%[0;1;37;97;47m8[0;1;30;90;47m.[0;1;37;97;47m:%[0;1;30;90;47mSSSS8[0;34;5;40;100m8[0;34;40m%%%%%%%%%%%%%X[0;30;5;40;100mSSSS[0;36;5;40;100m [0m   \n" \
"    [0;36;5;40;100m [0;30;5;40;100mSSSS8[0;34;40m%S[0;1;30;90;47m.[0;1;37;97;47m@@@@@@@XS%;:: [0;36;5;40;100m:[0;34;40m%%%%%%%%%%%%[0;30;5;40;100m8SSSS[0;36;5;40;100m [0m    \n" \
"     [0;37;5;40;100m8[0;30;5;40;100mSSSSX[0;1;30;90;40m@[0;34;40m%%%%%%%%%%%%%%%%%%%%%%%%%%[0;1;30;90;40m@[0;30;5;40;100mXSSSS[0;37;5;40;100m@[0m     \n" \
"       [0;36;5;40;100m [0;30;5;40;100mSSSSX[0;1;30;90;40m8[0;34;40mS%%%%%%%%%%%%%%%%%%%%S[0;1;30;90;40m8[0;30;5;40;100mXSSSS[0;36;5;40;100m:[0m       \n" \
"         [0;36;5;40;100m:[0;30;5;40;100mSSSSS@[0;1;30;90;40m8[0;34;40mXS%%%%%%%%%%%%SX[0;1;30;90;40m8[0;30;5;40;100mXSSSSS[0;36;5;40;100m;[0;1;30;90;47mt[0m        \n" \
"           [0;37;5;40;100mS[0;30;5;40;100mSSSSSSSX88[0;1;30;90;40m888888[0;30;5;40;100m88XSSSSSSS[0;36;5;40;100m [0m           \n" \
"              [0;37;5;40;100mS[0;36;5;40;100mt[0;30;5;40;100mSSSSSSSSSSSSSSSSSSS[0;36;5;40;100m [0m              \n" \
"                  [0;1;30;90;47mS[0;37;5;40;100m@[0;36;5;40;100m .[0;30;5;40;100mSSSSSX[0;36;5;40;100m; [0;37;5;40;100m@[0;1;30;90;47mX[0m                  \n" \
"\n\nType 'help' for a list of available commands\n"

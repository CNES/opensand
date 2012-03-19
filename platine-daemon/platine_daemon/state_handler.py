#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
#
#
# This file is part of the Platine testbed.
#
#
# Platine is free software : you can redistribute it and/or modify it under the
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
state_handler.py - server that handle component state request
"""

import logging
import threading
import thread
import time
from platine_daemon.process_list import ProcessList
from platine_daemon.tcp_server import MyTcpHandler
from platine_daemon.my_exceptions import Timeout

#macros
LOGGER = logging.getLogger('PtDmon')

class StateHandler(MyTcpHandler):
    """ The RequestHandler class for the state server """

    def setup(self):
        """ the function called when StateHandler is created """
        MyTcpHandler.setup(self)
        self._stop = threading.Event()
        self._compo_list = [] # the list of started components
        self._process_list = ProcessList()
        # this should already be done 
        self._process_list.reset()

    def finish(self):
        """ the function called when the handler returns """
        self._process_list.reset()

    def handle(self):
        """ handle a Platine manager state request """
        LOGGER.info("state server connected to manager: " + \
                    self.client_address[0])
        try:
            self.read_data()
        except Timeout:
            LOGGER.error("timeout exception on server!")
            return
        except EOFError:
            LOGGER.error("EOFError exception on server!")
            return
        except Exception, msg:
            LOGGER.error("state server exception (%s)!" % msg)
            return
        else:
            LOGGER.debug("received: '" + self._data + "'")

        try:
            if self._data == 'STATE':
                update_thread = threading.Thread(None, self.update, None, (), {})
                LOGGER.debug("start update thread")
                update_thread.start()
            else:
                LOGGER.error("unknown command '" + self._data + "'\n")
                self.wfile.write("ERROR unknown command '" + \
                                 self._data + "'\n")
                return
        except thread.error, msg:
            LOGGER.error("exception when starting the update thread: " + msg)
            return
        except Exception:
            LOGGER.error("exception while handling manager request")
            return

        # wait that thread is correctly started
        time.sleep(2)

        if not update_thread.is_alive():
            LOGGER.error("update thread is not started correctly: quit")
            self.wfile.write("ERROR cannot start update thread\n")

        # wait for BYE
        try:
            self.read_data(False)
        except EOFError:
            LOGGER.error("EOFError exception on server!")
            return
        except Exception, msg:
            LOGGER.error("state server exception (%s)!" % msg)
            return
        else:
            LOGGER.debug("received: '%s'", self._data)
        finally:
            # stop update thread because manager send a message only
            # when it stops
            LOGGER.debug("stop update thread")
            self._stop.set()
            if update_thread.is_alive():
                update_thread.join()
            LOGGER.debug("update thread joined")

        if self._data == 'BYE':
            LOGGER.info("manager is stopped")
            self.wfile.write("OK\n")
        else:
            LOGGER.error("%s received from manager", self._data)


    def update(self):
        """ update process list """
        self._process_list.load()
        self.send_and_update_state(True)
        while not self._stop.isSet():
            # check program state to detect crashes
            self._process_list.update(True)
            self.send_and_update_state()
            self._stop.wait(1.0)


    def send_and_update_state(self, first = False):
        """ send the status of each component for
            which the state has changed """
        new_compo_list = self._process_list.get_components()

        self._compo_list.sort()
        new_compo_list.sort()
        if first or self._compo_list != new_compo_list:
            # update the component list
            self._compo_list = new_compo_list
            LOGGER.debug("component list has changed: " + str(self._compo_list))
            self.send_list()
            try:
                self._process_list.serialize()
            except:
                LOGGER.error("error when serializing process list: " \
                             "stop update server")
                self._stop.set() 


    def send_list(self):
        """ send the list of started components to manager """
        compo_list = 'STARTED'
        for compo in self._compo_list:
            compo_list = compo_list + ' ' + compo
        compo_list = compo_list + '\n'
        LOGGER.debug("send: '%s'", compo_list.strip())
        self.wfile.write(compo_list)

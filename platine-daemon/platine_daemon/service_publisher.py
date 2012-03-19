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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>
# TODO lien vers exemple avahi

"""
service_publisher.py - publish a Platine service with Avahi
"""

import dbus
import gobject
import avahi
import logging
import sys
from dbus.mainloop.glib import DBusGMainLoop, threads_init

#macros
LOGGER = logging.getLogger('PtDmon')

class PlatineServicePublisher():
    """ publish avahi service for Platine """

    def __init__(self, type, name, port, descr=None):
        self._name = name # platine entity name (sat, st, gw, ws)
        self._type = type
        self._port = port
        if descr == None:
            self._text = {}
        else:
            self._text = descr
            if 'id' in descr:
                self._name = name + str(descr['id'])

        self._domain = "" # domain to publish on, default to .local
        self._host = ""   # host to publish records for, default to local_host

        self._group = None #our entry group
        self._bus = None
        self._main_loop = None
        self._server = None

    def add_service(self):
        """ add a new service """
        if self._group is None:
            self._group = dbus.Interface(
                    self._bus.get_object(avahi.DBUS_NAME,
                                         self._server.EntryGroupNew()),
                    avahi.DBUS_INTERFACE_ENTRY_GROUP)
            self._group.connect_to_signal('StateChanged',
                                          self.entry_group_state_changed)

        LOGGER.debug("Adding service '%s' of type '%s' with text %s",
                     self._name, self._type, self._text)

        self._group.AddService(
                avahi.IF_UNSPEC,    #interface
                avahi.PROTO_INET,   #protocol
                dbus.UInt32(0),     #flags
                self._name, self._type,
                self._domain, self._host,
                dbus.UInt16(self._port),
                avahi.dict_to_txt_array(self._text),
                reply_handler=self.commit_group,
                error_handler=self.print_error)

    def commit_group(self, *args):
        """ reply handler for AddService """
        self._group.Commit()

    def print_error(self, *args):
        """ error handler """
        LOGGER.error('service error handler: ' + str(args[0]))
        self.stop()

    def remove_service(self):
        """ remove a service """
        if not self._group is None:
            self._group.Reset()

    def server_state_changed(self, state):
        """ signal received when server state change """
        if state == avahi.SERVER_COLLISION:
            LOGGER.error("server name collision")
            self.remove_service()
        elif state == avahi.SERVER_RUNNING:
            self.add_service()

    def entry_group_state_changed(self, state, error):
        """ signal received when group state change """
        LOGGER.debug("state change: " + str(state))

        if state == avahi.ENTRY_GROUP_ESTABLISHED:
            LOGGER.debug("service established")
        elif state == avahi.ENTRY_GROUP_COLLISION:
            LOGGER.error("service name collision")
            self.stop()
        elif state == avahi.ENTRY_GROUP_FAILURE:
            LOGGER.error("error in group state changed" +  error)
            self.stop()

    def run(self):
        """ run the Platine service """
        DBusGMainLoop(set_as_default = True)
        # Init gobject threads and dbus threads
        gobject.threads_init()
        threads_init()

        self._main_loop = gobject.MainLoop()
        self._bus = dbus.SystemBus()

        self._server = dbus.Interface(
                       self._bus.get_object(avahi.DBUS_NAME,
                                            avahi.DBUS_PATH_SERVER),
                       avahi.DBUS_INTERFACE_SERVER)

        self._server.connect_to_signal("StateChanged",
                                       self.server_state_changed)
        self.server_state_changed(self._server.GetState())

        self._main_loop.run()

    def stop(self):
        """ stop the Platine service """
        self._main_loop.quit()
        if self._group is not None:
            self._group.Free()


##### TEST #####

if __name__ == '__main__':
    # Print logs in terminal for debug
    LOG_HANDLER = logging.StreamHandler(sys.stdout)
    FORMATTER = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s "\
                                  "- %(message)s")
    LOG_HANDLER.setFormatter(FORMATTER)
    LOGGER.addHandler(LOG_HANDLER)

    LOGGER.setLevel(logging.DEBUG)

    descr = {
                'id': 1,
                'state' : 5555,
                'command' : 4444,
             }

    SERVICE = PlatineServicePublisher("_platine._tcp", "st", 1234, descr)

    try:
        SERVICE.run()
    except KeyboardInterrupt:
        SERVICE.stop()

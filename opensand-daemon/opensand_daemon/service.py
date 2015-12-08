#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 TAS
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
service.py - handle OpenSAND services with Avahi
"""

import dbus
import gobject
import avahi
import logging
import sys
from dbus.mainloop.glib import DBusGMainLoop, threads_init
from dbus.exceptions import DBusException
from opensand_daemon.routes import OpenSandRoutes
from opensand_daemon.interfaces import TUN_NAME

#macros
LOGGER = logging.getLogger('sand-daemon')

def on_error(self, *args):
    """ error handler """
    if len(args) == 0:
        return
    LOGGER.error('service error handler: ' + str(args[0]))
    stop_serv()

def stop_serv():
    """ stop the OpenSAND service """
    OpenSandService._main_loop.quit()
    if OpenSandService._publisher._group is not None:
        OpenSandService._publisher._group.Free()
    if OpenSandService._routes is not None:
        OpenSandService._routes.delete()

class OpenSandService(object):
    """ Avahi service for OpenSAND Daemon """

    _bus = None
    _routes = None

    _main_loop = None
    _listener = None
    _publisher = None

    def __init__(self, cache_dir, iface, service_type, name,
                 instance, port, descr=None, output_handler=None):
        loop = DBusGMainLoop(set_as_default=True)
        # Init gobject threads and dbus threads
        gobject.threads_init()
        threads_init()

        OpenSandService._main_loop = gobject.MainLoop()
        OpenSandService._bus = dbus.SystemBus(mainloop=loop)

        OpenSandService._routes = OpenSandRoutes()
        if name.lower() != "sat":
            if name.lower() != "ws":
                # by default we use TUN interface but this can be modified
                # using setup routes when we are in Ethernet
                OpenSandService._routes.load(cache_dir, name, TUN_NAME)
            else:
                OpenSandService._routes.load(cache_dir, name,
                                             descr['lan_iface'], True)
        else:
            # no route to handle on satellite
            OpenSandService._routes.set_unused()

        OpenSandService._listener = self.Listener(iface, service_type,
                                                  name, instance, output_handler)
        OpenSandService._publisher = self.Publisher(iface, service_type,
                                                    name, port, descr)

    def run(self):
        """ run the dbus loop """
        OpenSandService._main_loop.run()

    def stop(self):
        """ stop the OpenSandService """
        stop_serv()

    class Listener(object):
        """ listen for OpenSAND service with avahi """
        def __init__(self, iface, service_type, compo, instance,
                     output_handler=None):
            self._iface = iface
            self._compo = compo.lower()
            # for WS get only the number, not the name of the instance
            self._instance = instance.split("_", 1)[0]
            self._output_handler = output_handler
            self._names = []
            self._current_host = self._compo + self._instance
            self._new_routes = []
            self._router_v4 = None
            self._router_v6 = None
            self._listener_server = \
                dbus.Interface(OpenSandService._bus.get_object(avahi.DBUS_NAME, '/'),
                               'org.freedesktop.Avahi.Server')

            if self._iface != '':
                try:
                    iface = self._listener_server.GetNetworkInterfaceIndexByName(self._iface)
                except DBusException:
                    LOGGER.warning("Cannot listen Avahi service on %s iface")
                    iface = avahi.IF_UNSPEC
            else:
                iface = avahi.IF_UNSPEC

            sbrowser = dbus.Interface(OpenSandService._bus.get_object(avahi.DBUS_NAME,
                    self._listener_server.ServiceBrowserNew(iface,
                        avahi.PROTO_INET, service_type, 'local', dbus.UInt32(0))),
                    avahi.DBUS_INTERFACE_SERVICE_BROWSER)

            sbrowser.connect_to_signal("ItemNew", self.handler_new)
            sbrowser.connect_to_signal("ItemRemove", self.handler_remove)

            LOGGER.debug("Avahi listener started")

        def service_resolved(self, *args):
            """ get the parameter of service once it is resolved """
            name = args[2].lower()
            if name == 'collector':
                # only add collector if it can receive information from our
                # publish interface
                try:
                    # check if we publish on one interface only
                    iface = \
                        self._listener_server.GetNetworkInterfaceIndexByName(self._iface)
                except DBusException:
                    pass
                else:
                    interface = args[0]
                    if interface != iface:
                        return
                address = args[7]
                port = int(args[8])
                if ':' in address:
                    LOGGER.debug("Ignoring collector IPv6 address")
                    return
                LOGGER.debug("found collector at %s:%d", address, port)
                if self._output_handler is not None:
                    self._output_handler.set_collector_addr(address, port)
                return
            elif self._compo == 'sat':
                # nothing to do for other hosts no sat
                return

            if name in self._names:
                LOGGER.debug("ignore %s that is already discovered" % name)
                return
            elif name == self._current_host:
                return
            elif not name.startswith('st') and not name.startswith('gw'):
                LOGGER.debug("ignore %s that is not a ST or GW" % name)
                return
            address = args[7]
            LOGGER.debug('service resolved')
            LOGGER.debug('name: ' + name)
            LOGGER.debug("Find %s at address %s" %
                         (name, address))

            if address.count(':') > 0:
                LOGGER.warning("IPv6 service: ignore it")
                LOGGER.warning("Restart the manager and/or the daemon if no "
                               "IPv4 service is detected")
                return

            v4 = None
            v6 = None
            inst = ''
            i = 0
            args_nbr = len(args[9])
            while i < args_nbr:
                txt = args[9].pop()
                i = i + 1
                j = 0
                key = ''
                while(j < len(txt) and str(txt[j]) != '='):
                    key = key + str(txt[j])
                    j = j + 1
                # remove '='
                j = j + 1
                val = ''
                while(j < len(txt)):
                    val = val + str(txt[j])
                    j = j + 1
                LOGGER.debug("txt: %s = %s" % (key, val))
                if key == 'lan_ipv4':
                    v4 = val
                elif key == 'lan_ipv6':
                    v6 = val
                elif key == 'id':
                    inst = val

            self._names.append(name)
            if self._compo == 'gw' or self._compo == 'st':
                OpenSandService._routes.add_distant_host(name, v4, v6)
            elif self._compo == 'ws' and not name.startswith('ws') and name != 'sat':
                if inst == self._instance:
                    # this host is our router (ST with the same ID)
                    self._router_v4 = v4.rsplit('/')[0]
                    self._router_v6 = v6.rsplit('/')[0]
                    # add the route toward other network that was not added yet
                    for route in self._new_routes:
                        OpenSandService._routes.add_distant_host(route[0],
                                                                 route[1],
                                                                 route[2],
                                                                 self._router_v4,
                                                                 self._router_v6)
                elif self._router_v4 is not None or self._router_v6 is not None:
                    # add the distant network route
                    OpenSandService._routes.add_distant_host(name, v4, v6,
                                                            self._router_v4,
                                                            self._router_v6)

                else:
                    # we need to know the router address to set the route
                    # gateway so keep this route until we got it
                    self._new_routes.append((name, v4, v6))

        def handler_new(self, interface, protocol, name, stype, domain, flags):
            """ handle a new service """
            LOGGER.debug("Found service '%s' type '%s' domain '%s' " %
                         (name, stype, domain))

            if flags & avahi.LOOKUP_RESULT_LOCAL:
                # local service, skip
                pass

            self._listener_server.ResolveService(interface, protocol, name, stype,
                                        domain, avahi.PROTO_INET, dbus.UInt32(0),
                                        reply_handler=self.service_resolved,
                                        error_handler=on_error)

        def handler_remove(self, interface, protocol, name, stype, domain, flags):
            """ handle a removed service """

            if name == 'collector':
                LOGGER.debug("Collector service disconnected")
                if self._output_handler is not None:
                    self._output_handler.unset_collector_addr()
                return
            elif self._compo == 'sat':
                # nothing to do for other hosts no sat
                return

            LOGGER.debug("Service removed '%s' type '%s' domain '%s' " %
                         (name, stype, domain))
            if not name in self._names:
                LOGGER.debug("ignore %s that is not in our list" % name)
                return
            LOGGER.info("the component %s was disconnected" % name)
            self._names.remove(name)
            OpenSandService._routes.remove_distant_host(name)

    class Publisher(object):
        """ publish avahi service for OpenSAND """

        def __init__(self, iface, service_type, name, port, descr=None):
            self._iface = iface
            self._name = name # opensand entity name (sat, st, gw, ws)
            self._type = service_type
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
            self._publisher_server = None

            self._publisher_server = dbus.Interface(
                           OpenSandService._bus.get_object(avahi.DBUS_NAME,
                                                avahi.DBUS_PATH_SERVER),
                           avahi.DBUS_INTERFACE_SERVER)

            self._publisher_server.connect_to_signal("StateChanged",
                                           self.server_state_changed)
            self.server_state_changed(self._publisher_server.GetState(), None)

        def add_service(self):
            """ add a new service """
            if self._group is None:
                self._group = dbus.Interface(
                        OpenSandService._bus.get_object(avahi.DBUS_NAME,
                                             self._publisher_server.EntryGroupNew()),
                        avahi.DBUS_INTERFACE_ENTRY_GROUP)
                self._group.connect_to_signal('StateChanged',
                                              self.entry_group_state_changed)

            if self._iface != '':
                try:
                    iface = self._publisher_server.GetNetworkInterfaceIndexByName(self._iface)
                except DBusException:
                    LOGGER.warning("Cannot publish Avahi service on %s iface")
                    iface = avahi.IF_UNSPEC
            else:
                iface = avahi.IF_UNSPEC

            LOGGER.debug("Adding on iface %s (%i) service '%s' of type '%s' with text %s",
                         self._iface, iface, self._name, self._type, self._text)

            self._group.AddService(
                    iface,              #interface
                    avahi.PROTO_INET,   #protocol
                    dbus.UInt32(0),     #flags
                    self._name, self._type,
                    self._domain, self._host,
                    dbus.UInt16(self._port),
                    avahi.dict_to_txt_array(self._text),
                    reply_handler=self.commit_group,
                    error_handler=on_error)

        def commit_group(self, *args):
            """ reply handler for AddService """
            self._group.Commit()

        def remove_service(self):
            """ remove a service """
            if not self._group is None:
                self._group.Reset()

        def server_state_changed(self, state, error):
            """ signal received when server state change """
            if state == avahi.SERVER_COLLISION:
                LOGGER.error("server name collision")
                self.remove_service()
            elif state == avahi.SERVER_RUNNING:
                self.add_service()
            else:
                LOGGER.warning("Server state changed %s (error: %s)" % (state,
                                                                        error))

        def entry_group_state_changed(self, state, error):
            """ signal received when group state change """
            LOGGER.debug("state change: " + str(state))

            if state == avahi.ENTRY_GROUP_ESTABLISHED:
                LOGGER.debug("service established")
            elif state == avahi.ENTRY_GROUP_COLLISION:
                LOGGER.error("service name collision")
                stop_serv()
            elif state == avahi.ENTRY_GROUP_FAILURE:
                LOGGER.error("error in group state changed" +  error)
                stop_serv()

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

    SERVICE = OpenSandService("/tmp", "eth2", "_opensand._tcp", "sat_test", "", 1234, descr)

    try:
        SERVICE.run()
    except KeyboardInterrupt:
        stop_serv()

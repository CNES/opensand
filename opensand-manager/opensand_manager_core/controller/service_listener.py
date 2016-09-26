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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>
# inspired from http://avahi.org/wiki/PythonBrowseExample

"""
service_listener.py - service browser for OpenSAND
"""

import sys
import time

import dbus, gobject, avahi
from dbus.mainloop.glib import DBusGMainLoop, threads_init

from opensand_manager_core.utils import WS
from opensand_manager_core.controller.host import HostController
from opensand_manager_core.loggers.manager_log import ManagerLog
from opensand_manager_core.opensand_model import Model
from opensand_manager_core.my_exceptions import ModelException

class OpenSandServiceListener():
    """ listen for OpenSAND service with avahi """
    def __init__(self, model, hosts, ws, env_plane, service_type, manager_log):
        self._server = None
        self._log = manager_log
        self._model = model
        self._hosts = hosts
        self._ws = ws
        self._env_plane = env_plane

        loop = DBusGMainLoop()
        # enable dbus multithreading
        threads_init()

        bus = dbus.SystemBus(mainloop=loop)

        self._server = dbus.Interface(bus.get_object(avahi.DBUS_NAME, '/'),
                      'org.freedesktop.Avahi.Server')

        sbrowser = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
                self._server.ServiceBrowserNew(avahi.IF_UNSPEC,
                    avahi.PROTO_INET, service_type, 'local', dbus.UInt32(0))),
                avahi.DBUS_INTERFACE_SERVICE_BROWSER)

        sbrowser.connect_to_signal("ItemNew", self.handler_new)
        sbrowser.connect_to_signal("ItemRemove", self.handler_remove)
        sbrowser.connect_to_signal("AllForNow", self.handler_end)

        # there is no need to create and start a gobject MainLoop because
        # there is already the gtk loop. Thus, this service MUST be created
        # before starting the gtk main loop


    def service_resolved(self, *args):
        """ get the parameter of service once it is resolved """
        name = args[2]
        address = args[7]
        port = args[8]
        txt = args[9]
        self._log.debug('service resolved')
        self._log.debug('name: ' + args[2])
        self._log.debug('address: ' + args[7])
        self._log.debug('port: ' + str(args[8]))
        self._log.info("Find %s at address %s" %
                       (name, address))

        if name == "collector":
            try:
                items = dict("".join(chr(i) for i in arg).split("=", 1)
                             for arg in txt)
                transfer_port = int(items.get('transfer_port', ""))
            except ValueError:
                self._log.error("Failed to get the collector transfer port.")
                return
            
            self._env_plane.register_on_collector(address, port, transfer_port)
            self._model.set_collector_known(True)
                        
            return

        if address.count(':') > 0:
            self._log.warning("IPv6 service: ignore it")
            self._log.warning("Restart the manager and/or the daemon if no "
                              "IPv4 service is detected")
            return

        inst = ''
        state_port = ''
        command_port = ''
        cache = None
        tools = []
        modules = []
        network_config = {'discovered' : address}
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
            self._log.debug("txt: %s = %s" % (key, val))
            if key == 'id':
                inst = val
            elif key == 'state':
                state_port = val
            elif key == 'command':
                command_port = val
            elif key == 'tools':
                tools = val.split()
            elif key == 'modules':
                modules = map(str.upper, val.split())
            elif key == 'emu_iface':
                network_config['emu_iface'] = val
            elif key == 'emu_ipv4':
                network_config['emu_ipv4'] = val
            elif key == 'lan_iface':
                network_config['lan_iface'] = val
            elif key == 'lan_ipv4':
                network_config['lan_ipv4'] = val
            elif key == 'lan_ipv6':
                network_config['lan_ipv6'] = val
            elif key == 'mac':
                network_config['mac'] = val
            elif key == 'cache':
                cache = val
        try:
            host_model = self._model.add_host(name, inst, network_config,
                                              state_port, command_port,
                                              tools, modules)
        except ModelException:
            # host already exists
            return
        else:
            # if already added model, return here, but add components
            existant_host = self.get_host(host_model.get_name())
            if existant_host:
                existant_host.new_machine(cache)
            elif not name.startswith(WS):
                new_host = HostController(host_model, self._log)
                new_host.new_machine(cache)
                self._hosts.append(new_host)
            # we need controller for workstations with tools
            elif name.startswith(WS):
                if len(host_model.get_tools()) > 0:
                    new_host = HostController(host_model, self._log)
                    new_host.new_machine(cache)
                    self._ws.append(new_host)
                    # stop here
                    return
        finally:
            # this is usefull when manager is started while some hosts are
            # running, collector may be registered before a host so the host
            # model would not be available at registration
            # also used when host's daemon is restarted
            self._env_plane.register_host(name)

        # host controller will try to get host state, tell model when this is
        # done
        count = 0
        while host_model.get_state() is None and count < 4:
            count += 1
            time.sleep(0.5)
        self._model.host_ready(host_model)

    def print_error(self, *args):
        """ error handler """
        self._log.error('service error handler: ' + str(args[0]))

    def handler_new(self, interface, protocol, name, stype, domain, flags):
        """ handle a new service """
        self._log.debug("Found service '%s' type '%s' domain '%s' " %
                        (name, stype, domain))

        if flags & avahi.LOOKUP_RESULT_LOCAL:
            # local service, skip
            pass

        self._server.ResolveService(interface, protocol, name, stype,
                                    domain, avahi.PROTO_INET, dbus.UInt32(0),
                                    reply_handler=self.service_resolved,
                                    error_handler=self.print_error)

    def handler_remove(self, interface, protocol, name, stype, domain, flags):
        """ handle a removed service """
        self._log.debug("Service removed '%s' type '%s' domain '%s' " %
                        (name, stype, domain))
        self._log.info("the component %s was disconnected" % name)
        
        if name == "collector":
            self._env_plane.unregister_on_collector()
            self._model.set_collector_known(False)
            self._model.set_collector_functional(False)
            return
        
        self._model.del_host(name)
        for host in self._hosts:
            if host.get_name().lower() == name:
                host.close()
                self._hosts.remove(host)
        for ws in self._ws:
            if ws.get_name().lower() == name:
                ws.close()
                self._ws.remove(ws)

    def handler_end(self):
        """ all service discovered """
        pass

    def get_host(self, host_name):
        """ get host controller from hosts list """
        for host in self._hosts:
            if host.get_name() == host_name.upper():
                return host
        return None


##### TEST #####
if __name__ == '__main__':
    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)
    HOSTS = []
    gobject.threads_init()

    SERVICE = OpenSandServiceListener(MODEL, HOSTS, [], '_opensand._tcp',
                                      LOGGER)
    try:
        gobject.MainLoop().run()
    except KeyboardInterrupt:
        LOGGER.debug('keyboard interrupt')
        for HOST in HOSTS:
            HOST.close()
        MODEL.close()
        gobject.MainLoop().quit()
        sys.exit(0)

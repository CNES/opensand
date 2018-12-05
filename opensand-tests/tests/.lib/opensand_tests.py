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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
opensand_tests.py - elements for OpenSAND tests
"""


import dbus, gobject, avahi
from dbus.mainloop.glib import DBusGMainLoop

TYPE = "_opensand._tcp"

def print_error(err):
    """ error hanlder """
    print err

class Service():
    """ listen for OpenSAND service with avahi and return info """
    def __init__(self, services = {}, error_handler=print_error):
        self._server = None
        self._mainloop = None
        self._services = services
        self._error_handler = error_handler

        loop = DBusGMainLoop()
        self._mainloop = gobject.MainLoop()

        bus = dbus.SystemBus(mainloop=loop)

        self._server = dbus.Interface(bus.get_object(avahi.DBUS_NAME, '/'),
                      'org.freedesktop.Avahi.Server')

        sbrowser = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
                self._server.ServiceBrowserNew(avahi.IF_UNSPEC,
                    avahi.PROTO_INET, TYPE, 'local', dbus.UInt32(0))),
                avahi.DBUS_INTERFACE_SERVICE_BROWSER)

        sbrowser.connect_to_signal("ItemNew", self.handler_new)
        sbrowser.connect_to_signal("AllForNow", self.handler_end)

        self._mainloop.run()

    def service_resolved(self, args):
        """ get the parameter of service once it is resolved """
        name = str(args[2])
        #print "Find %s at address %s" % (name, management_address)

        self._services[name] = {}
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
            self._services[name][key] = val

    def handler_new(self, interface, protocol, name, stype, domain, flags):
        """ handle a new service """
        #print "Found service '%s' type '%s' domain '%s'" % \
        #      (name, stype, domain)

        if flags & avahi.LOOKUP_RESULT_LOCAL:
            # local service, skip
            pass

        try:
            res = self._server.ResolveService(interface, protocol, name, stype,
                                              domain, avahi.PROTO_INET,
                                              dbus.UInt32(0))
        except dbus.DBusException, msg:
            if msg.get_dbus_name() == 'org.freedesktop.Avahi.TimeoutError':
                print str(msg)
            else:
                self._error_handler(str(msg))
        else:
            self.service_resolved(res)

    def handler_end(self):
        """ all service discovered: stop the program """
        #print "AllForNow signal: quit"
        self._mainloop.quit()

##### MAIN #####
if __name__ == '__main__':
    SERV = {}
    SERVICE = Service(SERV)
    print SERV


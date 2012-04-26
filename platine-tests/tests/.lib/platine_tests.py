#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
platine_tests.py - elements for Platine tests
"""


import dbus, gobject, avahi
from dbus.mainloop.glib import DBusGMainLoop

TYPE = "_platine2._tcp"

def print_error(err):
    """ error hanlder """
    print err

class Service():
    """ listen for Platine service with avahi and return info """
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


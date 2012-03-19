#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
ping.py - The ping test

Launch a ping command in destination to all satellite terminals
and workstations of the network and check the travel time
"""

import sys
import subprocess

import dbus, gobject, avahi
from dbus.mainloop.glib import DBusGMainLoop

TYPE = "_platine._tcp"
WS_NAME = "ws1_test"
WS_INSTANCE = "1"

class PingTest():
    """ listen for Platine service with avahi  and
        ping all ST and WS """

    returncode = 0

    def __init__(self, sat_type):
        self._type = '_platine._tcp'
        self._server = None
        self._mainloop = None
        self._sat = sat_type

        print "satellite type: %s" % sat_type

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

    def service_resolved(self, *args):
        """ get the parameter of service once it is resolved """
        name = args[2]
        management_address = args[7]
        print "Find %s at address %s" % (name, management_address)

        i = 0
        address_v4 = ''
        address_v6 = ''
        instance = 0
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
            if key == 'lan_ipv4':
                address_v4 = val
            if key == 'lan_ipv6':
                address_v6 = val
            if key == 'id':
                instance = val


        if ((name.startswith('ws') or name.startswith('st')) and \
            (name != WS_NAME and instance != WS_INSTANCE)) or \
           name.startswith('gw'):
            if address_v4 == '':
                self.print_error('no IPv4 lan address for %s' % name)
            else:
                self.ping(name, address_v4)
            if address_v6 == '':
                self.print_error('no IPv6 lan address for %s' % name)
            else:
                self.ping(name, address_v6, True)

    def print_error(self, *args):
        """ error handler """
        print 'service error handler: %s\n' % str(args[0])
        self.returncode = 1

    def handler_new(self, interface, protocol, name, stype, domain, flags):
        """ handle a new service """
        print "Found service '%s' type '%s' domain '%s'" % \
              (name, stype, domain)

        if flags & avahi.LOOKUP_RESULT_LOCAL:
            # local service, skip
            pass

        self._server.ResolveService(interface, protocol, name, stype,
                                    domain, avahi.PROTO_INET, dbus.UInt32(0),
                                    reply_handler=self.service_resolved,
                                    error_handler=self.print_error)


    def handler_end(self):
        """ all service discovered: stop the program """
        print "AllForNow signal: quit"
        self._mainloop.quit()

    def ping(self, name, address, v6=False):
        """ ping a st or ws """
        print "ping %s at address %s" % (name, address)
        cmd = 'ping'
        if v6:
            cmd = 'ping6'
        ping = subprocess.Popen([cmd, "-c", "1", address],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        out, err = ping.communicate()
        print out
        if err != '':
            print err + '\n'
        if ping.returncode != 0:
            self.returncode = ping.returncode
            print "ping returned %s\n" % str(ping.returncode)

        if ping.returncode == 0:
            # check that time is correct
            time = float(self.get_ping_time(out))
            if self._sat == 'transparent' and \
               (name != 'st2' and name != 'gw' and not name.startswith('ws2')):
                # time ~ 1200ms
                if time > 1400 or time < 1000:
                    print "bad ping time %s for %s\n" % (time, name)
                    self.returncode = 1
                else:
                    print "OK"
            else:
                # time ~ 600ms
                if time > 800 or time < 400:
                    print "bad ping time %s for %s\n" % (time, name)
                    self.returncode = 1
                else:
                    print "OK"


    def get_ping_time(self, msg):
        """ read the ping time from ping output """
        # get a line like this:
        # 64 bytes from 192.168.21.5: icmp_seq=1 ttl=64 time=1161 ms
        line = msg.split('\n')[1]
        # get time=1161
        time = line.split()[6]
        # get the time value
        val = time.split('=')[1]
        print "ping time: %s" % val

        return val



##### MAIN #####
if __name__ == '__main__':
    if len(sys.argv) > 1:
        test = PingTest(sys.argv[1])
    else:
        print "Usage: ping.py [sat_type] " \
              "with sat_type in {transparent, regenerative}\n"
        sys.exit(1)
    sys.exit(test.returncode)


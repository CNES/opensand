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


sys.path.append('../../.lib')
from platine_tests import Service

WS_NAME = "ws1_test"
WS_INSTANCE = "1"

class PingTest():
    """ listen for Platine service with avahi  and
        ping all ST and WS """

    returncode = 0

    def __init__(self, sat_type):
        self._sat = sat_type

        print "satellite type: %s" % sat_type

        services = {}
        Service(services, self.print_error)

        if len(services) == 0:
            self.print_error("error when getting Platine hosts")
            return

        address_v4 = ''
        address_v6 = ''
        instance = 0
        for name in services.keys():
            if 'id' in services[name]:
                instance = services[name]['id']
            if ((name.startswith('ws') or name.startswith('st')) and \
                (name != WS_NAME and instance != WS_INSTANCE)) or \
               name.startswith('gw'):
                if not 'lan_ipv4' in services[name]:
                    self.print_error('no IPv4 lan address for %s' % name)
                else:
                    address_v4 = services[name]['lan_ipv4']
                    address_v4 = address_v4.split("/")[0]
                    self.ping(name, address_v4)
                if not 'lan_ipv6' in services[name]:
                    self.print_error('no IPv6 lan address for %s' % name)
                else:
                    address_v6 = services[name]['lan_ipv6']
                    address_v6 = address_v6.split("/")[0]
                    self.ping(name, address_v6, True)

    def print_error(self, msg):
        """ error handler """
        print 'Error: %s\n' % msg
        self.returncode = 1

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
            self.print_error("ping returned %s\n" % str(ping.returncode))
            self.returncode = ping.returncode

        if ping.returncode == 0:
            # check that time is correct
            time = float(self.get_ping_time(out))
            if self._sat == 'transparent' and \
               (name != 'st2' and name != 'gw' and not name.startswith('ws2')):
                # time ~ 1200ms
                if time > 1400 or time < 1000:
                    self.print_error("bad ping time %s for %s\n" % (time, name))
                    self.returncode = 1
                else:
                    print "OK"
            else:
                # time ~ 600ms
                if time > 800 or time < 400:
                    self.print_error("bad ping time %s for %s\n" % (time, name))
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
        TEST = PingTest(sys.argv[1])
    else:
        print "Usage: ping.py [sat_type] " \
              "with sat_type in {transparent, regenerative}\n"
        sys.exit(1)
    sys.exit(TEST.returncode)


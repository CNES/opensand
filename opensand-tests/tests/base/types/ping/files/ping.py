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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
ping.py - The ping test

Launch a ping command in destination to all satellite terminals
and workstations of the network and check the travel time
"""

import sys
import subprocess
import time


sys.path.append('../../../../.lib')
from opensand_tests import Service

WS_NAME = "ws1_test"
WS_INSTANCE = "1"

class PingTest():
    """ listen for OpenSAND service with avahi  and
        ping all ST and WS """

    returncode = 0

    def __init__(self, test_name):
        if test_name.find("regen") >= 0:
            self._sat = "regenerative"
        elif test_name.find("transp") >= 0:
            self._sat = "transparent"
        else:
            self.print_error("Cannot find satellite type")
            return

        print "************************************************"
        print "**  NEW TEST (%s) **" % time.strftime("%c", time.gmtime())
        print "************************************************"
        print ""
        print "Name: %s => %s satellite" % (test_name, self._sat)
        print ""

        services = {}
        Service(services, self.print_error)

        if len(services) == 0:
            self.print_error("error when getting OpenSAND hosts")
            return

        # wait 10 second because for ethernet tests we wait for bridge to be
        # ready        
        time.sleep(10)

        address_v4 = ''
        address_v6 = ''
        instance = 0
        nbr_hosts = 0
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
                nbr_hosts += 1
        if nbr_hosts < 1:
            self.print_error("cannot find ST or WS to ping")

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
        # 3 pings because for Ethernet, the first is for ARP
        # and the second if often too long
        ping = subprocess.Popen([cmd, "-c", "3", address],
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
            if self._sat == 'transparent' and name != 'gw':
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
        # skip first line because for Ethernet the time is twice the RTT as we
        # need ARP
        line = msg.split('\n')[3]
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
        print "Usage: ping.py [test_name] " \
              "with test_name containing {transp or regen}\n"
        sys.exit(1)
    sys.exit(TEST.returncode)


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
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>

"""
ping_high_throughpu.py - The ping test

Launch a ping command in destination to all satellite terminals
and workstations of the network and check that no packet was lost
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
        print "************************************************"
        print "**  NEW TEST (%s) **" % time.strftime("%c", time.gmtime())
        print "************************************************"
        print ""
        print "Name: %s" % test_name
        print ""
        services = {}
        Service(services, self.print_error)

        if len(services) == 0:
            self.print_error("error when getting OpenSAND hosts")
            return

        # wait 10 seconds more because for ethernet tests we wait for bridge to be
        # ready        
        #TODO: increase time sleep for eth
        if test_name.find("eth") >= 0:
            time.sleep(20)
        else:
            time.sleep(1)

        address_v4 = ''
        address_v6 = ''
        instance = 0
        nbr_hosts = 0
        for name in services.keys():
            if 'id' in services[name]:
                instance = services[name]['id']
            if (((name.startswith('ws') or name.startswith('st')) and \
                (name != WS_NAME and instance != WS_INSTANCE)) or \
               name.startswith('gw')) and ('phy' not in name):
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

        # first ping twice for initialization that sometimes leads to errors
        ping = subprocess.Popen([cmd, "-c", "10", address],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        out, err = ping.communicate()
        print out

        # TODO it would be better to set a smaller interval but ping
        #      want us to be root
        ping = subprocess.Popen([cmd, "-c", "60", "-i", "0.2", "-s", "1400", "-q",
                                 address], stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        out, err = ping.communicate()
        print out
        if err != '':
            print err + '\n'
        if ping.returncode != 0:
            self.print_error("ping returned %s\n" % str(ping.returncode))
            self.returncode = ping.returncode

        if ping.returncode == 0:
            # check that no packet was lost
            lost = self.get_ping_loss(out)
            if lost > 0:
                self.print_error("%d packets lost for %s" % (lost, name))
            else:
                print "OK"


    def get_ping_loss(self, msg):
        """ read the ping loss from ping output """
        # get a line like this:
        # 2000 packets transmitted, 2000 received, 0% packet loss, time 21206ms
        line = msg.split('\n')[3]
        print line
        # get the difference between transmitted and received
        transmitted = int(line.split()[0])
        received = int(line.split()[3])
        return transmitted - received



##### MAIN #####
if __name__ == '__main__':
    TEST = PingTest(sys.argv[1])
    sys.exit(TEST.returncode)


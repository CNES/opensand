#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2017 TAS
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
iperf_client.py - iperf client for OpenSAND tests
"""

import subprocess
import shlex
import sys
import time

sys.path.append('../../../../.lib')
from opensand_tests import Service

# TODO we cannot get a correct output with bidirectionnal
#      test and binding... if possible, do bidir and use
#      GW as a server instead of client
#COMMAND = '/usr/bin/iperf -y c -d -t '
COMMAND = '/usr/bin/iperf -y c -t '
SERVER = ['st2'] #, 'gw']
TIME = 60

class IperfClient():
    """ iperf client for OpenSAND tests """

    returncode = 0

    def __init__(self, name, test_name, v6=False):
        print "************************************************"
        print "**  NEW TEST (%s) **" % time.strftime("%c", time.gmtime())
        print "************************************************"
        print ""
        print "Name: %s" % test_name
        print ""

        self._name = name

        services = {}
        Service(services, self.print_error)

        if len(services) == 0:
            self.print_error("error when getting OpenSAND hosts")
            return

        if test_name.find("regen") >= 0:
            self._sat = "regenerative"
        elif test_name.find("transp") >= 0:
            self._sat = "transparent"
        else:
            self.print_error("Cannot find satellite type")
            return

        address_v4 = ''
        address_v6 = ''
        for serv in SERVER:
            if not serv in services:
                self.print_error("cannot find iperf server: %s" % serv)
                return

            info = services[serv]

            # wait 11 seconds more because for ethernet tests we wait for bridge to be
            # ready        
            # TODO: increase time sleep for eth
            if test_name.find("eth") >= 0:
                time.sleep(20)
            else:
                time.sleep(1)

            if not v6 and not 'lan_ipv4' in info:
                self.print_error('no IPv4 lan address for %s' % serv)
            elif not v6:
                address_v4 = info['lan_ipv4']
                address_v4 = address_v4.split("/")[0]
                # first ping to initialize ARP in Ethernet mode
                # else we have loss
                if not self.ping(address_v4):
                    return
                self.iperf(address_v4)
            if v6 and not 'lan_ipv6' in info:
                self.print_error('no IPv6 lan address for %s' % serv)
            elif v6:
                address_v6 = info['lan_ipv6']
                address_v6 = address_v6.split("/")[0]
                # first ping to initialize ARP in Ethernet mode
                # else we have loss
                if not self.ping(address_v6, True):
                    return
                self.iperf(address_v6, True)
                
    def ping(self, address, v6=False, check_rtt=False, count=5):
        """ ping a st or ws """
        cmd = 'ping'
        if v6:
            cmd = 'ping6'
        # 3 pings because for Ethernet, the first is for ARP
        # and the second if often too long
        ping = subprocess.Popen([cmd, "-c", str(count), address],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        out, err = ping.communicate()
        print out
        if err != '':
            print err + '\n'
        if ping.returncode != 0:
            self.print_error("ping returned %s\n" % str(ping.returncode))
            #self.returncode = ping.returncode
            return False
        if not check_rtt:
            return True
        time = float(self.get_ping_time(out, count))
        if self._sat == 'transparent' and not self._name.startswith('gw'):
            # time ~1200 ms
            if time > 1400 or time < 1000:
                self.print_error("bad ping time %s\n" % (time))
                self.returncode = 1 
            else:
                print "OK"
        else:
            # time ~ 600ms
            if time > 800 or time < 400:
                self.print_error("bad ping time %s\n" % (time))
            else:
                print "OK"

    def get_ping_time(self, msg, count):
        """ read the ping time from ping output """
        # get a line like this:
        # 64 bytes from 192.168.21.5: icmp_seq=1 ttl=64 time=1161 ms
        # skip first line because for Ethernet the time is twice the RTT as we
        # need ARP
        line = msg.split('\n')[count - 2]
        # get time=1161
        time = line.split()[6]
        # get the time value
        val = time.split('=')[1]
        print "ping time: %s" % val

        return val

    def iperf(self, address, v6=False):
        """ launch an iperf """
        cmd = "%s %s -c %s %s -ub 6M" % \
              (COMMAND, TIME, address, ('-V' if v6 else ''))
        command = shlex.split(cmd)
        iperf = subprocess.Popen(command, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE,
                                 close_fds=True, shell=False)
        time.sleep(TIME + 10)
        iperf.terminate()
        out, err = iperf.communicate()
        print out
        #if err != '':
        #    self.print_error(err + '\n')
        # check the RTT with ping
        return self.ping(address, v6, check_rtt=True, count=40)

    def print_error(self, *args):
        """ error handler """
        print 'Error: %s\n' % str(args[0])
        self.returncode = 1


##### MAIN #####
if __name__ == '__main__':
    TEST = IperfClient(sys.argv[1], sys.argv[2])
    sys.exit(TEST.returncode)


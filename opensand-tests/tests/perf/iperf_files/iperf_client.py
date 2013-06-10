#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
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

sys.path.append('../../.lib')
from opensand_tests import Service

COMMAND = '/usr/bin/iperf -y c -t '
SERVER = 'st3'
TIME = 40

class IperfClient():
    """ iperf client for OpenSAND tests """

    returncode = 0

    def __init__(self, v6=False):
        print "************************************************"
        print "**  NEW TEST (%s) **" % time.strftime("%c", time.gmtime())
        print "************************************************"
        print ""

        services = {}
        Service(services, self.print_error)

        if len(services) == 0:
            self.print_error("error when getting OpenSAND hosts")
            return

        address_v4 = ''
        address_v6 = ''
        if not SERVER in services:
            self.print_error("cannot find iperf server: %s" % SERVER)
            return

        info = services[SERVER]

        if not v6 and not 'lan_ipv4' in info:
            self.print_error('no IPv4 lan address for %s' % SERVER)
        elif not v6:
            address_v4 = info['lan_ipv4']
            address_v4 = address_v4.split("/")[0]
            self.iperf(address_v4)
        if v6 and not 'lan_ipv6' in info:
            self.print_error('no IPv6 lan address for %s' % SERVER)
        elif v6:
            address_v6 = info['lan_ipv6']
            address_v6 = address_v6.split("/")[0]
            self.iperf(address_v6, True)

    def iperf(self, address, v6=False):
        """ launch an iperf """
        cmd = "%s %s -c %s %s -ub 800k" % \
              (COMMAND, TIME, address, ('-V' if v6 else ''))
        command = shlex.split(cmd)
        iperf = subprocess.Popen(command, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE,
                                 close_fds=True, shell=False)
        time.sleep(TIME + 5)
        iperf.terminate()
        out, err = iperf.communicate()
        print out
        if err != '':
            self.print_error(err + '\n')

        bw = self.get_bandwidth(out)
        if bw is None or bw == '':
            return
        bandwidth = float(bw)
        if bandwidth < 700000:
            self.print_error("not enough throughput: %s" % bandwidth)
            self.returncode = 1
        else:
            print "OK"

    def get_bandwidth(self, msg):
        """ read the bandwidth from iperf output """
        # the -y option allow to get the iperf result in the following format:
        # date,ip source,port source,ip dest,port dest,id,interval,transfer,
        # bandwidth(bits/s)
        bdw = 0
        try:
            bdw = msg.split('\n')[1]
            bdw = bdw.split(',')[8]
        except IndexError:
            self.print_error("cannot parse iperf output (%s)" % msg)
            return None

        return bdw

    def print_error(self, *args):
        """ error handler """
        print 'Error: %s\n' % str(args[0])
        self.returncode = 1


##### MAIN #####
if __name__ == '__main__':
    TEST = IperfClient()
    sys.exit(TEST.returncode)


#!/usr/bin/env python 
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
iperf_client.py - iperf client for Platine tests
"""

import subprocess
import shlex
import sys
import time

sys.path.append('../../.lib')
from platine_tests import Service

COMMAND = '/usr/bin/iperf -y c -t '
SERVER = 'st3'
TIME = 40

class IperfClient():
    """ iperf client for Platine tests """

    returncode = 0

    def __init__(self, v6=False):
        services = {}
        Service(services, self.print_error)

        if len(services) == 0:
            self.print_error("error when getting Platine hosts")
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

        bandwidth = float(self.get_bandwidth(out))
        if not bandwidth:
            return
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

        return bdw

    def print_error(self, *args):
        """ error handler """
        print 'Error: %s\n' % str(args[0])
        self.returncode = 1


##### MAIN #####
if __name__ == '__main__':
    TEST = IperfClient()
    sys.exit(TEST.returncode)


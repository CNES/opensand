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
iperf_server.py - iperf server for OpenSAND tests
"""

import os
import sys
import subprocess
import shlex
import time
import ConfigParser

COMMAND = '/usr/bin/iperf -su -B '
CONF_FILE = "/etc/opensand/daemon.conf"

if __name__ == '__main__':
    """ iperf server for Platine tests """
    print "************************************************"
    print "**  NEW TEST (%s) **" % time.strftime("%c", time.gmtime())
    print "************************************************"
    print ""
    print "Name: %s" % sys.argv[1]
    print ""

    if not os.path.exists(CONF_FILE):
        print >> sys.stderr, "configuration file '%s' does not exist" % CONF_FILE
        sys.exit(1)

    parser = ConfigParser.SafeConfigParser()
    # read configuration file
    if len(parser.read(CONF_FILE)) == 0:
        print >> sys.stderr, "cannot parser configuration file", CONF_FILE
        sys.exit(1)

    try:
        lan_ipv4 = parser.get('network', 'lan_ipv4')
        lan_ipv4 = lan_ipv4.split("/")[0]
#        lan_ipv6 = parser.get('network', 'lan_ipv6')
    except ConfigParser.Error, error:
        print >> sys.stderr, "unable to parse configuration file:", str(error)
        sys.exit(1)

    command = shlex.split(COMMAND + lan_ipv4)
    iperf = subprocess.Popen(command, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             close_fds=True, shell=False)
    print "wait for connections"
    time.sleep(60)
    print "stop now"
    try:
        iperf.terminate()
        # second terminate if a client is still connect
        time.sleep(2)
        iperf.terminate()
        out, err = iperf.communicate()
        print out
        iperf.wait()
    except OSError, (errno, strerror):
        pass



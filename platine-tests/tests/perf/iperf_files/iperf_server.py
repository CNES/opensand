#!/usr/bin/env python 
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
iperf_server.py - iperf server for Platine tests
"""

import os
import sys
import subprocess
import shlex
import time
import ConfigParser

COMMAND = '/usr/bin/iperf -s -u -B '
CONF_FILE = "/etc/platine/daemon.conf"

if __name__ == '__main__':
    """ iperf server for Platine tests """
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



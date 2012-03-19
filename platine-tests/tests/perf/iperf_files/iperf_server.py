#!/usr/bin/env python 
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
iperf_server.py - iperf server for Platine tests
"""

import subprocess
import shlex
import time

COMMAND = '/usr/bin/iperf -s'

if __name__ == '__main__':
    """ iperf server for Platine tests """
    command = shlex.split(COMMAND)
    iperf = subprocess.Popen(command, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
    print "wait for connections"
    time.sleep(130)
    print "stop now"
    try:
        iperf.kill()
        out, err = iperf.communicate()
        print out
        iperf.wait()
    except OSError, (errno, strerror):
        pass



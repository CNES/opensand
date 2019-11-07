#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>

"""
run_output_tests.py - Start the output test program and checks
                      that it produces the correct messages
"""

from __future__ import print_function

import itertools
import json
import os
import re
import select
import signal
import socket
import subprocess
import sys


COMMAND = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test_output")


def grouper(iterable, n):
    args = [iter(iterable)] * n
    return itertools.izip(*args)


class MessageSendProbes(object):
    def __init__(self, data):
        self.values = {}
        msg = iter(data.split())
        timestamp = next(msg)
        for name, value in grouper(msg, 2):
            self.values[name] = json.loads(value)

    def assert_values(self, values):
        assert self.values == values, self.values

    def __repr__(self):
        return "<MessageSendProbes: %r>" % self.values


class MessageSendLog(object):
    pattern = re.compile(r'\[(?P<timestamp>[^\]]+)\]\[\s*(?P<log_level>\w+)\] (?P<log_name>[^:]+): (?P<log_message>.*)')

    def __init__(self, data):
        self.timestamp = None
        self.log_level = None
        self.log_name = None
        self.log_message = None
        match = self.pattern.match(data)
        if match:
            vars(self).update(match.groupdict())
        del self.timestamp
        self.original_message = data

    def assert_values(self, level, name, message):
        assert self.log_level == level, 'Log level mismatch (expected {}): {}'.format(level, self)
        assert self.log_name == name, 'Log name mismatch (expected {}): {}'.format(name, self)
        assert self.log_message == message, 'Log message mismatch (expected {}): {}'.format(message, self)

    def __repr__(self):
        return "<MessageSendLog: %r (%r) - %r>" % (self.log_name,
                                                   self.log_level,
                                                   self.log_message)


class EnvironmentPlaneBaseTester(object):
    def __init__(self, startup_arg=None):
        self.socket = None
        self.proc = None
        self.command = [COMMAND, 'localhost', startup_arg] if startup_arg else [COMMAND, 'localhost']

    def __enter__(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(('', 58008))
        signal.signal(signal.SIGCHLD, self.sigchld_caught)

        self.proc = subprocess.Popen(self.command, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, bufsize=0)

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        try:
            self.proc.terminate()
        except OSError:
            pass
        self.socket.close()

    def sigchld_caught(self, signum, sigframe):
        if not self.proc:
            return

        self.proc.wait()

        retcode = self.proc.returncode
        if retcode < 0:
            try:
                sig_name = signal.Signals(-retcode).name
            except ValueError:
                sig_name = '?'
            print("*** Test process was killed by signal", -retcode, "({})".format(sig_name))
        elif retcode > 0:
            print("*** Test process exited with non-zero status", retcode)
        else:
            return

        print("Check /var/log/syslog for details.")


    def assert_line(self, expected_line, timeout=5):
        rlist, _, _ = select.select([self.proc.stdout], [], [], timeout)
        if not self.proc.stdout in rlist:
            assert False, 'Timeout'

        read = self.proc.stdout.readline()
        assert read == expected_line, 'Expected {}; Got {}'.format(expected_line, read)

    def send_cmd(self, vlast, vmin, vmax, vavg, vsum, vdis,
                 vfloat, vdouble, cmd):
        self.proc.stdin.write("%d %d %d %d %d %d %f %lf %c\n" %
                              (vlast, vmin, vmax, vavg, vsum, vdis,
                               vfloat, vdouble, cmd))

    def send_quit(self):
        self.proc.stdin.write(">\n")
        self.proc.stdin.close()

    def get_message(self, expected_type):
        try:
            packet, addr = self.socket.recvfrom(4096)
        except socket.error as e:
            print("Error reading from socket:", e, file=sys.stderr)
            sys.exit(1)

        if packet.startswith('['):
            assert issubclass(MessageSendLog, expected_type), 'Expected {}; Got MessageSendLog'.format(expected_type)
            return MessageSendLog(packet)
        else:
            assert issubclass(MessageSendProbes, expected_type), 'Expected {}; Got MessageSendProbes'.format(expected_type)
            return MessageSendProbes(packet)

    def assert_no_msg(self):
        rlist, _, _ = select.select([self.socket], [], [], .5)
        assert not rlist, "Unexpected message received: %s" % self.socket.recv(4096)

    def check_startup(self, min_level=7):
        print("Test: Startup")
        
        self.assert_line("init\n")
        self.assert_line("fin_init\n")
        self.assert_line("start\n")

    def check_quit(self):
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        print("Test: Quit")
        self.send_quit()
        self.assert_line("quit\n")
        signal.alarm(2)
        self.proc.wait()
        signal.alarm(0)

    def check_info_log(self):
        print("Test: Info log")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "i")
        self.assert_line("info\n")
        msg = self.get_message(MessageSendLog)
        msg.assert_values('INFO', 'info', '[test_output.cpp:main():165] This is the info log message.')

    def check_default_log(self):
        print("Test: default log")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "t")
        self.assert_line("default log\n")
        msg = self.get_message(MessageSendLog)
        msg.assert_values('ERROR', 'default', 'This is a default log message.')


class EnvironmentPlaneNormalTester(EnvironmentPlaneBaseTester):
    def __init__(self):
        super(EnvironmentPlaneNormalTester, self).__init__()

    def check_no_msg(self):
        print("Test: No message if no probe values to send")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0.0, 0.0, "s")
        self.assert_line("send\n")

        self.assert_no_msg();

    def check_one_probe(self):
        print("Test: One-probe message")

        self.send_cmd(42, 0, 0, 0, 0, 0, 0.0, 0.0, "s")
        self.assert_line("send\n")

        msg = self.get_message(MessageSendProbes)
        msg.assert_values({"testing.int32_last_probe": 42})

    def check_all_probes(self):
        print("Test: All probes")

        self.send_cmd(100, 100, 100, 100, 100, 100, 3.1415, 2.7182, "x")
        self.send_cmd(-1, -1, -1, -1, -1, -1, 0, 0, "x")
        self.send_cmd(42, 42, 42, 42, 42, 42, 0, 0, "s")

        self.assert_line("send\n")

        msg = self.get_message(MessageSendProbes)
        msg.values["testing.float_probe"] = round(msg.values["testing.float_probe"], 4)
        msg.values["testing.double_probe"] = round(msg.values["testing.double_probe"], 4)
        msg.assert_values({
            "testing.int32_last_probe": 42,        # Last value
            "testing.int32_max_probe": 100,        # Max value
            "testing.int32_min_probe": -1,        # Min value
            "testing.int32_avg_probe": 47,        # Average int value
            "testing.int32_sum_probe": 141,        # Sum
            # Probe testing.int32_dis_probe is disabled by default
            "testing.float_probe": 3.1415,
            "testing.double_probe": 2.7182,
        })

    def check_disabling_probes(self):
        print("Test: Disabling and enabling probes")

        self.send_cmd(-42, 0, 0, 0, 0, 42, 0, 0, "s")
        self.assert_line("send\n")
        msg = self.get_message(MessageSendProbes)
        msg.assert_values({"testing.int32_last_probe": -42})

        self.send_cmd(0, 1, 1, 1, 1, 1, 1, 1, "e")
        self.assert_line("enable/disable probes\n")

        self.send_cmd(-42, 0, 0, 0, 0, 42, 0, 0, "s")
        self.assert_line("send\n")
        msg = self.get_message(MessageSendProbes)
        msg.assert_values({"testing.int32_dis_probe": 42})

        self.send_cmd(1, 1, 1, 1, 1, 0, 1, 1, "e")
        self.assert_line("enable/disable probes\n")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "s")
        self.assert_line("send\n")
        self.assert_no_msg()

    def check_debug_log(self):
        print("Test: Debug log")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "d")
        self.assert_line("debug\n")
        msg = self.get_message(MessageSendLog)
        msg.assert_values('DEBUG', 'debug', '[test_output.cpp:main():159] This is a debug log message.')

    def run(self):
        self.check_startup()
        self.check_no_msg()
        self.check_one_probe()
        self.check_all_probes()
        self.check_disabling_probes()
        self.check_debug_log()
        self.check_info_log()
        self.check_default_log()
        self.check_quit()


class EnvironmentPlaneDisabledTester(EnvironmentPlaneBaseTester):
    def __init__(self):
        super(EnvironmentPlaneDisabledTester, self).__init__("disable")

    def check_startup(self):
        print("Test: Startup")
        self.assert_line("init\n")
        self.assert_line("fin_init\n")
        self.assert_no_msg()

        self.assert_line("start\n")

    def check_all_probes(self):
        print("Test: All probes")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "x")
        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "x")
        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "s")

        self.assert_line("send\n")

        self.assert_no_msg()

    def check_debug_log(self):
        print("Test: Debug log")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "d")
        self.assert_line("debug\n")
        self.assert_no_msg()

    def check_info_log(self):
        print("Test: Info log")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "i")
        self.assert_line("info\n")
        self.assert_no_msg()

    def check_default_log(self):
        print("Test: default log")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "t")
        self.assert_line("default log\n")
        self.assert_no_msg()

    def run(self):
        self.check_startup()
        self.check_all_probes()
        self.check_debug_log()
        self.check_info_log()
        self.check_default_log()
        self.check_quit()


class EnvironmentPlaneNoDebugTester(EnvironmentPlaneBaseTester):
    def __init__(self):
        super(EnvironmentPlaneNoDebugTester, self).__init__("nodebug")

    def check_all_probes(self):
        print("Test: All probes")

        self.send_cmd(100, 100, 100, 100, 100, 100, 3.1415, 2.7182, "x")
        self.send_cmd(-1, -1, -1, -1, -1, -1, 0, 0, "x")
        self.send_cmd(42, 42, 42, 42, 42, 42, 0, 0, "s")

        self.assert_line("send\n")

        self.assert_no_msg()

    def check_debug_log(self):
        print("Test: Debug log")

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "d")
        self.assert_line("debug\n")
        self.assert_no_msg()

    def run(self):
        self.check_startup(6)
        self.check_debug_log()
        self.check_info_log()
        self.check_default_log()
        self.check_quit()

if __name__ == '__main__':
    print("* Normal startup:")
    with EnvironmentPlaneNormalTester() as tester:
        tester.run()

    print("* Startup with disabled output:")
    with EnvironmentPlaneDisabledTester() as tester:
        tester.run()

    print("* Startup with no-debug output:")
    with EnvironmentPlaneNoDebugTester() as tester:
        tester.run()

    print("All tests passed.")

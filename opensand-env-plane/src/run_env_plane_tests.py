#!/usr/bin/env python
# -*- coding: utf8 -*-

from os.path import exists, isdir
import atexit
import os
import select
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time

BASE_PATH = tempfile.mkdtemp()
DAEMON_SOCK_PATH = BASE_PATH + "/sand-daemon.socket"
PROG_SOCK_PATH_FMT = BASE_PATH + "/program-%d.socket"
COMMAND = "./test_env_plane"
MAGIC_NUMBER = 0x5A7D0001

MSG_CMD_REGISTER = 1
MSG_CMD_ACK = 2
MSG_CMD_SEND_PROBES = 3
MSG_CMD_SEND_EVENT = 4
MSG_CMD_ENABLE_PROBE = 5
MSG_CMD_DISABLE_PROBE = 6

TYPE_INT = 0
TYPE_FLOAT = 1
TYPE_DOUBLE = 2

LEVEL_DEBUG = 0
LEVEL_INFO = 1
LEVEL_WARNING = 2
LEVEL_ERROR = 3

class MessageRegister(object):
    def __init__(self, data):
        self.pid, num_probes, num_events = struct.unpack("!LBB", data[0:6])
        assert num_probes, "Got 0 probes"
        assert num_events, "Got 0 events"

        self.probes = []
        self.events = []

        pos = 6
        for _ in xrange(num_probes):
            storage_type, length = struct.unpack("!BB", data[pos:pos + 2])
            pos += 2
            enabled = bool(storage_type & (1 << 7))
            storage_type = (storage_type & ~(1 << 7))
            name = data[pos:pos + length]
            assert len(name) == length, "Incorrect length during string unpacking"
            pos += length

            self.probes.append((name, enabled, storage_type))

        for _ in xrange(num_events):
            level, length = struct.unpack("!BB", data[pos:pos + 2])
            pos += 2
            ident = data[pos:pos + length]
            assert len(ident) == length, "Incorrect length during string unpacking"
            pos += length

            self.events.append((ident, level))

        assert data[pos:] == "", "Garbage data found after string unpacking"

    def __repr__(self):
        return "<MessageRegister: %r, %r (PID %d)>" % (self.probes, self.events, self.pid)


class MessageSendProbes(object):
    def __init__(self, probe_types, data):
        self.values = {}
        length = len(data)
        pos = 0
        while pos < length:
            probe_id = struct.unpack("!B", data[pos])[0]
            probe_type = probe_types[probe_id]
            pos += 1

            if probe_type == 0:
                value = struct.unpack("!i", data[pos:pos + 4])[0]
                pos += 4

            elif probe_type == 1:
                value = struct.unpack("!f", data[pos:pos + 4])[0]
                pos += 4

            elif probe_type == 2:
                value = struct.unpack("!d", data[pos:pos + 8])[0]
                pos += 8

            else:
                raise Exception("Unknown storage type")

            self.values[probe_id] = value


    def __repr__(self):
        return "<MessageSendProbes: %r>" % self.values


class MessageSendEvent(object):
    def __init__(self, data):
        self.event_id = struct.unpack("!B", data[0:1])[0]
        self.event_message = data[1:]

    def __repr__(self):
        return "<MessageSendEvent: %d - %r>" % (self.event_id, self.event_message)


class EnvironmentPlaneBaseTester(object):
    def __init__(self, startup_arg=None):
        self.socket = None
        self.proc = None
        self.probe_types = None
        self.command = [COMMAND, BASE_PATH, startup_arg] if startup_arg else [COMMAND, BASE_PATH]

    def __enter__(self):
        try:
            os.unlink(DAEMON_SOCK_PATH)
        except OSError:
            pass

        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        self.socket.bind(DAEMON_SOCK_PATH)
        signal.signal(signal.SIGCHLD, self.sigchld_caught)

        self.proc = subprocess.Popen(self.command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, bufsize=0)
        self.program_sock_path = PROG_SOCK_PATH_FMT % self.proc.pid

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        try:
            self.proc.terminate()
        except OSError:
            pass
        self.socket.shutdown(socket.SHUT_RDWR)
        self.socket.close()
        if exists(DAEMON_SOCK_PATH):
            os.unlink(DAEMON_SOCK_PATH)

    def sigchld_caught(self, signum, sigframe):
        if not self.proc:
            return

        self.proc.wait()

        sig_names = dict((k, v) for v, k in signal.__dict__.iteritems() if v.startswith('SIG'))
        retcode = self.proc.returncode

        if retcode < 0:
            print "*** Test process was killed by signal %d (%s)!" % (-retcode, sig_names.get(-retcode, '?'))
        elif retcode > 0:
            print "*** Test process exited with non-zero status %d!" % (retcode)
        else:
            return

        print "Check /var/log/syslog for details."


    def get_line(self, timeout=1):
        rlist, _, _ = select.select([self.proc.stdout], [], [], timeout)
        if not self.proc.stdout in rlist:
            return "<timeout>"

        return self.proc.stdout.readline()

    def send_cmd(self, vlast, vmin, vmax, vavg, vsum, vdis, vfloat, vdouble, cmd):
        self.proc.stdin.write("%d %d %d %d %d %d %f %lf %c\n" % (vlast, vmin, vmax, vavg, vsum, vdis, vfloat, vdouble, cmd))

    def send_quit(self):
        self.proc.stdin.write("\n")
        self.proc.stdin.close()

    def get_message(self):
        try:
            packet, addr = self.socket.recvfrom(4096)
        except socket.error, e:
            print "Error reading from socket: %s" % e
            sys.exit(1)
        assert addr == self.program_sock_path, "Got message from unexpected socket path"

        magic, cmd = struct.unpack("!LB", packet[0:5])
        assert magic == MAGIC_NUMBER, "Received message with bad magic number"

        if cmd == MSG_CMD_REGISTER:
            return MessageRegister(packet[5:])

        if cmd == MSG_CMD_SEND_PROBES:
            return MessageSendProbes(self.probe_types, packet[5:])

        if cmd == MSG_CMD_SEND_EVENT:
            return MessageSendEvent(packet[5:])

        raise Exception("Unknown command ID %d" % cmd)

    def assert_no_msg(self):
        rlist, _, _ = select.select([self.socket], [], [], .5)
        assert not rlist, "Unexpected message received"

    def check_startup(self):
        print "Test: Startup"
        assert self.get_line() == "init\n"
        assert self.get_line() == "fin_init\n"
        msg = self.get_message()
        assert isinstance(msg, MessageRegister)
        assert msg.pid == self.proc.pid
        assert msg.probes == [
            ("int32_last_probe", True, TYPE_INT),
            ("int32_max_probe", True, TYPE_INT),
            ("int32_min_probe", True, TYPE_INT),
            ("int32_avg_probe", True, TYPE_INT),
            ("int32_sum_probe", True, TYPE_INT),
            ("int32_dis_probe", False, TYPE_INT),

            ("float_probe", True, TYPE_FLOAT),
            ("double_probe", True, TYPE_DOUBLE),
        ]
        assert msg.events == [
            ("debug_event", LEVEL_DEBUG),
            ("info_event", LEVEL_INFO),
        ]

        self.probe_types = [t for _, _, t in msg.probes]

        self.socket.sendto(struct.pack("!LB", MAGIC_NUMBER, MSG_CMD_ACK), self.program_sock_path)

        line = self.get_line()
        assert line == "start\n", "Unexpected line %r" % line

    def check_quit(self):
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        print "Test: Quit"
        self.send_quit()
        assert self.get_line() == "quit\n"
        signal.alarm(2)
        self.proc.wait()
        signal.alarm(0)

class EnvironmentPlaneNormalTester(EnvironmentPlaneBaseTester):
    def __init__(self):
        super(EnvironmentPlaneNormalTester, self).__init__()

    def check_no_msg(self):
        print "Test: No message if no probe values to send"

        self.send_cmd(0, 0, 0, 0, 0, 0, 0.0, 0.0, "s")
        assert self.get_line() == "send\n"

        self.assert_no_msg();

    def check_one_probe(self):
        print "Test: One-probe message"

        self.send_cmd(42, 0, 0, 0, 0, 0, 0.0, 0.0, "s")
        assert self.get_line() == "send\n"

        msg = self.get_message()
        assert isinstance(msg, MessageSendProbes)
        assert msg.values == {0: 42}

    def check_all_probes(self):
        print "Test: All probes"

        self.send_cmd(100, 100, 100, 100, 100, 100, 3.1415, 2.7182, "x")
        self.send_cmd(-1, -1, -1, -1, -1, -1, 0, 0, "x")
        self.send_cmd(42, 42, 42, 42, 42, 42, 0, 0, "s")

        assert self.get_line() == "send\n"

        msg = self.get_message()
        assert isinstance(msg, MessageSendProbes)
        msg.values[6] = round(msg.values[6], 4)
        msg.values[7] = round(msg.values[7], 4)
        assert msg.values == {
            0: 42,        # Last value
            1: 100,        # Max value
            2: -1,        # Min value
            3: 47,        # Average int value
            4: 141,        # Sum
            # Probe 5 is disabled by default
            6: 3.1415,
            7: 2.7182,
        }

    def check_disabling_probes(self):
        print "Test: Disabling and enabling probes"

        self.send_cmd(-42, 0, 0, 0, 0, 42, 0, 0, "s")
        assert self.get_line() == "send\n"
        msg = self.get_message()
        assert isinstance(msg, MessageSendProbes)
        assert msg.values == {0: -42}

        self.socket.sendto(struct.pack("!LBB", MAGIC_NUMBER, MSG_CMD_ENABLE_PROBE, 5), self.program_sock_path)
        self.socket.sendto(struct.pack("!LBB", MAGIC_NUMBER, MSG_CMD_DISABLE_PROBE, 0), self.program_sock_path)
        time.sleep(.1)

        self.send_cmd(-42, 0, 0, 0, 0, 42, 0, 0, "s")
        assert self.get_line() == "send\n"
        msg = self.get_message()
        assert isinstance(msg, MessageSendProbes)
        assert msg.values == {5: 42}

        self.socket.sendto(struct.pack("!LBB", MAGIC_NUMBER, MSG_CMD_DISABLE_PROBE, 5), self.program_sock_path)
        self.socket.sendto(struct.pack("!LBB", MAGIC_NUMBER, MSG_CMD_ENABLE_PROBE, 0), self.program_sock_path)

        time.sleep(.1)

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "s")
        assert self.get_line() == "send\n"
        self.get_message()

    def check_debug_event(self):
        print "Test: Debug event"

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "d")
        assert self.get_line() == "debug\n"
        msg = self.get_message()
        assert isinstance(msg, MessageSendEvent)
        assert msg.event_id == 0
        assert msg.event_message == "This is the debug event message."

    def check_info_event(self):
        print "Test: Info event"

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "i")
        assert self.get_line() == "info\n"
        msg = self.get_message()
        assert isinstance(msg, MessageSendEvent)
        assert msg.event_id == 1
        assert msg.event_message == "This is the info event message."

    def run(self):
        self.check_startup()
        self.check_no_msg()
        self.check_one_probe()
        self.check_all_probes()
        self.check_disabling_probes()
        self.check_debug_event()
        self.check_info_event()
        self.check_quit()

class EnvironmentPlaneDisabledTester(EnvironmentPlaneBaseTester):
    def __init__(self):
        super(EnvironmentPlaneDisabledTester, self).__init__("disable")

    def check_startup(self):
        print "Test: Startup"
        assert self.get_line() == "init\n"
        assert self.get_line() == "fin_init\n"
        self.assert_no_msg()

        assert self.get_line() == "start\n"

    def check_all_probes(self):
        print "Test: All probes"

        self.send_cmd(100, 100, 100, 100, 100, 100, 3.1415, 2.7182, "x")
        self.send_cmd(-1, -1, -1, -1, -1, -1, 0, 0, "x")
        self.send_cmd(42, 42, 42, 42, 42, 42, 0, 0, "s")

        assert self.get_line() == "send\n"

        self.assert_no_msg()

    def check_debug_event(self):
        print "Test: Debug event"

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "d")
        assert self.get_line() == "debug\n"
        self.assert_no_msg()

    def check_info_event(self):
        print "Test: Info event"

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "i")
        assert self.get_line() == "info\n"
        self.assert_no_msg()

    def run(self):
        self.check_startup()
        self.check_all_probes()
        self.check_debug_event()
        self.check_info_event()
        self.check_quit()

class EnvironmentPlaneNoDebugTester(EnvironmentPlaneBaseTester):
    def __init__(self):
        super(EnvironmentPlaneNoDebugTester, self).__init__("nodebug")

    def check_all_probes(self):
        print "Test: All probes"

        self.send_cmd(100, 100, 100, 100, 100, 100, 3.1415, 2.7182, "x")
        self.send_cmd(-1, -1, -1, -1, -1, -1, 0, 0, "x")
        self.send_cmd(42, 42, 42, 42, 42, 42, 0, 0, "s")

        assert self.get_line() == "send\n"

        self.assert_no_msg()

    def check_debug_event(self):
        print "Test: Debug event"

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "d")
        assert self.get_line() == "debug\n"
        self.assert_no_msg()

    def check_info_event(self):
        print "Test: Info event"

        self.send_cmd(0, 0, 0, 0, 0, 0, 0, 0, "i")
        assert self.get_line() == "info\n"
        msg = self.get_message()
        assert isinstance(msg, MessageSendEvent)
        assert msg.event_id == 1
        assert msg.event_message == "This is the info event message."

    def run(self):
        self.check_startup()
        self.check_debug_event()
        self.check_info_event()
        self.check_quit()

if __name__ == '__main__':
    atexit.register(shutil.rmtree, BASE_PATH)

    print "* Normal startup:"
    with EnvironmentPlaneNormalTester() as tester:
        tester.run()

    print "* Startup with disabled environment plane:"
    with EnvironmentPlaneDisabledTester() as tester:
        tester.run()

    print "* Startup with no-debug environment plane:"
    with EnvironmentPlaneNoDebugTester() as tester:
        tester.run()

    print "All tests passed."

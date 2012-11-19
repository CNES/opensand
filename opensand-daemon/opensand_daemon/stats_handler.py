#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 TAS
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

# Author: Vincent DUVERT / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
stats_handler.py - handler for environment plane stats management
"""

import logging
import os
import select
import socket
import struct
import threading

from opensand_daemon.process_list import ProcessList

LOGGER = logging.getLogger('sand-daemon')

MAGIC_NUMBER = 0x5A7D0001
DAEMON_SOCK_PATH = "/var/run/sand-daemon/sand-daemon.socket"

MSG_CMD_REGISTER = 1
MSG_CMD_ACK = 2
MSG_CMD_SEND_PROBES = 3
MSG_CMD_SEND_EVENT = 4
MSG_CMD_ENABLE_PROBE = 5
MSG_CMD_DISABLE_PROBE = 6
MSG_CMD_UNREGISTER = 7
MSG_CMD_RELAY = 10

REL_MSGS_PROG_TO_COL = frozenset([MSG_CMD_SEND_PROBES, MSG_CMD_SEND_EVENT])


class StatsHandler(threading.Thread):
    """
    Stats handler class.
    """

    def __init__(self, uid):
        super(StatsHandler, self).__init__()

        self._process_list = ProcessList()
        self._process_list.register_end_callback(self._proc_stopped)
        self._collector_addr = None
        self._running = False
        self._stopped = False

        # Internal socket for processes
        self._int_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        try:
            os.unlink(DAEMON_SOCK_PATH)
        except OSError:
            pass
        self._int_socket.bind(DAEMON_SOCK_PATH)
        os.chown(DAEMON_SOCK_PATH, uid, 0)

        # External socket for communication with the collector
        self._ext_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._ext_socket.bind(('', 0))

    def cleanup(self):
        """
        Closes the sockets used by the stats handler. This will automatically
        tear down the listener thread if it is running.
        """
        self._running = False

        if self._stopped:
            return

        self._stopped = True

        try:
            self._int_socket.shutdown(socket.SHUT_RDWR)
        except socket.error:
            pass

        try:
            self._ext_socket.shutdown(socket.SHUT_RDWR)
        except socket.error:
            pass

        self._int_socket.close()
        self._ext_socket.close()

        try:
            os.unlink(DAEMON_SOCK_PATH)
        except OSError:
            pass

    def get_external_port(self):
        """
        Returns the UDP port opened by the stats handler for communication with
        the collector.
        """
        return self._ext_socket.getsockname()[1]

    def set_collector_addr(self, address, port):
        """
        Set the address used to send messages to the collector.
        """
        self._collector_addr = (address, port)
        LOGGER.debug("Collector address set to %s:%d", address, port)

    def unset_collector_addr(self):
        """
        Unset the address of the collector.
        """
        if not self._collector_addr:
            return

        self._collector_addr = None
        LOGGER.debug("Collector address unset")

    def run(self):
        """
        The handler thread process.
        """
        self._running = True

        LOGGER.info("Starting the stats handler")

        try:
            while self._running:
                self._handle()
        except StandardError:
            LOGGER.exception("Exception in stats handler:")

        LOGGER.info("Stats handler exiting...")
        self.cleanup()

    def _handle(self):
        """
        Handles a command received on one of the two sockets.
        """
        rlist, _, _ = select.select([self._int_socket,
            self._ext_socket], [], [])

        if self._int_socket in rlist:
            self._handle_prog_message()

        if self._running and self._ext_socket in rlist:
            self._handle_collector_message()

    def _handle_prog_message(self):
        """
        Handles a command from one local program.
        """
        cmd, msg, addr = self._get_message(self._int_socket)

        if cmd == 0:  # Socket closed
            self._running = False
            return

        msg_len = len(msg) if msg else 0

        if cmd == MSG_CMD_REGISTER and msg_len >= 6:
            pid, num_probes, num_events = struct.unpack("!LBB", msg[0:6])
            LOGGER.debug("REGISTER from PID %d (%d probes, %d events)", pid,
                         num_probes, num_events)

            process = self._process_list.find_process('pid', pid)

            if not process:
                LOGGER.error("PID %d tried to register but is unknown, sending "
                             "ACK anyway.")
                self._int_socket.sendto(struct.pack("!LB", MAGIC_NUMBER,
                                        MSG_CMD_ACK), addr)
                return

            used_ids = self._process_list.get_processes_attr('prog_id')
            prog_id = 1
            while prog_id in used_ids:
                prog_id += 1

            process.prog_addr = addr
            process.prog_id = prog_id

            prog_name = process.prog_name

            LOGGER.info("Registering program ‘%s’ with program ID %d.",
                        process.prog_name, prog_id)

            if self._collector_addr:
                # Send REGISTER to the collector
                header = struct.pack("!LBBBBB", MAGIC_NUMBER,
                                     MSG_CMD_REGISTER, prog_id, num_probes, num_events,
                                     len(process.prog_name))

                self._ext_socket.sendto(header + prog_name + msg[6:],
                                        self._collector_addr)

                # Wait for ACK
                rlist, _, _ = select.select([self._ext_socket], [], [], 5)
                if not rlist:
                    LOGGER.error("No ACK message from collector")

                else:
                    cmd, _, _ = self._get_message(self._ext_socket,
                                                  self._collector_addr)

                    if cmd != MSG_CMD_ACK:
                        LOGGER.error("Bad ACK message from collector")

            else:
                LOGGER.error("Collector not known, not relaying REGISTER")

            self._int_socket.sendto(struct.pack("!LB", MAGIC_NUMBER,
                                                MSG_CMD_ACK), addr)

            return

        if cmd in REL_MSGS_PROG_TO_COL:  # Message to be relayed
            process = self._process_list.find_process('prog_addr', addr)

            if not process:
                LOGGER.error("Got message %d from unknown address %r", cmd,
                             addr)
                return

            if not self._collector_addr:
                LOGGER.error("Collector not known, not relaying "
                             "message %d from ‘%s’", cmd, process.prog_name)
                return

            LOGGER.debug("Relaying message %d from program ‘%s’", cmd,
                         process.prog_name)

            header = struct.pack("!LBBB", MAGIC_NUMBER, MSG_CMD_RELAY,
                                 process.prog_id, cmd)
            self._ext_socket.sendto(header + msg, self._collector_addr)
            return

        LOGGER.error("Unexpected message %d received from %s.", cmd,
            addr)

    def _handle_collector_message(self):
        """
        Handles a command from the collector.
        """
        cmd, msg, _ = self._get_message(self._ext_socket,
                                        self._collector_addr)

        if cmd == MSG_CMD_RELAY and len(msg) >= 1:
            prog_id = struct.unpack("!B", msg[0])[0]

            process = self._process_list.find_process('prog_id', prog_id)
            if not process:
                LOGGER.error("Collector sent a relay message for unknown "
                             "program ID %d", prog_id)
                return

            LOGGER.debug("Relaying message from collector to program ‘%s’",
                         process.prog_name)

            self._int_socket.sendto(struct.pack("!L", MAGIC_NUMBER) +
                                    msg[1:], process.prog_addr)
            return

        LOGGER.error("Unexpected message %d received from the collector.",
                     cmd)

    def _proc_stopped(self, process):
        """
        Handle a stopped process.
        """
        try:
            prog_id = getattr(process, 'prog_id')
        except AttributeError:
            return

        LOGGER.info("Unregistering program ‘%s’ with program ID %d.",
                    process.prog_name, prog_id)

        if self._collector_addr:
            # Send UNREGISTER to the collector
            message = struct.pack("!LBB", MAGIC_NUMBER, MSG_CMD_UNREGISTER,
                                  prog_id)

            self._ext_socket.sendto(message, self._collector_addr)

        else:
            LOGGER.error("Collector not known, not relaying UNREGISTER")

    def _get_message(self, sock, expected_addr=None):
        """
        Gets a message from the socket. Returns a (command_id, message, addr)
        tuple. Returns (0, None, None) if the socket was closed,
        (-1, None, addr) if the message was incorrect.
        """
        try:
            packet, addr = sock.recvfrom(4096)
        except socket.error:
            packet = ""
            addr = None

        if addr is None:
            LOGGER.debug("Socket closed, exiting loop")
            self._running = False
            return (0, None, None)

        if expected_addr is not None and addr != expected_addr:
            LOGGER.error("Message received from unexpected address %s.", addr)
            return (-1, None, addr)

        if len(packet) < 5:
            LOGGER.error("Message received from %s is too short.", addr)
            return (-1, None, addr)

        magic, cmd = struct.unpack("!LB", packet[0:5])
        if magic != MAGIC_NUMBER:
            LOGGER.error("Bad magic number %08x in message from address %r.",
                magic, addr)
            return (-1, None, addr)

        return cmd, packet[5:], addr

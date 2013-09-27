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
import time

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
MSG_CMD_DISABLE = 8
MSG_CMD_ENABLE = 9
MSG_CMD_RELAY = 10
MSG_CMD_NACK = 11
MSG_CMD_REGISTER_LIVE = 12

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
        self._rstop, self._wstop = os.pipe()  # pipe to stop select
        # pipe to transmit collector ack to avoid trying to read on collector
        # socket two times simulaneously
        self._rack, self._wack = os.pipe()
        self._register_msg = {}

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
        if self._stopped:
            return

        LOGGER.debug("clean stats sockets")

        self._enable_output(False)

        self._running = False
        self._stopped = True
        # write something in pipe to beak the select command
        os.write(self._wstop, "stop")

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
        register = False
        if self._collector_addr is None:
            register = True

        LOGGER.info("set collector address: %s:%s" % (address, port))
        self._collector_addr = (address, port)

        if register:
            # resend all register commands and enable outputs
            self._resend_register()

            # enable output in case it was stopped previously
            enable = threading.Thread(None, self._send_enable, None, (), {})
            enable.start()

        LOGGER.debug("Collector address set to %s:%d", address, port)

    def unset_collector_addr(self):
        """
        Unset the address of the collector.
        """
        if self._collector_addr is None:
            return

        LOGGER.info("unset collector address")
        self._collector_addr = None
        # disable output as it won't be save or transmitted to
        # manager anymore
        self._enable_output(False)
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

        LOGGER.debug("Stats handler exiting...")
        self.cleanup()

    def _handle(self):
        """
        Handles a command received on one of the two sockets.
        """
        rlist, _, _ = select.select([self._int_socket,
                                     self._ext_socket,
                                     self._rstop], [], [])

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

        msg_len = len(msg) if msg is not None else 0

        if (cmd == MSG_CMD_REGISTER or cmd == MSG_CMD_REGISTER_LIVE) and \
           msg_len >= 6:
            pid, num_probes, num_events = struct.unpack("!LBB", msg[0:6])
            LOGGER.debug("REGISTER from PID %d (%d probes, %d events)", pid,
                         num_probes, num_events)

            process = self._process_list.find_process('pid', pid)

            if not process:
                # TODO enable non registered process ?
                LOGGER.error("PID %d tried to register but is unknown, sending "
                             "NACK", pid)
                sendtosock(self._int_socket, struct.pack("!LB", MAGIC_NUMBER,
                                                         MSG_CMD_NACK), addr)
                return

            if cmd == MSG_CMD_REGISTER_LIVE:
                try:
                    prog_id = process.prog_id
                except AttributeError:
                    LOGGER.error("Register live for unknown program with pid %d",
                                 pid)
                    return
                header = struct.pack("!LBBBBB", MAGIC_NUMBER,
                                     MSG_CMD_REGISTER, prog_id, num_probes, num_events,
                                     len(process.prog_name))

                if self._collector_addr:
                    # Send REGISTER_LIVE to the collector
                    sendtosock(self._ext_socket,
                               header + process.prog_name + msg[6:],
                               self._collector_addr)
                else:
                    # Add REGISTER_LIVE message to the list of REGISTER message
                    self._register_msg[prog_id].append(header + prog_name +
                                                       msg[6:])
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

            resp = MSG_CMD_ACK
            header = struct.pack("!LBBBBB", MAGIC_NUMBER,
                                 MSG_CMD_REGISTER, prog_id, num_probes, num_events,
                                 len(process.prog_name))

            if self._collector_addr:
                # Send REGISTER to the collector
                sendtosock(self._ext_socket, header + prog_name + msg[6:],
                           self._collector_addr)

                # Wait for ACK
                # We do not need to read on pipe because we are blocking the
                # _handle function 
                rlist, _, _ = select.select([self._ext_socket], [], [], 5)
                if not rlist:
                    LOGGER.error("No ACK message from collector")
                    resp = MSG_CMD_NACK
                else:
                    cmd, _, _ = self._get_message(self._ext_socket,
                                                  self._collector_addr)
                    if cmd != MSG_CMD_ACK:
                        resp = MSG_CMD_NACK
                        LOGGER.error("Bad ACK message from collector (%s)" % cmd)
                    else:
                        # store the register message to send it back if
                        # collector is restarted or moved
                        self._register_msg[prog_id] = [header + prog_name +
                                                       msg[6:]]
            else:
                # store the register message to send it if
                # collector is started
                self._register_msg[prog_id] = [header + prog_name + msg[6:]]
                resp = MSG_CMD_NACK
                LOGGER.error("Collector not known, not relaying REGISTER")

            sendtosock(self._int_socket, struct.pack("!LB", MAGIC_NUMBER,
                                                     resp), addr)

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
                self._enable_output(False)
                return

            LOGGER.debug("Relaying message %d from program ‘%s’", cmd,
                         process.prog_name)

            header = struct.pack("!LBBB", MAGIC_NUMBER, MSG_CMD_RELAY,
                                 process.prog_id, cmd)
            sendtosock(self._ext_socket, header + msg, self._collector_addr)
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

            sendtosock(self._int_socket, struct.pack("!L", MAGIC_NUMBER) +
                       msg[1:], process.prog_addr)
        elif cmd == MSG_CMD_ACK:
            LOGGER.debug("received ACK from collector, transmist on pipe")
            os.write(self._wack, "ACK")
        else:
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
            sendtosock(self._ext_socket, message, self._collector_addr)
        else:
            LOGGER.error("Collector not known, not relaying UNREGISTER")
        try:
            del self._register_msg[prog_id]
        except KeyError:
            pass

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

    def _resend_register(self):
        """ send the stored REGISTER and RESITER_LIVE messages because the
            collector was restarted """
        LOGGER.info("Resend register messages because collector was restarted")
        for msg_list in self._register_msg.values():
            for msg in msg_list:
                sendtosock(self._ext_socket, msg, self._collector_addr)
                # Wait for ACK
                rlist, _, _ = select.select([self._rack], [], [], 5)
                # TODO enable/disable on programs according to collector response
                if not rlist or not self._rack in rlist:
                    LOGGER.error("no ACK from collector after new register")
                    return
                LOGGER.debug(os.read(self._rack, 4))

    def _enable_output(self, value=True):
        """
        Tell the programs to stop sending outputs
        """
        if value:
            cmd = MSG_CMD_ENABLE
        else:
            cmd = MSG_CMD_DISABLE
        proc_addr = self._process_list.get_processes_attr('prog_addr')
        for addr in proc_addr:
            sendtosock(self._int_socket, struct.pack("!LB", MAGIC_NUMBER,
                                                     cmd), addr)

    def _send_enable(self):
        """
        Send the enable commands once process list was initialized
        """
        nbr = 0
        time.sleep(0.5)
        while self._process_list.is_initialized() == False and nbr < 5:
            time.sleep(1)
            nbr = nbr + 1
        # enable output
        self._enable_output()


def sendtosock(sock, msg, addr):
    """
    Send a message on a socket
    """
    try:
        sock.sendto(msg, addr)
    except socket.error, err:
        LOGGER.error("Cannot send a message on socket: %s" % err)


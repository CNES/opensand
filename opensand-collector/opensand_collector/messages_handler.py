#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 TAS
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
messages_handler.py - OpenSAND collector UDP messages handler.
"""

from time import time
import gobject
import socket
import struct
import logging
import threading

LOGGER = logging.getLogger('sand-collector')

MAGIC_NUMBER = 0x5A7D0001
MSG_CMD_REGISTER_INIT = 1
MSG_CMD_REGISTER_END = 2
MSG_CMD_REGISTER_LIVE = 3
MSG_CMD_UNREGISTER = 4
MSG_CMD_ACK = 5
MSG_CMD_RELAY = 7

MSG_CMD_SEND_PROBES = 10
MSG_CMD_SEND_LOG = 20

MSG_CMD_ENABLE_PROBE = 11
MSG_CMD_DISABLE_PROBE = 12

MSG_CMD_SET_LOG_LEVEL = 22
MSG_CMD_ENABLE_LOGS = 23
MSG_CMD_DISABLE_LOGS = 24
MSG_CMD_ENABLE_SYSLOG = 25
MSG_CMD_DISABLE_SYSLOG = 26

MSG_MGR_REGISTER = 40
MSG_MGR_REGISTER_PROGRAM = 41
MSG_MGR_UNREGISTER_PROGRAM = 42
MSG_MGR_UNREGISTER = 43
MSG_MGR_REGISTER_ACK = 44
MSG_MGR_STATUS = 45

MSG_MGR_SEND_PROBES = 50
MSG_MGR_SET_PROBE_STATUS = 51

MSG_MGR_SEND_LOG = 60
MSG_MGR_SET_LOG_LEVEL = 61
MSG_MGR_SET_LOGS_STATUS = 62
MSG_MGR_SET_SYSLOG_STATUS = 63

MAX_DATA_LENGHT = 8192


class MessagesHandler(object):
    """
    UDP messages handler class
    """
    def __init__(self, host_manager):
        self._host_manager = host_manager
        self._manager_addr = None
        self._temp_manager = []
        self._sock = None
        self._tag = None
        self._time = 0
        self._stop = threading.Event()
        self._mgr_ok = threading.Event()
        self._mgr_status = None
        self._lock = threading.Lock()

    def get_port(self):
        """
        Get the port allocated for the socket
        """
        _, port = self._sock.getsockname()
        return port

    def __enter__(self):
        """
        Set up the socket.
        """
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.bind(('', 0))
        self._tag = gobject.io_add_watch(self._sock, gobject.IO_IN,
                                         self._data_received)

        LOGGER.info("Socket bound to port %d.", self.get_port())

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Tear down the socket.
        """
        gobject.source_remove(self._tag)
        self._stop.set()
        if self._mgr_status is not None:
            self._mgr_status.join()

        try:
            self._sock.shutdown(socket.SHUT_RDWR)
        except socket.error:
            pass

        self._sock.close()

        return False

    def _data_received(self, _sock, _tag):
        """
        Called when a packet is received on the socket. Checks the message
        validity and finds the command number.
        """
        self._time = time()

        packet, addr = self._sock.recvfrom(MAX_DATA_LENGHT)
        if len(packet) > MAX_DATA_LENGHT:
            LOGGER.warning("Too many data received from daemon, "
                           "we may not be able to parse command")

        if len(packet) < 5:
            LOGGER.error("Received short packet from address %s:%d." % addr)
            return True

        magic, cmd = struct.unpack("!LB", packet[0:5])
        if magic != MAGIC_NUMBER:
            LOGGER.error("Received bad magic number from address %s:%d." % addr)
            return True

        if cmd >= MSG_MGR_REGISTER:
            self._handle_manager_command(cmd, addr, packet[5:])
            return True

        try:
            host = self._host_manager.get_host(addr)
        except KeyError:
            LOGGER.error("Received data from unknown host %s:%d." % addr)
            return True


        self._handle_command(cmd, host, addr, packet[5:])

        return True

    def _handle_command(self, cmd, host, addr, data):
        """
        Handles a received message. Interprets the message, and calls the
        probes manager to perform the appropriate action.
        """
        if cmd in [MSG_CMD_REGISTER_LIVE, MSG_CMD_REGISTER_INIT,
                   MSG_CMD_REGISTER_END]:
            
            try:
                success = self._handle_cmd_register(host, addr, data,
                                                    cmd == MSG_CMD_REGISTER_LIVE,
                                                    cmd == MSG_CMD_REGISTER_END)
            except struct.error:
                success = False

            if not success:
                LOGGER.error("Bad data received for '%s' REGISTER command",
                             host)

        elif cmd == MSG_CMD_UNREGISTER:
            if len(data) != 1:
                LOGGER.error("Bad data received for '%s' UNREGISTER command",
                             host)

            prog_id = struct.unpack("!B", data)[0]
            if host.remove_program(prog_id):
                self._notify_manager_unreg_program(host.ident, prog_id)

        elif cmd == MSG_CMD_RELAY:
            try:
                success = self._handle_cmd_relay(host, data)
            except struct.error:
                success = False

            if not success:
                LOGGER.error("Bad data received for '%s' RELAY command",
                             host)

        else:
            LOGGER.error("Unknown command id %d received from '%s'", cmd,
                         host)

    def _handle_cmd_register(self, host, addr, data, live=False, end=False):
        """
        Handles a registration command.
        """
        prog_id, num_probes, num_logs, name_length = \
                                      struct.unpack("!BBBB", data[0:4])
        prog_name = data[4:4 + name_length]

        if len(prog_name) != name_length:
            return False

        pos = 4 + name_length

        probe_list = []
        for _ in xrange(num_probes):
            probe_id, storage_type, name_length, unit_length = \
                                  struct.unpack("!BBBB", data[pos:pos + 4])
            enabled = bool(storage_type >> 7)
            storage_type = storage_type & ~(1 << 7)
            pos += 4

            name = data[pos:pos + name_length]
            if len(name) != name_length:
                return False

            pos += name_length

            unit = data[pos:pos + unit_length]
            if len(unit) != unit_length:
                return False

            pos += unit_length

            probe_list.append((probe_id, name, unit, storage_type, enabled))

        log_list = []
        for _ in xrange(num_logs):
            log_id, level, ident_length = struct.unpack("!BBB",
                                                        data[pos:pos + 3])
            pos += 3

            name = data[pos:pos + ident_length]
            if len(name) != ident_length:
                return False
            log_list.append((log_id, name, level))
            pos += ident_length

        if data[pos:] != "":
            return False

        program = host.add_program(prog_id, prog_name, probe_list, log_list)

        if not live:
            self._sock.sendto(struct.pack("!LB", MAGIC_NUMBER, MSG_CMD_ACK), addr)

        self._notify_manager_new_program(program)
        if end:
            program.initialized = True

        return True

    def _handle_cmd_relay(self, host, data):
        """
        Handles a relayed command from a daemon.
        """
        prog_id, sub_cmd = struct.unpack("!BB", data[0:2])

        try:
            prog = host.get_program(prog_id)
        except KeyError:
            LOGGER.error("No registered program for ID %d", prog_id)
            return False

        LOGGER.debug("Got relayed message, from program %s" % prog)

        if sub_cmd == MSG_CMD_SEND_PROBES:
            return self._handle_cmd_send_probes(host, prog, data[2:])

        if sub_cmd == MSG_CMD_SEND_LOG:
            return self._handle_cmd_send_log(host, prog, data[2:])

    def _handle_cmd_send_probes(self, host, prog, data):
        """
        Handles a SEND_PROBES command from a daemon.
        """
        total_length = len(data)
        timestamp = struct.unpack("!L", data[0:4])[0]
        pos = 4

        displayed_values = []

        while pos < total_length:
            probe_id = struct.unpack("!B", data[pos])[0]
            pos += 1

            try:
                probe = prog.get_probe(probe_id)
            except KeyError:
                LOGGER.error("Unknown probe ID %d", probe_id)
                return False

            value, pos = probe.read_value(data, pos)
            probe.save_value(timestamp, value)

            if probe.displayed:
                displayed_values.append((probe_id, probe, value))

        LOGGER.debug("Probe %s: Value %s (t = %d)", probe, value, timestamp)

        if not prog.initialized:
            LOGGER.info("Program %s is not initialized, do not transmit probe "
                        "to the manager" % prog)
            return True

        self._notify_manager_probes(host, prog, timestamp, displayed_values)

        return True

    def _handle_cmd_send_log(self, host, prog, data):
        """
        Handles a SEND_LOG command from a daemon
        """
        log_id = struct.unpack("!B", data[0])[0]
        log_level = struct.unpack("!B", data[1])[0]

        try:
            log = prog.get_log(log_id)
        except KeyError:
            LOGGER.error("Unknown log ID %d", log_id)
            return False

        text = data[2:]
        log.save(self._time, log_level, text)

        LOGGER.debug("Log %s: %s", log, text)

        self._notify_manager_log(host, prog, log_id, log_level, text)

        return True

    def _handle_manager_command(self, cmd, addr, data):
        """
        Handles a received message from the manager. Interprets the message, and
        performs the requested action.
        """
        if cmd == MSG_MGR_REGISTER:
            if self._manager_addr is not None and \
               addr != self._manager_addr:
                self._temp_manager.append(addr)
                LOGGER.info("register from another manager, keep it in the "
                             "temporary list")
                return
            # reset the programs registered elements before registering a new
            # manager
            self._host_manager.unreg_manager()
            self._manager_addr = addr
            LOGGER.info("Manager registered from address %s:%d" % addr)

            self._notify_manager_ack()

            for program in self._host_manager.all_programs():
                self._notify_manager_new_program(program)
                
            if self._mgr_status is None:             
                self._mgr_status = threading.Thread(None,
                                                    self._check_manager_status,
                                                    None, (), {})
                self._mgr_status.start()
            return

        if cmd == MSG_MGR_UNREGISTER:
            if addr == self._manager_addr:
                self._manager_addr = None
                if len(self._temp_manager) > 0:
                    # register the next manager
                    self._handle_manager_command(MSG_MGR_REGISTER,
                                                 self._temp_manager.pop(0),
                                                 "")
            elif addr in self._temp_manager:
                self._temp_manager.remove(addr)
            LOGGER.info("Manager unregistered from address %s:%d" % addr)
            return
        
        if cmd == MSG_MGR_STATUS:
            self._mgr_ok.set()
            LOGGER.info("Manager from address %s:%d is still running" % addr)
            return

        if addr != self._manager_addr:
            LOGGER.error("Ignoring manager command %d from unregistered "
                         "address %s:%d", cmd, addr[0], addr[1])
            return

        # for next commands the process should be initialized, check for
        # initialized in program
        host_id, program_id = struct.unpack("!BB", data[0:2])
        if not self._host_manager.is_initialized(host_id, program_id):
            LOGGER.info("Program [%d:%d] is not initialized, ignore manager commands"
                        % (host_id, program_id))
            return
        data = data[2:]
        if cmd == MSG_MGR_SET_PROBE_STATUS:
            probe_id, status = struct.unpack("!BB", data)

            new_enabled = (status > 0)
            new_displayed = (status == 2)

            LOGGER.info("New probe status set from manager for probe %d of "
                        "program %d:%d: enabled = %s, displayed = %s", host_id,
                        probe_id, program_id, new_enabled, new_displayed)

            host = self._host_manager.set_probe_status(host_id,
                                                       program_id,
                                                       probe_id,
                                                       new_enabled,
                                                       new_displayed)

            cmd = MSG_CMD_ENABLE_PROBE if new_enabled else MSG_CMD_DISABLE_PROBE

            if host:  # Need to propagate the new enabled state upstream
                LOGGER.info("The enabled of the above probe has been relayed.")
                self._sock.sendto(struct.pack("!LBBBB", MAGIC_NUMBER,
                                              MSG_CMD_RELAY, program_id, cmd,
                                              probe_id), host.address)

        elif cmd == MSG_MGR_SET_LOG_LEVEL:
            log_id, level = struct.unpack("!BB", data)
            
            LOGGER.info("New log level set from manager for log %d of "
                        "program %d:%d: new level = %s", log_id,
                        host_id, program_id, level)

            host = self._host_manager.set_log_display_level(host_id,
                                                            program_id,
                                                            log_id,
                                                            level)

            if host:  # Need to propagate the new enabled state upstream
                LOGGER.info("The enabled of the above probe has been relayed.")
                self._sock.sendto(struct.pack("!LBBBBB", MAGIC_NUMBER,
                                              MSG_CMD_RELAY, program_id,
                                              MSG_CMD_SET_LOG_LEVEL,
                                              log_id, level), host.address)

        elif cmd == MSG_MGR_SET_LOGS_STATUS:
            status = struct.unpack("!B", data)
            LOGGER.info("New logs status set from manager for "
                        "program %d:%d: enabled = %s", host_id,
                        program_id, status)

            cmd = MSG_CMD_ENABLE_LOGS if status[0] else MSG_CMD_DISABLE_LOGS
            
            address = self._host_manager.get_host_address(host_id)

            if address:
                LOGGER.info("The enabled of the logs has been relayed.")
                self._sock.sendto(struct.pack("!LBBB", MAGIC_NUMBER,
                                              MSG_CMD_RELAY, program_id, cmd),
                                  address)

        elif cmd == MSG_MGR_SET_SYSLOG_STATUS:
            status = struct.unpack("!B", data)

            LOGGER.info("New syslog status set from manager for "
                        "program %d:%d: enabled = %s", host_id,
                        program_id, status)

            cmd = MSG_CMD_ENABLE_SYSLOG if status[0] else MSG_CMD_DISABLE_SYSLOG

            address = self._host_manager.get_host_address(host_id)

            if address:
                LOGGER.info("The enabled of syslog has been relayed.")
                self._sock.sendto(struct.pack("!LBBB", MAGIC_NUMBER,
                                              MSG_CMD_RELAY, program_id, cmd),
                                  address)
        else:
            LOGGER.error("Unknown command id %d received from manager %s:%d",
                         cmd, addr[0], addr[1])

    def _notify_manager_ack(self):
        """
        Sends a MSG_MGR_REGISTER_ACK to the manager.
        """
        if not self._manager_addr:
            return

        self._sock.sendto(struct.pack("!LB", MAGIC_NUMBER,
                                      MSG_MGR_REGISTER_ACK),
                          self._manager_addr)

    def _notify_manager_new_program(self, program):
        """
        Sends a MSG_MGR_REGISTER_PROGRAM to the manager.
        """
        # TODO for each register we send the complete list, even if some things
        # has already been sent !..
        if not self._manager_addr:
            return

        host_name, host_ident, prog_ident, probes, logs = program.unreg_att()
        if type(host_name) == unicode:
            host_name = host_name.encode('utf8')

        if len(host_name) > 255:
            LOGGER.error("Program name %s is too long, truncating.", host_name)
            host_name = host_name[0:255]

#        message = struct.pack("!LBBBBBB", MAGIC_NUMBER, MSG_MGR_REGISTER_PROGRAM,
#                              host_ident, prog_ident, len(probes),
#                              len(logs), len(host_name)) + host_name
        content = ""
        header_length = 10 + len(host_name)
        probe_nbr = 0
        log_nbr = 0
        reg_p = []
        reg_l = []

        for probe_id, name, unit, storage_type, enabled, displayed in probes:
            storage_type |= enabled << 7
            storage_type |= displayed << 6
            if len(content) + header_length + 4 + len(name) + len(unit) > \
              MAX_DATA_LENGHT or probe_nbr >= 255:
                # max size, send a first register message
                message = struct.pack("!LBBBBBB", MAGIC_NUMBER, MSG_MGR_REGISTER_PROGRAM,
                                      host_ident, prog_ident, probe_nbr,
                                      log_nbr, len(host_name)) + host_name
                message += content
                probe_nbr = 0
                log_nbr = 0
                content = ""
                if self._manager_addr:
                    program.set_registered(probes=reg_p)
                    reg_p = []
                    self._sock.sendto(message, self._manager_addr)

            probe_nbr += 1
            reg_p.append(probe_id)
            content += struct.pack("!BBBB", probe_id, storage_type, len(name), len(unit))
            content += name
            content += unit

        for log_id, ident, level in logs:
            if len(content) + header_length + 3 + len(ident) > MAX_DATA_LENGHT or \
               log_nbr >= 255:
                # max size, send a first register message
                message = struct.pack("!LBBBBBB", MAGIC_NUMBER, MSG_MGR_REGISTER_PROGRAM,
                                      host_ident, prog_ident, probe_nbr,
                                      log_nbr, len(host_name)) + host_name
                message += content
                probe_nbr = 0
                log_nbr = 0
                content = ""
                if self._manager_addr:
                    program.set_registered(probes=reg_p, logs=reg_l)
                    reg_p = []
                    reg_l = []
                    self._sock.sendto(message, self._manager_addr)

            log_nbr += 1
            reg_l.append(log_id)
            content += struct.pack("!BBB", log_id, level, len(ident))
            content += ident
            
        if len(content) > 0:
            message = struct.pack("!LBBBBBB", MAGIC_NUMBER, MSG_MGR_REGISTER_PROGRAM,
                                  host_ident, prog_ident, probe_nbr,
                                  log_nbr, len(host_name)) + host_name
            message += content
        if self._manager_addr:
            program.set_registered(probes=reg_p, logs=reg_l)
            self._sock.sendto(message, self._manager_addr)

    def _notify_manager_unreg_program(self, host_ident, prog_ident):
        """
        Sends a MSG_MGR_UNREGISTER_PROGRAM to the manager.
        """
        if not self._manager_addr:
            return

        self._sock.sendto(struct.pack("!LBBB", MAGIC_NUMBER,
                                      MSG_MGR_UNREGISTER_PROGRAM,
                                      host_ident, prog_ident),
                          self._manager_addr)

    def _notify_manager_probes(self, host, prog, timestamp, displayed_values):
        """
        Sends a MSG_MGR_SEND_PROBES to the manager with the displayed probes.
        """
        if not displayed_values:
            return

        message = struct.pack("!LBBBL", MAGIC_NUMBER, MSG_MGR_SEND_PROBES,
                              host.ident, prog.ident, timestamp)

        for probe_id, probe, value in displayed_values:
            message += struct.pack("!B", probe_id)
            message += probe.encode_value(value)

        if self._manager_addr:
            self._sock.sendto(message, self._manager_addr)

    def _notify_manager_log(self, host, prog, log_id, log_level, text):
        """
        Sends a MSG_MGR_SEND_LOG to the manager with the relayed log.
        """
        message = struct.pack("!LBBBBB", MAGIC_NUMBER, MSG_MGR_SEND_LOG,
                              host.ident, prog.ident, log_id, log_level)
        message += text

        if self._manager_addr:
            LOGGER.debug("Transmit log to manager")
            self._sock.sendto(message, self._manager_addr)

    def _check_manager_status(self):
        """
        Check that the manager is still running else register with another one
        """
        while not self._stop.wait(1):
            if self._manager_addr is None:
                if len(self._temp_manager) > 0:
                    # register the next manager
                    self._handle_manager_command(MSG_MGR_REGISTER,
                                                 self._temp_manager.pop(0),
                                                 "")
                continue
            self._mgr_ok.clear()
            message = struct.pack("!LB", MAGIC_NUMBER, MSG_MGR_STATUS)
            LOGGER.info("Check whether manager is still running")
            addr = self._manager_addr
            self._sock.sendto(message, addr)
            # wait some time
            if not self._mgr_ok.wait(10):
                # if the manager has changed, nothing to do
                if addr != self._manager_addr:
                    continue
                # manager is not responding, register a new one
                self._manager_addr = None
                if len(self._temp_manager) > 0:
                    # register the next manager
                    self._handle_manager_command(MSG_MGR_REGISTER,
                                                 self._temp_manager.pop(0),
                                                 "")
                LOGGER.info("Manager from address %s:%d detected as crashed or "
                            "stopped" % addr)


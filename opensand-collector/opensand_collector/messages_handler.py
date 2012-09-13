"""
OpenSAND collector UDP messages handler.
"""

from time import time
import gobject
import socket
import struct
import logging

LOGGER = logging.getLogger('messages_handler')

MAGIC_NUMBER = 0x5A7D0001
MSG_CMD_REGISTER = 1
MSG_CMD_ACK = 2
MSG_CMD_SEND_PROBES = 3
MSG_CMD_SEND_EVENT = 4
MSG_CMD_ENABLE_PROBE = 5
MSG_CMD_DISABLE_PROBE = 6
MSG_CMD_UNREGISTER = 7
MSG_CMD_RELAY = 10


class MessagesHandler(object):
    def __init__(self, collector):
        self.collector = collector
        self._sock = None
        self._tag = None
        self._time = 0

    def get_port(self):
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

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Tear down the socket.
        """

        gobject.source_remove(self._tag)

        try:
            self._sock.shutdown(socket.SHUT_RDWR)
        except socket.error:
            pass

        self._sock.close()

        return False

    def _data_received(self, _sock, _tag):
        """
        Called when a packet is received on the socket. Checks the message
        validity and finds the command number
        """

        self._time = time()

        packet, addr = self._sock.recvfrom(4096)

        try:
            host = self.collector.host_manager.get_host(addr)
        except KeyError:
            LOGGER.error("Received data from unknown host %s:%d." % addr)
            return True

        if len(packet) < 5:
            LOGGER.error("Received short packet from host '%s'.", host)
            return True

        magic, cmd = struct.unpack("!LB", packet[0:5])
        if magic != MAGIC_NUMBER:
            LOGGER.error("Received bad magic number from host '%s'.", host)
            return True

        self._handle_command(cmd, host, addr, packet[5:])

        return True

    def _handle_command(self, cmd, host, addr, data):
        """
        Handles a received message. Interprets the message, and calls the
        probes manager to perform the appropriate action.
        """

        if cmd == MSG_CMD_REGISTER:
            try:
                success = self._handle_cmd_register(host, addr, data)
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
            host.remove_program(prog_id)

        elif cmd == MSG_CMD_RELAY:
            try:
                success = self._handle_cmd_relay(host, addr, data)
            except struct.error:
                success = False

            if not success:
                LOGGER.error("Bad data received for '%s' RELAY command",
                    host)

        else:
            LOGGER.error("Unknown command id %d received from '%s'", cmd,
                host)


    def _handle_cmd_register(self, host, addr, data):
        """
        Handles a registration command.
        """

        prog_id, num_probes, num_events, name_length = struct.unpack("!BBBB",
            data[0:4])
        prog_name = data[4:4 + name_length]

        if len(prog_name) != name_length:
            return False

        pos = 4 + name_length

        probe_list = []
        for _ in xrange(num_probes):
            storage_type, name_length = struct.unpack("!BB", data[pos:pos+2])
            enabled = bool(storage_type >> 7)
            storage_type = storage_type & ~(1 << 7)
            pos += 2

            name = data[pos:pos + name_length]
            if len(name) != name_length:
                return False

            probe_list.append((name, storage_type, enabled))
            pos += name_length

        event_list = []
        for _ in xrange(num_events):
            level, ident_length = struct.unpack("!BB", data[pos:pos+2])
            pos += 2

            name = data[pos:pos + ident_length]
            if len(name) != ident_length:
                return False
            event_list.append((name, level))
            pos += ident_length

        if data[pos:] != "":
           return False

        host.add_program(prog_id, prog_name, probe_list, event_list)

        self._sock.sendto(struct.pack("!LB", MAGIC_NUMBER, MSG_CMD_ACK), addr)

        return True

    def _handle_cmd_relay(self, host, addr, data):
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
            return self._handle_cmd_send_probes(prog, data[2:])

        if sub_cmd == MSG_CMD_SEND_EVENT:
            return self._handle_cmd_send_event(prog, data[2:])

    def _handle_cmd_send_probes(self, prog, data):
        total_length = len(data)
        pos = 0

        while pos < total_length:
            probe_id = struct.unpack("!B", data[pos])[0]
            pos += 1

            try:
                probe = prog.get_probe(probe_id)
            except IndexError:
                LOGGER.error("Unknown probe ID %d", probe_id)
                return False

            value, pos = probe.read_value(data, pos)
            probe.log_value(self._time, value)

            LOGGER.debug("Probe %s: Value %s", probe, value)

        return True

    def _handle_cmd_send_event(self, prog, data):
        total_length = len(data)
        event_id = struct.unpack("!B", data[0])[0]

        try:
            event = prog.get_event(event_id)
        except IndexError:
            LOGGER.error("Unknown event ID %d", probe_id)
            return False

        text = data[1:]
        event.log(self._time, text)

        LOGGER.debug("Event %s: %s", event, text)

        return True


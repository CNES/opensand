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
MSG_MGR_REGISTER = 21
MSG_MGR_REGISTER_PROGRAM = 22
MSG_MGR_UNREGISTER_PROGRAM = 23
MSG_MGR_SET_PROBE_STATUS = 24
MSG_MGR_SEND_PROBES = 25
MSG_MGR_SEND_EVENT = 26
MSG_MGR_UNREGISTER = 27

class MessagesHandler(object):
    def __init__(self, collector):
        self.collector = collector
        self._manager_addr = None
        self._sock = None
        self._tag = None
        self._time = 0

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
        
        LOGGER.debug("Socket bound to port %d.",self.get_port())

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
        validity and finds the command number.
        """

        self._time = time()

        packet, addr = self._sock.recvfrom(4096)

        if len(packet) < 5:
            LOGGER.error("Received short packet from address %s:%d.", *addr)
            return True
        
        magic, cmd = struct.unpack("!LB", packet[0:5])
        if magic != MAGIC_NUMBER:
            LOGGER.error("Received bad magic number from address %s:%d.", *addr)
            return True

        if cmd >= MSG_MGR_REGISTER:
            self._handle_manager_command(cmd, addr, packet[5:])
            return True

        try:
            host = self.collector.host_manager.get_host(addr)
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
            self._notify_manager_unreg_program(host.ident, prog_id)

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
            storage_type, name_length, unit_length = struct.unpack("!BBB",
                data[pos:pos + 3])
            enabled = bool(storage_type >> 7)
            storage_type = storage_type & ~(1 << 7)
            pos += 3

            name = data[pos:pos + name_length]
            if len(name) != name_length:
                return False
            
            pos += name_length
            
            unit = data[pos:pos + unit_length]
            if len(unit) != unit_length:
                return False

            pos += unit_length

            probe_list.append((name, unit, storage_type, enabled))
            

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

        program = host.add_program(prog_id, prog_name, probe_list, event_list)

        self._sock.sendto(struct.pack("!LB", MAGIC_NUMBER, MSG_CMD_ACK), addr)
        
        self._notify_manager_new_program(program)

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
            return self._handle_cmd_send_probes(host, prog, data[2:])

        if sub_cmd == MSG_CMD_SEND_EVENT:
            return self._handle_cmd_send_event(host, prog, data[2:])

    def _handle_cmd_send_probes(self, host, prog, data):
        total_length = len(data)
        pos = 0
        
        displayed_values = []

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
            
            if probe.displayed:
                displayed_values.append((probe_id, probe, value))

            LOGGER.debug("Probe %s: Value %s", probe, value)
        
        self._notify_manager_probes(host, prog, displayed_values)

        return True

    def _handle_cmd_send_event(self, host, prog, data):
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
        
        self._notify_manager_event(host, prog, event_id, text)

        return True
    
    def _handle_manager_command(self, cmd, addr, data):
        """
        Handles a received message from the manager. Interprets the message, and
        performs the requested action.
        """

        if cmd == MSG_MGR_REGISTER:
            self._manager_addr = addr
            LOGGER.info("Manager registered from address %s:%d", *addr)
            
            for program in self.collector.host_manager.all_programs():
                self._notify_manager_new_program(program)
            
            return
        
        if cmd == MSG_MGR_UNREGISTER:
            self._manager_addr = addr
            LOGGER.info("Manager unregistered from address %s:%d", *addr)
            return
        
        if addr != self._manager_addr:
            LOGGER.error("Ignoring manager command %d from unregistered "
                "address %s:%d", cmd, *addr)
            return
        
        if cmd == MSG_MGR_SET_PROBE_STATUS:
            host_id, program_id, probe_id, status = struct.unpack("!BBBB", data)
            
            new_enabled = (status > 0)
            new_displayed = (status == 2)
            
            LOGGER.info("New probe status set from manager for probe %d of "
                "program %d:%d: enabled = %s, displayed = %s", host_id,
                probe_id, program_id, new_enabled, new_displayed)
            
            host = self.collector.host_manager.set_probe_status(host_id,
                program_id, probe_id, new_enabled, new_displayed)
            
            cmd = MSG_CMD_ENABLE_PROBE if new_enabled else MSG_CMD_DISABLE_PROBE
            
            if host:  # Need to propagate the new enabled state upstream
                LOGGER.debug("The enabled of the above probe has been relayed.")
                self._sock.sendto(struct.pack("!LBBB", MAGIC_NUMBER,
                    MSG_CMD_RELAY, program_id, probe_id), host.address)
        
        else:
            LOGGER.error("Unknown command id %d received from manager %s:%d",
                cmd, *addr)
    
    def _notify_manager_new_program(self, program):
        """
        Sends a MSG_MGR_REGISTER_PROGRAM to the manager.
        """
        
        if not self._manager_addr:
            return
    
        name, host_ident, prog_ident, probes, events = program.attributes()
        if type(name) == unicode:
            name = name.encode('utf8')
        
        if len(name) > 255:
            LOGGER.error("Program name %s is too long, truncating.", name)
            name = name[0:255]
        
        message = struct.pack("!LBBBBBB", MAGIC_NUMBER,
            MSG_MGR_REGISTER_PROGRAM, host_ident, prog_ident, len(probes),
            len(events), len(name)) + name
        
        for name, unit, storage_type, enabled, displayed in probes:
            storage_type |= enabled << 7
            storage_type |= displayed << 6
            
            message += struct.pack("!BBB", storage_type, len(name), len(unit))
            message += name
            message += unit
        
        for ident, level in events:
            message += struct.pack("!BB", level, len(ident))
            message += ident
        
        if self._manager_addr:
            self._sock.sendto(message, self._manager_addr)
    
    def _notify_manager_unreg_program(self, host_ident, prog_ident):
        """
        Sends a MSG_MGR_UNREGISTER_PROGRAM to the manager.
        """

        if not self._manager_addr:
            return
    
        self._sock.sendto(struct.pack("!LBBB", MAGIC_NUMBER,
            MSG_MGR_UNREGISTER_PROGRAM, host_ident, prog_ident),
            self._manager_addr)
    
    def _notify_manager_probes(self, host, prog, displayed_values):
        if not displayed_values:
            return
        
        message = struct.pack("!LBBB", MAGIC_NUMBER, MSG_MGR_SEND_PROBES,
            host.ident, prog.ident)
        
        for probe_id, probe, value in displayed_values:
            message += struct.pack("!B", probe_id)
            message += probe.encode_value(value)
        
        if self._manager_addr:
            self._sock.sendto(message, self._manager_addr)
    
    def _notify_manager_event(self, host, prog, event_id, text):
        message = struct.pack("!LBBBB", MAGIC_NUMBER, MSG_MGR_SEND_EVENT,
            host.ident, prog.ident, event_id)
        message += text
        
        if self._manager_addr:
            self._sock.sendto(message, self._manager_addr)
    
    

#!/usr/bin/env python
# -*- coding: utf8 -*-

from opensand_manager_core.model.environment_plane import Program, Probe
from time import time
import gobject
import logging
import socket
import struct

LOGGER = logging.getLogger("environment_plane")

MAGIC_NUMBER = 0x5A7D0001
MSG_MGR_REGISTER = 21
MSG_MGR_REGISTER_PROGRAM = 22
MSG_MGR_UNREGISTER_PROGRAM = 23
MSG_MGR_SET_PROBE_STATUS = 24
MSG_MGR_SEND_PROBES = 25
MSG_MGR_SEND_EVENT = 26
MSG_MGR_UNREGISTER = 27


class EnvironmentPlaneController(object):
    """
    Controller for the environment plane.
    """
    
    def __init__(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.bind(('', 0))
        self._tag = gobject.io_add_watch(self._sock, gobject.IO_IN,
            self._data_received)
        self._collector_addr = None
        self._programs = {}
        self._observer = None
        self._time = 0
    
    def set_observer(self, observer):
        """
        Sets the observer object which will be notified of changes on the
        program list.
        """
        
        self._observer = observer
    
    def register_on_collector(self, ipaddr, port):
        """
        Register the probe controller on the specified collector.
        """
        
        addr = (ipaddr, port)
    
        if self._collector_addr:
            self.unregister_on_collector()
    
        self._collector_addr = addr
        LOGGER.info("Registering on collector %s:%d", *addr)
        self._sock.sendto(struct.pack("!LB", MAGIC_NUMBER, MSG_MGR_REGISTER),
            addr)
    
    def unregister_on_collector(self):
        """
        Unregisters on the specified collector.
        """
        
        if not self._collector_addr:
            return
        
        LOGGER.info("Unregistering on collector %s:%d", *self._collector_addr)
        
        self._sock.sendto(struct.pack("!LB", MAGIC_NUMBER, MSG_MGR_UNREGISTER),
            self._collector_addr)
        
        self._collector_addr = None
    
    def cleanup(self):
        """
        Shut down the probe controller.
        """

        self.unregister_on_collector()

        gobject.source_remove(self._tag)

        try:
            self._sock.shutdown(socket.SHUT_RDWR)
        except socket.error:
            pass

        self._sock.close()
    
    def get_programs(self):
        """
        Returns a list of all known programs.
        """
        
        return self._programs.values()
    
    def get_program(self, ident):
        """
        Returns the program identified by ident
        """
        
        return self._programs[ident]        
        
    def _data_received(self, _sock, _tag):
        """
        Called when a packet is received on the socket. Decodes and interprets
        the message.
        """
    
        self._time = time()
        packet, addr = self._sock.recvfrom(4096)
    
        if addr != self._collector_addr:
            LOGGER.error("Received data from unknown host %s:%d.", *addr)
            return True
        
        if len(packet) < 5:
            LOGGER.error("Received short packet from the collector.")
            return True
        
        magic, cmd = struct.unpack("!LB", packet[0:5])
        if magic != MAGIC_NUMBER:
            LOGGER.error("Received bad magic number from the collector.")
            return True
        
        if cmd == MSG_MGR_REGISTER_PROGRAM:
            try:
                success = self._handle_register_program(packet[5:])
            except struct.error:
                success = False

            if not success:
                LOGGER.error("Bad data received for REGISTER_PROGRAM command.")
            
            return True
        
        if cmd == MSG_MGR_UNREGISTER_PROGRAM:
            try:
                success = self._handle_unregister_program(packet[5:])
            except struct.error:
                success = False

            if not success:
                LOGGER.error("Bad data received for UNREGISTER_PROGRAM "
                    "command.")
            
            return True
        
        if cmd == MSG_MGR_SEND_PROBES:
            try:
                success = self._handle_send_probes(packet[5:])
            except struct.error:
                success = False
            
            if not success:
                LOGGER.error("Bad data received for SEND_PROBES command.")
            
            return True
        
        if cmd == MSG_MGR_SEND_EVENT:
            try:
                success = self._handle_send_event(packet[5:])
            except struct.error:
                success = False
            
            if not success:
                LOGGER.error("Bad data received for SEND_EVENT command.")
            
            return True
        
        LOGGER.error("Unknown message id %d received from the collector.", cmd)
    
        return True
    
    def _handle_register_program(self, data):
        """
        Handles a registration message.
        """
        
        host_id, prog_id, num_probes, num_events, name_length = struct.unpack(
            "!BBBBB", data[0:5])
        prog_name = data[5:5 + name_length]
        full_prog_id = (host_id << 8) | prog_id

        if len(prog_name) != name_length:
            return False


        pos = 5 + name_length
        
        probe_list = []
        for _ in xrange(num_probes):
            storage_type, name_length, unit_length = struct.unpack("!BBB",
                data[pos:pos + 3])
            enabled = bool(storage_type & (1 << 7))
            displayed = bool(storage_type & (1 << 6))
            storage_type = storage_type & ~(3 << 6)
            pos += 3

            name = data[pos:pos + name_length]
            if len(name) != name_length:
                return False
            
            pos += name_length
            
            unit = data[pos:pos + unit_length]
            if len(unit) != unit_length:
                return False

            pos += unit_length

            probe_list.append((name, unit, storage_type, enabled, displayed))

        event_list = []
        for _ in xrange(num_events):
            level, ident_length = struct.unpack("!BB", data[pos:pos+2])
            pos += 2

            ident = data[pos:pos + ident_length]
            if len(ident) != ident_length:
                return False
            
            event_list.append((ident, level))
            pos += ident_length

        if data[pos:] != "":
            return False
           
        LOGGER.debug("registration of [%d:%d] %s %r %r", host_id, prog_id,
            prog_name, probe_list, event_list)
        
        
        program = Program(self, full_prog_id, prog_name, probe_list, event_list)
        self._programs[full_prog_id] = program
        
        if self._observer:
            self._observer.program_list_changed()
        
        return True
    
    def _handle_unregister_program(self, data):
        """
        Handles an unregistration message.
        """
        
        host_id, prog_id = struct.unpack("!BB", data)
        full_prog_id = (host_id << 8) | prog_id
        
        LOGGER.debug("Unregistration of [%d:%d]", host_id, prog_id)
        
        try:
            del self._programs[full_prog_id]
        except KeyError:
            LOGGER.error("Unregistering program [%d:%d] not found", host_id,
                prog_id)
        
        if self._observer:
            self._observer.program_list_changed()
        
        return True
    
    def _handle_send_probes(self, data):
        """
        Handles probes transmission.
        """
        
        host_id, prog_id = struct.unpack("!BB", data[0:2])
        full_prog_id = (host_id << 8) | prog_id
        
        try:
            program = self._programs[full_prog_id]
        except KeyError:
            LOGGER.error("Program [%d:%d] which sent probe data is not found",
                host_id, prog_id)
            return False
        
        pos = 2
        total_length = len(data)
        
        while pos < total_length:
            probe_id = struct.unpack("!B", data[pos])[0]
            pos += 1

            try:
                probe = program.get_probe(probe_id)
            except IndexError:
                LOGGER.error("Unknown probe ID %d", probe_id)
                return False

            value, pos = probe._read_value(data, pos)
            
            if self._observer:
                self._observer.new_probe_value(probe, self._time, value)
        
        return True
    
    def _handle_send_event(self, data):
        """
        Handles events transmission.
        """
        
        host_id, prog_id, event_id = struct.unpack("!BBB", data[0:3])
        message = data[3:]
        full_prog_id = (host_id << 8) | prog_id
        
        try:
            program = self._programs[full_prog_id]
        except KeyError:
            LOGGER.error("Program [%d:%d] which sent event data is not found",
                host_id, prog_id)
            return False
        
        try:
            name, level = program.get_event(event_id)
        except IndexError:
            LOGGER.error("Incorrect event ID %d for program [%d:%d] received",
                event_id, host_id, prog_id)
            
        if self._observer:
            self._observer.new_event(program, name, level, message)
        
        return True
    
    def _update_probe_status(self, probe):
        """
        Notifies the collector that the status of a given probe has changed.
        """
        
        if not self._collector_addr:
            raise RuntimeError("Unable to update probe status: collector "
                "unknown.")
        
        host_id = (probe.program.ident >> 8) & 0xFF
        program_id = probe.program.ident & 0xFF
        probe_id = probe.ident
        
        LOGGER.debug("Updating status of probe %d on program %d:%d: enabled = "
            "%s, displayed = %s" % (probe_id, host_id, program_id,
            probe._enabled, probe._displayed))
        
        state = 2 if probe._displayed else (1 if probe._enabled else 0)
        
        message = struct.pack("!LBBBBB", MAGIC_NUMBER, MSG_MGR_SET_PROBE_STATUS,
            host_id, program_id, probe_id, state)
        
        self._sock.sendto(message, self._collector_addr)        

if __name__ == '__main__':
    import sys

    logging.basicConfig(level=logging.DEBUG)
    main_loop = gobject.MainLoop()
    controller = EnvironmentPlaneController()
    try:
        controller.register_on_collector("127.0.0.1", int(sys.argv[1]))
        main_loop.run()
    finally:
        controller.cleanup()
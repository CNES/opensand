#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
tcp_server.py - server that get Platine commands
"""

import SocketServer
import select

class Plop(SocketServer.TCPServer):
    """ The TCP socket server with reuse address to true """

    allow_reuse_address = True

    def __init__(self, server_address, RequestHandlerClass, controller):
        SocketServer.TCPServer.__init__(self, server_address, RequestHandlerClass)
        self.controller = controller

    def run(self):
        """ run the TCP server """
        try:
            self.serve_forever()
        except Exception:
            raise

    def stop(self):
        """ stop the TCP server """
        self.shutdown()


class MyTcpHandler(SocketServer.StreamRequestHandler):
    """ The RequestHandler class for the TCP server """

    # line buffered
    rbufsize = 1
    wbufsize = 0


    def setup(self):
        """ the function called when MyTcpHandler is created """
        self.connection = self.request
        self.rfile = self.connection.makefile('rb', self.rbufsize)
        self.wfile = self.connection.makefile('wb', self.wbufsize)
        self._data = ''
        self._controller = self.server.controller
        self._log = self._controller._log

    def finish(self):
        """ the function called when handle returns """
        if not self.wfile.closed:
            self.wfile.flush()
        self.wfile.close()
        self.rfile.close()

    def read_data(self, timeout = True):
        """ read data on socket """
        if timeout:
            inputready, out, err = select.select([self.rfile], [], [], 60)
            if(len(inputready) == 0):
                raise CommandException("timeout")

        self._data = self.rfile.readline().strip()
        if (self._data == ''):
            raise CommandException("distant socket is closed")


class CommandServer(MyTcpHandler):
    def handle(self):
        """ function for TCPServer """
        self._log.info("command server connected to: %s" %
                       self.client_address[0])

        try:
            self.read_data()
        except CommandException, msg:
            self._log.error("Error on command server %s" % msg)
            return
        else:
            self._log.debug("received: '%s'" % self._data)

        ret = False
        #TODO HELP
        data = self._data.split(' ', 1)
        instr = data[0]
        cmd = ''
        if len(data) > 1:
            cmd = data[1]

        try:
            if instr == 'START':
                if not cmd.isspace():
                    self._controller._model.set_run(cmd)
                else:
                    cmd = "default_i"
                ret = self._controller.start_platform()
            elif instr == 'STOP':
                ret = self._controller.stop_platform()

            if ret:
                self.wfile.write("OK\n")
            else:
                self.wfile.write("ERROR\n")
        except Exception, msg:
            self._log.error("Error on command server %s" % msg)
            self.wfile.write("ERROR\n")



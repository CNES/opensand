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
transfer_server.py - OpenSAND collector probe transfer server.
"""

from tempfile import TemporaryFile
from zipfile import ZipFile, ZIP_DEFLATED
import logging
import os
import shutil
import socket
import struct
import threading

LOGGER = logging.getLogger('sand-collector')

class TransferServer(threading.Thread):
    """
    Server listening on an arbitrary port which responds to TCP connections
    by sending the probe collection folder as a ZIP archive, preceded by
    the size of the archive.
    """

    def __init__(self, host_manager):
        super(TransferServer, self).__init__()

        self._listener = None
        self._host_manager = host_manager
        self._continue = True
        self._daemon = True
        self._sock = None

    def get_port(self):
        """
        Gets the port allocated for the socket
        """
        _, port = self._sock.getsockname()
        return port

    def __enter__(self):
        """
        Starts the background thread listening for incoming connections
        """
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.bind(('', 0))

        LOGGER.info("Transfer socket bound to port %d.", self.get_port())

        self.start()

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Tear down the socket.
        """
        try:
            self._sock.shutdown(socket.SHUT_RDWR)
        except socket.error:
            pass

        self._sock.close()

        return False

    def run(self):
        """
        Daemon thread, listening for incoming connections
        """
        self._sock.listen(0)

        while self._continue:
            self._accept_loop()

    def _accept_loop(self):
        """
        Inner method to accept a connection from the manager and handle it.
        """
        LOGGER.info("Waiting for connections...")

        # Reference socket.error before calling accept(). This exception may
        # be raised by accept() during the shutdown of the Python interpreter,
        # and in that case “socket.error” may be invalid
        socket_error = socket.error

        try:
            conn, addr = self._sock.accept()
        except socket_error:  # Socket closed
            self._continue = False
            return

        LOGGER.info("Connected to %s:%d" % addr)

        root_path = self._host_manager.switch_storage()
        LOGGER.debug("Zipping folder %s contents", root_path)

        if root_path[-1:] != "/":
            root_path = root_path + "/"

        prefix_length = len(root_path)

        paths = []
        for dirpath, dirnames, filenames in os.walk(root_path):
            paths.extend(os.path.join(dirpath, name) for name in dirnames)
            paths.extend(os.path.join(dirpath, name) for name in filenames)

        with TemporaryFile() as temp_file:
            zip_file = ZipFile(temp_file, mode='w', compression=ZIP_DEFLATED)
            for path in paths:
                archive_name = path[prefix_length:]
                LOGGER.debug("Zipping %s as %s", path, archive_name)
                zip_file.write(path, archive_name)
            zip_file.close()

            data_size = temp_file.tell()
            temp_file.seek(0)

            LOGGER.debug("Sending %d bytes of data from the file", data_size)

            conn.settimeout(60)

            try:
                conn.sendall(struct.pack("!L", data_size))

                while True:
                    data = temp_file.read(4096)

                    if data == "":
                        break

                    conn.sendall(data)
            except socket.error, msg:
                LOGGER.exception("Error sending data: %s" % str(msg))
            finally:
                try:
                    conn.shutdown(socket.SHUT_RDWR)
                except socket.error:
                    pass

                conn.close()

        LOGGER.info("Deleting folder %s", root_path)
        shutil.rmtree(root_path)

if __name__ == "__main__":
    from collections import namedtuple
    import gobject
    logging.basicConfig(level=logging.DEBUG)

    SWITCH_STORAGE = lambda: "/tmp/test"
    FAKE_MGR = namedtuple('FakeHostManager', ['switch_storage'])(SWITCH_STORAGE)

    gobject.threads_init()

    with TransferServer(FAKE_MGR):
        gobject.MainLoop().run()

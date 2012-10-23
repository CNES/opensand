# -*- coding: utf8 -*-

"""
OpenSAND collector probe transfer server.
"""

from tempfile import TemporaryFile
from zipfile import ZipFile, ZIP_DEFLATED
import logging
import os
import socket
import struct
import threading

LOGGER = logging.getLogger('transfer_server')

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
        self.daemon = True
    
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
        
        LOGGER.debug("Transfer socket bound to port %d.", self.get_port())
        
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
        LOGGER.debug("Waiting for connections...")
    
        try:
            conn, addr = self._sock.accept()
        except socket.error:  # Socket closed
            LOGGER.exception("Socket closed")
            self._continue = False
            return
        
        LOGGER.info("Connected to %s:%d", *addr)
        
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
            
            conn.settimeout(10)
            
            try:
                conn.sendall(struct.pack("!L", data_size))
                
                while True:
                    data = temp_file.read(4096)
                
                    if data == "":
                        break
                    
                    conn.sendall(data)
            except socket.error:
                LOGGER.exception("Error sending data:")
            
            finally:
                try:
                    conn.shutdown(socket.SHUT_RDWR)
                except socket.error:
                    pass
                
                conn.close()

if __name__ == "__main__":
    from collections import namedtuple
    from gtk.gdk import threads_init
    import gobject
    logging.basicConfig(level=logging.DEBUG)

    host_manager = namedtuple('FakeHostManager', ['switch_storage'])(lambda: "/tmp/test")
    
    threads_init()
    
    with TransferServer(host_manager):
        gobject.MainLoop().run()

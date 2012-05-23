#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
stream.py - transform files and directory into a stream
"""

import os
import glob
import socket
import tempfile
from base64 import encodestring

from opensand_manager_core.my_exceptions import CommandException

# macros
DATA_END = "DATA_END\n"

class Stream:
    """ transform the content of a directory or a file into a stream """

    def __init__(self, sock, manager_log):
        # sock can be used for reading or writing
        # depending on the method you use
        self._sock = sock
        self._log = manager_log
        try:
            self._tmp_file = tempfile.NamedTemporaryFile()
        except Exception:
            self._tmp_file = None
            self._log.error("exception when creating temporary file")
            raise

    def __del__(self):
        if self._tmp_file is not None:
            self._tmp_file.close()


    def send_dir(self, src_directory, dst_directory, prolog=True):
        """ send the content of a folder """
        buf = ''

        if not os.path.exists(src_directory):
            self._log.warning("directory '%s' does not exist, " \
                              "send an empty file" % src_directory)
            raise IOError

        self._log.debug("send content of folder '%s' to '%s'" %
                        (src_directory, dst_directory))

        # get the whole content of the folder and sort it
        content = glob.glob(src_directory + "/*")
        content.sort()

        if prolog:
            buf = '<?xml version="1.0" encoding="utf-8"?>\n'
            # open directory tag: put complete directory
            buf = buf + '<directory name="%s">\n' % dst_directory
        else:
            # open directory tag
            buf = buf + '<directory name="%s">\n' % \
                  os.path.basename(dst_directory)

        # write tags
        self._tmp_file.write(buf)

        # browse the folder
        for elt in content:
            # folder: call this method again
            try:
                if os.path.isdir(elt):
                    self.send_dir(elt, elt, False)
                # file: add a file object in document
                else:
                    self.send(elt, elt, False)
            except IOError:
                raise
            except Exception:
                self._log.error("unknown exception when sending content")
                raise

        # close directory tag
        buf = '</directory>\n'
        # write tag
        self._tmp_file.write(buf)

        self._tmp_file.seek(0)
        try:
            for line in self._tmp_file.readlines():
                self._sock.send(line)
        except socket.error, (errno, strerror):
            raise CommandException(strerror)
        finally:
            self._tmp_file.close()
            self._tmp_file = None


    def send(self, src_filename, dst_filename, prolog=False, mode=None):
        """ send the stream """
        buf = ''

        self._log.debug("send content of file '%s'" % src_filename)

        # open the file
        try:
            new_file = open(src_filename, "rb")
        except IOError, (errno, strerror):
            self._log.warning("error when opening '%s': %s. Send empty data"
                              % (src_filename, strerror))
            raise
        except Exception:
            self._log.error("exception when opening '%s'" % src_filename)
            raise

        name = os.path.basename(dst_filename)
        if mode is None:
            mode = os.stat(src_filename).st_mode

        if prolog:
            buf = '<?xml version="1.0" encoding="utf-8"?>\n'
            # open directory tag: put complete directory
            buf = buf + '<directory name="' + os.path.dirname(dst_filename) + \
                  '">\n'

        # open file tag
        buf = buf + '<file name="' + name + '" mode="' + str(mode) + '">'
        # open CDATA tag
        buf = buf + '<![CDATA['

        # write tags
        self._tmp_file.write(buf)

        # write raw data
        buf = new_file.readline()
        # encode buffer to avoid XML parsing errors
        buf = encodestring(buf)
        while(buf != ''):
            self._tmp_file.write(buf)
            buf = new_file.readline()
            buf = encodestring(buf)

        # close CDATA tag
        buf = buf + ']]>'
        # close file tag
        buf = buf + '</file>\n'

        if prolog:
            # close directory tag
            buf = buf + '</directory>\n'

        # write tags
        self._tmp_file.write(buf)
        new_file.close()

        if prolog:
            self._tmp_file.seek(0)
            try:
                for line in self._tmp_file.readlines():
                    self._sock.send(line)
            except socket.error, (errno, strerror):
                raise CommandException(strerror)
            finally:
                self._tmp_file.close()
                self._tmp_file = None

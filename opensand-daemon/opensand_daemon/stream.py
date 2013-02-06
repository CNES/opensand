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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
stream.py - transform files and directory into a stream
"""

import os
import shutil
import select
import logging
import tempfile
from base64 import decodestring
from xml.sax import make_parser, handler, SAXException
from opensand_daemon.my_exceptions import Timeout, InstructionError, XmlError

# macros
LOGGER = logging.getLogger('sand-daemon')
TIMEOUT = 20
DATA_END = "DATA_END\n"

class DirectoryHandler(object):
    """ transform the content of a directory into a stream """

    def __init__(self, fd):
        """ initialize the handler """
        # fd can be used for reading or writing depending on the method you use
        self._fd = fd
        self._buf = ''

    def receive_content(self):
        """ receive the content of a folder """
        # wait for 'DATA'
        inputready, out, err = select.select([self._fd], [], [], TIMEOUT)
        if(len(inputready) == 0):
            LOGGER.warning("timeout when trying to read")
            raise Timeout()

        self._buf = self._fd.readline().strip()
        # no more stream will follow
        if self._buf == 'STOP':
            LOGGER.debug("last stream received")
            return False
        # wrong instruction
        if not self._buf == 'DATA':
            LOGGER.error("bad instruction: '%s' (expecting 'DATA')" % self._buf)
            raise InstructionError("bad instruction: '%s' (expecting 'DATA')" %
                                   self._buf)

        LOGGER.debug("received: 'DATA'")

        try:
            self.parse()
        except:
            raise

        return True

    def parse(self):
        """ parse an XML stream """
        end_tag = DATA_END.strip()

        # create the XML parser
        parser = make_parser()
        parser.setContentHandler(XmlParser())
        # parse the document
        try:
            stop = False
            read = False
            while not stop:
                for line in self._fd:
                    if(line.strip().endswith(end_tag) == True):
                        LOGGER.debug("received: '%s'" % end_tag)
                        pos = line.rfind(end_tag)
                        line = line[0:pos]
                        parser.feed(line)
                        stop = True
                        read = True
                        break
                    else:
                        parser.feed(line)

                if not stop:
                    if not read:
                        LOGGER.warning('Remote peer disconnected')
                        raise Timeout()
                    else:
                        read = False
                    inputready, out, err = \
                      select.select([self._fd], [], [], TIMEOUT)
                    if(len(inputready) == 0):
                        LOGGER.warning("timeout when trying to read")
                        raise Timeout()
        except SAXException, ex:
            LOGGER.error("error when parsing the XML stream")
            raise XmlError(ex.getMessage())
        else:
            parser.close()


class XmlParser(handler.ContentHandler):
    """ parse an XML file containing a directory tree or a file """

    def __init__(self):
        self._root_dir = ''
        self._curr_dir = ''
        self._curr_filename = ''
        self._tmp_file = None
        self._is_file = False
        self._curr_mode = -1
        self._directory_tree = []


    def startDocument(self):
        """ start parsing the XML document """
        LOGGER.debug("start parsing XML document")

    def startElement(self, node_name, attrs):
        """ start of an XML element """
        if 'name' not in attrs.getNames():
            LOGGER.error("Missing mandatory attribute 'name' " \
                         "in node '%s'" % node_name)
            raise SAXException("Missing mandatory attribute 'name' " \
                               "in node '%s'" % node_name)

        name = attrs.getValue('name')

        if(node_name == "directory"):
            if self._root_dir == '':
                self._root_dir = name
                self._curr_dir = name
            else:
                self._curr_dir = self._curr_dir + "/" + name
            self._curr_dir = self._curr_dir.replace("//", "/")
            self._directory_tree.append(self._curr_dir)
            LOGGER.debug("enter directory '%s'" % self._curr_dir)
            if(os.path.exists(self._curr_dir) == False):
                try:
                    os.makedirs(self._curr_dir) #TODO mode
                except OSError, (errno, strerror):
                    msg = ("Error when creating directory '%s' (%d: %s)" %
                            (self._curr_dir, errno, strerror))
                    LOGGER.error(msg)
                    raise SAXException(msg)
                except Exception:
                    msg = ("exception when creating directory '%s'" %
                            self._curr_dir)
                    LOGGER.error(msg)
                    raise SAXException(msg)
        elif(node_name == "file"):
            if(name == ''):
                LOGGER.warning("the file we try to transmit does not " + \
                               "exist on manager host !")
                raise SAXException("the file we try to transmit does not exist "
                                   "on manager host")
            if 'mode' in attrs.getNames():
                mode = attrs.getValue('mode')
                try:
                    self._curr_mode = int(mode)
                except ValueError:
                    LOGGER.warning('the file has mode attribute which ' \
                                   'is not integer: %s' % mode)
                    raise SAXException("the has mode attibute which is not "
                                       "interger: %s" % mode)

            self._curr_filename = self._curr_dir + "/" + name
            self._curr_filename = self._curr_filename.replace("//", "/")
            self._directory_tree.append(self._curr_filename)
            LOGGER.debug("create temporary file")
            try:
                self._tmp_file = tempfile.NamedTemporaryFile()
            except Exception:
                LOGGER.error("exception when creating temporary file")
                raise SAXException("exception when creating temporary file")
            self._is_file = True

    def endElement(self, name):
        """ end of a XML element """
        if(name == "directory"):
            LOGGER.debug("quit '%s'" % self._curr_dir)
            self._curr_dir = os.path.dirname(self._curr_dir)
        elif(name == "file" and self._is_file == True):
            try:
                self._tmp_file.flush()
                shutil.copyfile(self._tmp_file.name, self._curr_filename)
                if self._curr_mode != -1:
                    os.chmod(self._curr_filename, self._curr_mode)
            except (IOError, OSError), (errno, strerror):
                msg = ("error when copying '%s' to '%s' (%d: %s)" %
                        (self._tmp_file.name, self._curr_filename,
                        errno, strerror))
                LOGGER.error(msg)
                raise SAXException(msg)
            except shutil.Error, (srcname, dstname, exception):
                msg = ("error when copying '%s' to '%s' (%s)" %
                       (srcname, dstname, exception))
                LOGGER.error(msg)
                raise SAXException(msg)
            finally:
                self._tmp_file.close()

            self._is_file = False
            LOGGER.debug("file '%s' written" % self._curr_filename)
            self._curr_filename = ''
            self._curr_mode = -1
            self._tmp_file = None

    def characters(self, content):
        """ content of an element """
        if self._is_file and  self._tmp_file is not None:
                # do not forget to decode data
                self._tmp_file.write(decodestring(content))

    def endDocument(self):
        """ end of the XML document """
        LOGGER.debug("stop parsing XML document")
        LOGGER.debug("directory tree: " + str(self._directory_tree))

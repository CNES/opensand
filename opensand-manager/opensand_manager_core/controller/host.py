#!/usr/bin/env python
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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
host.py - controller that configure, install, start, stop
          and get status of OpenSAND processes on a specific host
"""

import threading
import socket
import thread
import os
import ConfigParser
import tempfile

from opensand_manager_core.my_exceptions import CommandException
from opensand_manager_core.controller.stream import Stream

CONF_DESTINATION_PATH = '/etc/opensand/'
START_DESTINATION_PATH = '/var/cache/sand-daemon/start.ini'
DATA_END = 'DATA_END\n'

#TODO factorize
class HostController:
    """ controller which implements the client that connects in order to get
        program states on distant host and the client that sends command to
        the distant host """
    def __init__(self, host_model, manager_log):
        self._host_model = host_model
        self._log = manager_log

        self._state_thread = threading.Thread(None, self.state_client,
                                              None, (), {})
        self._stop = threading.Event()
        self._state_thread.start()

    def close(self):
        """ close the host connections """
        self._log.debug(self.get_name() + ": close")
        self._stop.set()
        self._log.debug(self.get_name() + ": join state client")
        self._state_thread.join(10)
        self._log.debug(self.get_name() +
                        ": state client joined")
        self._log.debug(self.get_name() + ": closed")

    def get_name(self):
        """ get the name of host """
        return self._host_model.get_name().upper()

    def state_client(self):
        """ client that check for host state """
        # address of the state server
        address = (self._host_model.get_ip_address(),
                   self._host_model.get_state_port())
        # create the socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)

        # connect to the server
        try:
            sock.connect(address)
            self._log.debug(self.get_name() + ": connected to state server")
        except socket.error, strerror:
            self._log.error("%s: unable to connect to state server (%s:%s): " \
                            "'%s'" % (self.get_name(), address[0],
                            address[1], strerror))
            self._host_model.set_started(None)
            thread.exit()

        sock.send('STATE\n')
        self._log.debug("%s: send 'STATE' to state server" % self.get_name())

        # reduce the timeout to avoid blocking too long in the loop
        sock.settimeout(1)
        while not self._stop.is_set():
            # get the periodic reports
            try:
                received = sock.recv(512).strip()
            except socket.timeout:
                continue
            except Exception, msg:
                self._log.error("%s: exception when receiving message from " \
                                "state server (%s)" % (self.get_name(), msg))
                sock.close()
                thread.exit()

            self._log.debug("%s: received '%s' from state server" %
                            (self.get_name(), received))
            cmd = received.split()
            if len(cmd) < 1 or cmd[0] != 'STARTED':
                sock.send("ERROR wrong command '%s' \n" % cmd)
                self._log.error("%s: received '%s' while waiting for 'STARTED'"
                                % (self.get_name(), cmd))
                self._host_model.set_started(None)
                sock.close()
                thread.exit()
            self._host_model.set_started(cmd[1:])

        sock.send('BYE\n')
        self._log.debug("%s: send 'BYE' to state server" % self.get_name())
        try:
            received = sock.recv(512).strip()
        except socket.timeout:
            pass
        else:
            self._log.debug("%s: received '%s' from state server" %
                            (self.get_name(), received))
        finally:
            sock.close()

    def configure(self, conf_files, scenario, run,
                  deploy_config, dev_mode=False):
        """ send the configure command to command server """
        # connect to command server and send the configure command
        try:
            sock = self.connect_command('CONFIGURE')
        except CommandException:
            raise

        if sock is None:
            return

        try:
            for conf in conf_files:
                # send the configuration file
                self.send_file(sock, conf,
                               os.path.join(CONF_DESTINATION_PATH,
                                            os.path.basename(conf)))
                # TODO handle modcod and dra for gw
                #      (according to regen/transp et coll/ind)
        except CommandException:
            sock.close()
            raise

        # stop after configuration if disabled or if dev mode and no deploy
        # information
        if not self._host_model.is_enabled() or \
           (dev_mode and not self._host_model.get_name() in
            deploy_config.sections()):
            if self._host_model.is_enabled():
                self._log.warning("%s :disabled because it has no deploy "
                                  "information" % self.get_name())
                self.disable()

            # send an empty start.ini file because we will send the start
            # command in order to apply routes on host
            with tempfile.NamedTemporaryFile() as tmp_file:
                self.send_file(sock, tmp_file.name, START_DESTINATION_PATH)

            try:
                # send 'STOP' tag
                sock.send('STOP\n')
                self._log.debug("%s: send 'STOP'" % self.get_name())
            except socket.error, strerror:
                self._log.error("Cannot contact %s command server: %s" %
                                (self.get_name(), strerror))
                raise CommandException("Cannot contact %s command server: %s" %
                                        (self.get_name(), strerror))

            try:
                self.receive_ok(sock)
            except CommandException:
                sock.close()
                raise

            sock.close()
            return

        prefix = '/'
        if deploy_config.has_option('prefix', 'destination'):
            prefix = deploy_config.get('prefix', 'destination')

        component = self._host_model.get_name()

        ld_library_path = '/'
        if deploy_config.has_option(component, 'ld_library_path'):
            ld_library_path = deploy_config.get(component, 'ld_library_path')
            ld_library_path = os.path.join(prefix, ld_library_path.lstrip('/'))

        if not dev_mode:
            bin_file = self._host_model.get_component()
        else:
            bin_file = deploy_config.get(self._host_model.get_name(), 'binary')
            bin_file = os.path.join(prefix, bin_file.lstrip('/'))

        # create the start.ini file
        start_ini = ConfigParser.SafeConfigParser()
        command_line = '%s -i %s -a %s -n %s' % \
                       (bin_file, self._host_model.get_instance(),
                        self._host_model.get_emulation_address(),
                        self._host_model.get_emulation_interface())
        try:
            start_ini.add_section(self._host_model.get_component())
            start_ini.set(self._host_model.get_component(), 'command',
                          command_line)
            if dev_mode:
                # Add custom library path
                start_ini.set(self._host_model.get_component(),
                              'ld_library_path', ld_library_path)
            # add tools in start.ini and send tools configuration
            self.configure_tools(sock, start_ini, deploy_config, dev_mode)
            # send the start.ini file
            with tempfile.NamedTemporaryFile() as tmp_file:
                start_ini.write(tmp_file)
                tmp_file.flush()
                self.send_file(sock, tmp_file.name, START_DESTINATION_PATH)
        except ConfigParser.Error, msg:
            self._log.error("Cannot create start.ini file: " + msg)
            sock.close()
            raise CommandException("Cannot create start.ini file")
        except CommandException:
            sock.close()
            raise

        try:
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % self.get_name())
        except socket.error, strerror:
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            raise CommandException("Cannot contact %s command server: %s" %
                                    (self.get_name(), strerror))

        try:
            self.receive_ok(sock)
        except CommandException:
            sock.close()
            raise

        sock.close()

    def configure_ws(self, deploy_config, dev_mode=False):
        """ send the configure command to command server on WS """
        # connect to command server and send the configure command
        try:
            sock = self.connect_command('CONFIGURE')
        except CommandException:
            raise

        if sock is None:
            return
        # create the start.ini file
        start_ini = ConfigParser.SafeConfigParser()
        try:
            # add tools in start.ini and send tools configuration
            self.configure_tools(sock, start_ini, deploy_config, dev_mode)
            # send the start.ini file
            with tempfile.NamedTemporaryFile() as tmp_file:
                start_ini.write(tmp_file)
                tmp_file.flush()
                self.send_file(sock, tmp_file.name, START_DESTINATION_PATH)
        except ConfigParser.Error, msg:
            self._log.error("Cannot create start.ini file: " + msg)
            sock.close()
            raise CommandException("Cannot create start.ini file")
        except CommandException:
            sock.close()
            raise

        try:
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % self.get_name())
        except socket.error, (errno, strerror):
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            raise CommandException("Cannot contact %s command server: %s" %
                                    (self.get_name(), strerror))

        try:
            self.receive_ok(sock)
        except CommandException:
            sock.close()
            raise

        sock.close()


    def configure_tools(self, sock, start_ini, deploy_config, dev_mode=False):
        """ send tools configuration and add command lines in start.ini file """
        prefix = '/'
        if deploy_config.has_option('prefix', 'destination'):
            prefix = deploy_config.get('prefix', 'destination')
        ld_library_path = '/'

        for tool in self._host_model.get_tools():
            # check if the tool is selected
            if not tool.is_selected():
                continue

            try:
                # send the configuration file
                self.send_file(sock, tool.get_conf_src(),
                                     tool.get_conf_dst())
                # send additionnal files
                for (src_conf, dest_conf) in \
                            tool.get_conf_files().iteritems():
                    self.send_file(sock, src_conf, dest_conf)

                # Create start.ini file
                start_ini.add_section(tool.get_name())
                start_ini.set(tool.get_name(), 'command',
                              tool.get_command())

                if dev_mode and \
                   deploy_config.has_option(tool.get_name(), 'ld_library_path'):
                    # Add custom library path
                    ld_library_path = deploy_config.get(tool.get_name(),
                                                        'ld_library_path')
                    ld_library_path = os.path.join(prefix,
                                                   ld_library_path.lstrip('/'))
                    start_ini.set(tool.get_name(),
                                  'ld_library_path', ld_library_path)
            except ConfigParser.Error, msg:
                self._log.error("Cannot add %s command line to %s "
                                "start.ini file: %s" %
                                (tool.get_name(), self.get_name(),
                                 str(msg)))
                raise
            except CommandException:
                self._log.error("Cannot copy %s configuration on %s" %
                                (tool.get_name(), self.get_name()))
                raise

    def send_file(self, sock, src_file, dst_file, mode=None):
        """ send a file """
        self._log.debug("Send file '%s' to dest '%s'" % (src_file, dst_file)) 
        try:
            # send 'DATA' tag
            sock.send('DATA\n')
            self._log.debug("%s: send 'DATA'" % self.get_name())
        except socket.error, strerror:
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            raise CommandException("Cannot contact %s command server: %s" %
                                   (self.get_name(), strerror))

        try:
            # create the stream handler
            stream_handler = Stream(sock, self._log)
        except Exception:
            raise CommandException

        try:
            stream_handler.send(src_file, dst_file, True, mode)
        except CommandException as error:
            self._log.error("%s: error when sending stream: %s" %
                            (self.get_name(), error))
            raise CommandException("%s: error when sending stream: %s" %
                                   (self.get_name(), error))
        except Exception:
            self._log.error("%s: error when sending file '%s'" %
                            (self.get_name(), file))
            sock.close()
            raise

        # send 'DATA_END' tag
        try:
            sock.send(DATA_END)
            self._log.debug("%s: send '%s'" %
                            (self.get_name(), DATA_END.strip()))
        except socket.error, strerror:
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            raise CommandException("Cannot contact %s command server: %s" %
                                    (self.get_name(), strerror))

        try:
            self.receive_ok(sock)
        except CommandException:
            sock.close()
            raise

    def deploy(self, deploy_config):
        """ send the deploy command to command server """
        try:
            sock = self.connect_command('DEPLOY')
        except CommandException:
            raise

        if sock is None:
            return

        try:
            component = self._host_model.get_name()
            self.deploy_files(component, sock, deploy_config)
            for tool in self._host_model.get_tools():
                self.deploy_files(tool.get_name(), sock, deploy_config,
                                  is_tool=True)
        except CommandException:
            self._log.warning("unable to send files to %s" % self.get_name())
            sock.close()
            raise

        try:
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % self.get_name())
        except socket.error, strerror:
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            raise CommandException("Cannot contact %s command server: %s" %
                                   (self.get_name(), strerror))

        try:
            self.receive_ok(sock)
        except CommandException:
            raise

        sock.close()

    def deploy_files(self, component, sock, config, is_tool=False):
        """ send the host files for deployment """

        self._log.debug("Deploy files for component: %s" % component)

        try:
            # create the stream handler
            stream_handler = Stream(sock, self._log)
        except Exception:
            raise CommandException

        src_prefix = '/'
        if config.has_option('prefix', 'source'):
            src_prefix = config.get('prefix', 'source')

        dst_prefix = '/'
        if config.has_option('prefix', 'destination'):
            dst_prefix = config.get('prefix', 'destination')

        self._log.debug('Source prefix is: %s' % src_prefix)
        self._log.debug('Destination prefix is: %s' % dst_prefix)

        # check if options are directories or files
        directories = {}
        files = {}

        sections = [component]
        if not is_tool and config.has_section('common'):
            sections.append('common')
        
        for section in sections:
            if not config.has_option(section, 'files'):
                self._log.warning("No files value for section '%s'" % section)
                continue
            for elt in map(str.strip, config.get(section, 'files').split(',')):
                path = os.path.join(src_prefix, elt)
                if os.path.isfile(path):
                    dst_path = elt.lstrip('/')
                    files[path] = os.path.join(dst_prefix, dst_path)
                elif os.path.isdir(path):
                    dst_path = elt.lstrip('/')
                    directories[path] = os.path.join(dst_prefix, dst_path)
                else:
                    self._log.error("'%s' does no exist on system" % path)
                    raise CommandException

        for (directory, dst_directory) in directories.iteritems():
            try:
                # send 'DATA' tag
                sock.send('DATA\n')
                self._log.debug("%s: send 'DATA'" % self.get_name())
            except socket.error, strerror:
                self._log.error("Cannot contact %s command server: %s" %
                                (self.get_name(), strerror))
                raise CommandException("Cannot contact %s command server: %s" %
                                        (self.get_name(), strerror))

            try:
                stream_handler.send_dir(directory, dst_directory)
            except CommandException as error:
                self._log.error("%s: error when sending stream: %s" %
                                (self.get_name(), error))
                raise CommandException("%s: error when sending stream: %s" %
                                       (self.get_name(), error))
            except Exception:
                self._log.error("%s: error when sending directory '%s'" %
                                (self.get_name(), directory))
                raise CommandException("%s: error when sending directory '%s'" %
                                       (self.get_name(), directory))

            try:
                # send 'DATA_END' tag
                sock.send(DATA_END)
                self._log.debug("%s: send '%s'" %
                                (self.get_name(), DATA_END.strip()))
            except socket.error, strerror:
                self._log.error("Cannot contact %s command server: %s" %
                                (self.get_name(), strerror))
                raise CommandException("Cannot contact %s command server: %s" %
                                        (self.get_name(), strerror))

            try:
                self.receive_ok(sock)
            except CommandException:
                raise

        for (file, dst_file) in files.iteritems():
            try:
                self.send_file(sock, file, dst_file)
            except CommandException:
                raise

    def start_stop(self, command):
        """ send the start or stop command to host server """
        if command != 'START' and command != 'STOP':
            self._log.error("%s: wrong command %s" % (self.get_name(), command))

        try:
            sock = self.connect_command(command)
        except CommandException:
            raise

        if sock is not None:
            sock.close()

    def connect_command(self, command):
        """ connect to command server and send a command """
        if self._host_model.get_state() is None:
            self._log.error("cannot get %s status" % self.get_name())
            return None

        address = (self._host_model.get_ip_address(),
                   self._host_model.get_command_port())
        # create the socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(60)

        # connect to the server
        try:
            sock.connect(address)
            self._log.debug("%s: connected to command server" % self.get_name())
        except socket.error, strerror:
            self._log.error("%s: unable to connect to command server ('%s')" %
                            (self.get_name(), strerror))
            raise CommandException("Connection error to " + self.get_name())

        try:
            sock.send(command +'\n')
            self._log.debug("%s: send %s to command server" %
                            (self.get_name(), command))
        except socket.error, strerror:
            sock.close()
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            raise CommandException("Cannot contact %s command server: %s" %
                                   (self.get_name(), strerror))

        try:
            self.receive_ok(sock)
        except CommandException:
            sock.close()
            raise

        return sock

    def receive_ok(self, sock):
        """ receive OK on a socket and handle exceptions """
        try:
            received = sock.recv(512).strip()
        except socket.timeout:
            self._log.error("%s: timeout with command server" % self.get_name())
            raise CommandException("%s: timeout with command server" %
                                   self.get_name())
        except socket.error, strerror:
            self._log.error("%s: error when reading on server: %s" %
                            (self.get_name(), strerror))
            raise CommandException("%s: error when reading on server: %s" %
                                   (self.get_name(), strerror))
        if received != "OK":
            self._log.error("%s: received '%s' while waiting for 'OK'" %
                            (self.get_name(), received))
            raise CommandException("%s: server answers '%s' while waiting " \
                                   "for 'OK'" % (self.get_name(), received))

    def disable(self):
        """ diasable the host """
        self._host_model.enable(False)

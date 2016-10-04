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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>

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
import time

from opensand_manager_core.my_exceptions import CommandException
from opensand_manager_core.controller.stream import Stream
from opensand_manager_core.utils import ST, SAT, GW, GW_types, GW_NET_ACC, GW_PHY

CONF_DESTINATION_PATH = '/etc/opensand/'
START_DESTINATION_PATH = '/var/cache/sand-daemon/start.ini'
START_INI = 'start.ini'
DATA_END = 'DATA_END\n'

#TODO factorize
class MachineController:
    """ controller which implements the client that connects in order to get
        program states on distant host and the client that sends command to
        the distant host """
    def __init__(self, machine_model, manager_log,
                 cache_dir='/var/cache/sand-daemon/'):
        self._machine_model = machine_model
        self._log = manager_log

        self._state_thread = threading.Thread(None, self.state_client,
                                              "State", (), {})
        self._stop = threading.Event()
        self._state_thread.start()
        self._cache = START_DESTINATION_PATH
        if cache_dir is not None:
            self._cache = os.path.join(cache_dir, START_INI)

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
        return self._machine_model.get_name().upper()
    
    def state_client(self):
        """ client that check for host state """
        # address of the state server
        address = (self._machine_model.get_ip_address(),
                   self._machine_model.get_state_port())
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
            self._machine_model.set_started(None)
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
            if len(cmd) < 1:
                self._log.warning("%s: socket seems to be closed by host" %
                                  self.get_name())
                self._machine_model.set_started(None)
                sock.close()
                thread.exit()
            if cmd[0] != 'STARTED':
                sock.send("ERROR wrong command '%s' \n" % cmd)
                self._log.error("%s: received '%s' while waiting for 'STARTED'"
                                % (self.get_name(), cmd))
                self._machine_model.set_started(None)
                sock.close()
                thread.exit()
            self._machine_model.set_started(cmd[1:])

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

    def configure(self, conf_files, conf_modules,
                  deploy_config, dev_mode=False, errors=[]):
        """ send the configure command to command server """
        # connect to command server and send the configure command
        sock = None
        try:
            sock = self.connect_command('CONFIGURE')
        except CommandException, msg:
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        if sock is None:
            return

        try:
            for conf in conf_files:
                # send the configuration file
                self.send_file(sock, conf,
                               os.path.join(CONF_DESTINATION_PATH,
                                            os.path.basename(conf)))

            plugin_path = os.path.join(CONF_DESTINATION_PATH, 'plugins')
            for module_dir in conf_modules:
                self.send_dir(sock, module_dir, plugin_path)
        except CommandException, msg:
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        # stop after configuration if disabled or if dev mode and no deploy
        # information
        if not self._machine_model.is_enabled() or \
           (dev_mode and (not self._machine_model.get_name() in
                          deploy_config.sections() and 
                          not self._machine_model.get_component() in
                          deploy_config.sections())):
            if self._machine_model.is_enabled():
                self._log.warning("%s: disabled because it has no deploy "
                                  "information" % self.get_name())
                self.disable()

            # send an empty start.ini file because we will send the start
            # command in order to apply routes on host
            with tempfile.NamedTemporaryFile() as tmp_file:
                self.send_file(sock, tmp_file.name, self._cache)

            try:
                # send 'STOP' tag
                sock.send('STOP\n')
                self._log.debug("%s: send 'STOP'" % self.get_name())
            except socket.error, strerror:
                self._log.error("Cannot contact %s command server: %s" %
                                (self.get_name(), strerror))
                errors.append("%s: %s" % (self.get_name(), strerror))
                return

            try:
                self.receive_ok(sock)
            except CommandException, msg:
                sock.close()
                errors.append("%s: %s" % (self.get_name(), msg))
                return

            sock.close()
            return

        prefix = '/'
        if deploy_config.has_option('prefix', 'destination'):
            prefix = deploy_config.get('prefix', 'destination')

        component = self._machine_model.get_name()
        if not component  in deploy_config.sections():
            component = self._machine_model.get_component()
            if not component in deploy_config.sections():
                self._log.error("Cannot create start.ini file: no "\
                                "information about %s (%s) deployment" %
                                (self._machine_model.get_name(), component))
                sock.close()
                errors.append("%s: no information about %s deployment" %
                              (self.get_name(), self._machine_model.get_name()))
                return

        ld_library_path = '/'
        if deploy_config.has_option(component, 'ld_library_path'):
            ld_library_path = deploy_config.get(component, 'ld_library_path')
            ld_library_path = os.path.join(prefix, ld_library_path.lstrip('/'))

        if not dev_mode:
            bin_file = self._machine_model.get_component()
        else:
            bin_file = deploy_config.get(component, 'binary')
            bin_file = os.path.join(prefix, bin_file.lstrip('/'))

        # create the start.ini file
        start_ini = ConfigParser.SafeConfigParser()
        instance_param = ''
        if component.startswith(ST) or component.startswith(GW):
            instance_param = '-i ' + self._machine_model.get_instance()
        lan_iface = ''
        if component not in {SAT, GW_PHY}:
            lan_iface = '-l ' + self._machine_model.get_lan_interface()
        if component == GW_NET_ACC:
            command_line = '%s %s %s -u %s -w %s' % \
                           (bin_file, instance_param, lan_iface,
                            self._machine_model.get_upward_port(),
                            self._machine_model.get_downward_port())
        elif component == GW_PHY:
            command_line = '%s %s -a %s -n %s -t %s -u %s -w %s' % \
                           (bin_file, instance_param, 
                            self._machine_model.get_emulation_address(),
                            self._machine_model.get_emulation_interface(),
                            self._machine_model.get_remote_ip_addr(),
                            self._machine_model.get_upward_port(),
                            self._machine_model.get_downward_port())
        else:
            command_line = '%s -a %s -n %s %s %s' % \
                           (bin_file,
                            self._machine_model.get_emulation_address(),
                            self._machine_model.get_emulation_interface(),
                            lan_iface, instance_param)
        if not self._machine_model.is_collector_functional():
            command_line += " -q"
        try:
            start_ini.add_section(self._machine_model.get_component())
            start_ini.set(self._machine_model.get_component(), 'command',
                          command_line)
            if dev_mode:
                # Add custom library path
                start_ini.set(self._machine_model.get_component(),
                              'ld_library_path', ld_library_path)
            # add tools in start.ini and send tools configuration
            self.configure_tools(sock, start_ini, deploy_config, dev_mode)
            # send the start.ini file
            with tempfile.NamedTemporaryFile() as tmp_file:
                start_ini.write(tmp_file)
                tmp_file.flush()
                self.send_file(sock, tmp_file.name, self._cache)
        except ConfigParser.Error, msg:
            self._log.error("Cannot create start.ini file: " + msg)
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return
        except CommandException, msg:
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        try:
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % self.get_name())
        except socket.error, strerror:
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            errors.append("%s: %s" % (self.get_name(), strerror))
            return

        try:
            self.receive_ok(sock)
        except CommandException, msg:
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        sock.close()

    def configure_ws(self, deploy_config, dev_mode=False, errors=[]):
        """ send the configure command to command server on WS """
        # connect to command server and send the configure command
        sock = None
        try:
            sock = self.connect_command('CONFIGURE')
        except CommandException, msg:
            errors.append("%s: %s" % (self.get_name(), msg))
            return

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
                self.send_file(sock, tmp_file.name, self._cache)
        except ConfigParser.Error, msg:
            self._log.error("Cannot create start.ini file: " + msg)
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return
        except CommandException, msg:
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        try:
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % self.get_name())
        except socket.error, (_, strerror):
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            errors.append("%s: %s" % (self.get_name(), strerror))
            return

        try:
            self.receive_ok(sock)
        except CommandException, msg:
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        sock.close()


    def configure_tools(self, sock, start_ini, deploy_config, dev_mode=False):
        """ send tools configuration and add command lines in start.ini file """
        prefix = '/'
        if deploy_config.has_option('prefix', 'destination'):
            prefix = deploy_config.get('prefix', 'destination')
        ld_library_path = '/'

        for tool in self._machine_model.get_tools():
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

    def deploy_modified_files(self, files, errors=[]):
        """ send some files """
        sock = None
        try:
            sock = self.connect_command('CONFIGURE')
            if sock is None:
                return

            for (src, dst) in files:
                self.send_file(sock, src, dst)
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % self.get_name())
        except IOError, msg:
            self._log.error("Cannot install simulation files: %s" % msg)
            errors.append("%s: %s" % (self.get_name(), msg))
            return
        except socket.error, (_, strerror):
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            errors.append("%s: %s" % (self.get_name(), strerror))
            return
        except CommandException, msg:
            self._log.error("cannot install simulation files")
            errors.append("%s: %s" % (self.get_name(), msg))
            return
        else:
            self._machine_model.set_deployed()
        finally:
            if sock is not None:
                sock.close()

        return True

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
        except BaseException, error:
            self._log.error("%s: error when sending file '%s': %s" %
                            (self.get_name(), src_file, error))
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

    def send_dir(self, sock, src_dir, dst_dir):
        """ send a directory """
        self._log.debug("Send directory '%s' to dest '%s'" % (src_dir, dst_dir)) 
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
            stream_handler.send_dir(src_dir, dst_dir)
        except CommandException as error:
            self._log.error("%s: error when sending stream: %s" %
                            (self.get_name(), error))
            raise CommandException("%s: error when sending stream: %s" %
                                   (self.get_name(), error))
        except BaseException, error:
            self._log.error("%s: error when sending directory '%s': %s" %
                            (self.get_name(), src_dir, error))
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

    def deploy(self, deploy_config, errors=[]):
        """ send the deploy command to command server """
        sock = None
        try:
            sock = self.connect_command('DEPLOY')
        except CommandException, msg:
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        if sock is None:
            return
        
        # try with component, then with host name
        component = self._machine_model.get_name()
        if not component  in deploy_config.sections():
            if component.startswith(ST):
                component = ST
            elif component.startswith(GW_NET_ACC):
                component = GW_NET_ACC
            elif component.startswith(GW_PHY):
                component = GW_PHY
            elif component.startswith(GW):
                component = GW
        if not component in deploy_config.sections():
            component = self._machine_model.get_host_name()
        try:
            self.deploy_files(component, sock, deploy_config)
            for tool in self._machine_model.get_tools():
                self.deploy_files(tool.get_name(), sock, deploy_config,
                                  is_tool=True)
        except CommandException, msg:
            self._log.warning("unable to send files to %s" % self.get_name())
            sock.close()
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        try:
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % self.get_name())
        except socket.error, strerror:
            self._log.error("Cannot contact %s command server: %s" %
                            (self.get_name(), strerror))
            errors.append("%s: %s" % (self.get_name(), strerror))
            return

        try:
            self.receive_ok(sock)
        except CommandException, msg:
            errors.append("%s: %s" % (self.get_name(), msg))
            return

        sock.close()

    def deploy_files(self, component, sock, config, is_tool=False):
        """ send the host files for deployment """

        self._log.debug("Deploy files for component: %s" % component)

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
                self.send_dir(sock, directory, dst_directory)
            except CommandException:
                raise

        for (src_file, dst_file) in files.iteritems():
            try:
                self.send_file(sock, src_file, dst_file)
            except CommandException:
                raise

    def start_stop(self, command):
        """ send the start or stop command to host server """
        if not command.startswith('START') and not command.startswith('STOP'):
            self._log.error("%s: wrong command %s" % (self.get_name(), command))

        sock = None
        try:
            sock = self.connect_command(command)
        except CommandException:
            raise

        if sock is not None:
            sock.close()

    def connect_command(self, command):
        """ connect to command server and send a command """
        if self._machine_model.get_state() is None:
            time.sleep(0.1)
            if self._machine_model.get_state() is None:
                self._log.error("cannot get %s status" % self.get_name())
                return None

        address = (self._machine_model.get_ip_address(),
                   self._machine_model.get_command_port())
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
        """ disable the host """
        self._machine_model.enable(False)

    def get_interface_type(self):
        """ get the type of interface according to the stack """
        return self._machine_model.get_interface_type()

    def get_deploy_files(self):
        """ get the files to deploy """
        return self._machine_model.get_deploy_files()

    def first_deploy(self):
        """ check if this is the first deploy """
        return self._machine_model.first_deploy()


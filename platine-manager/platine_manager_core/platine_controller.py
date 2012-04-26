#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
#
#
# This file is part of the Platine testbed.
#
#
# Platine is free software : you can redistribute it and/or modify it under the
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
platine_controller.py - thread that configure, install, start, stop
                        and get status of all Platine processes
"""

import threading
import os
import shutil
import array
import struct
import socket
import fcntl
import tempfile
import ConfigParser
import stat

from platine_manager_core.my_exceptions import CommandException
from platine_manager_core.controller.service_listener import PlatineServiceListener
from platine_manager_core.controller.environment_plane import EnvironmentPlaneController
from platine_manager_core.controller.tcp_server import Plop, CommandServer
from platine_manager_core.controller.stream import Stream
from platine_manager_core.utils import copytree

SCRIPT_PATH = '/usr/libexec/platine/'
DEFAUL_PATH = '/usr/share/platine/'
DEFAULT_INI_FILE = '/usr/share/platine/deploy.ini'
COM_PARAMETERS = '/etc/platine/env_plane/com_parameters.conf'
CMD_PORT = 5656
DATA_END = 'DATA_END\n'

class Controller(threading.Thread):
    """ controller that controll all hosts """
    def __init__ (self, model, service_type, manager_log, interactive=False):
        try:
            threading.Thread.__init__(self)
            self._model = model
            self._log = manager_log
            self._event_manager = self._model.get_event_manager()
            self._event_manager_response = self._model.get_event_manager_response()
            self._hosts = []
            self._ws = []
            self._env_plane = EnvironmentPlaneController(model.get_env_plane(),
                                                         manager_log)
            self._server = None
            self._command = None

            # The configuration of deployment
            self._deploy_config = None

            if not 'HOME' in os.environ:
                self._log.warning("cannot get $HOME environment variable, "
                                  "default deploy file will be used")
            else:
                ini_file = os.path.join(os.environ['HOME'],
                                        ".platine/deploy.ini")
                if not os.path.exists(ini_file):
                    self._log.debug("cannot find file %s, " \
                                    "copy default" % ini_file)
                    try:
                        shutil.copy(DEFAULT_INI_FILE,
                                    ini_file)
                    except IOError, msg:
                        self._log.warning("failed to copy %s configuration file "
                                          "in '%s': %s, default deploy file "
                                          "will be used"
                                          % (DEFAULT_INI_FILE, ini_file, msg))

            # create the service browser here because we need hosts as argument
            # but it will be started with gtk main loop
            PlatineServiceListener(self._model, self._hosts, self._ws,
                                   service_type, self._log)

            if interactive:
                self._command = threading.Thread(None, self.start_server, None, (), {})
        except Exception:
            self.close()
            raise

    def update_com_parameters(self, addr):
        """ modify the output address in com_parameters.conf file """
        with open(COM_PARAMETERS) as com_param:
            lines = com_param.readlines()
            new_lines = []
            edit = False
            for line in lines:
                if edit and addr is not None:
                    if line == '}\n':
                        edit = False
                    elif line != '{\n':
                        elt = line.split(',')
                        line = elt[0] + ", " + addr + "," + elt[2]
                if line == 'Controllers_ports\n':
                    edit = True
                new_lines.append(line)

            return new_lines

    def send_com_parameters(self, host, content):
        """ send the com_parametes.conf file on hosts """
        sock = None
        with tempfile.NamedTemporaryFile() as tmp_file:
            tmp_file.writelines(content)
            tmp_file.flush()
            try:
                sock = host.connect_command('DEPLOY')
                if sock is None:
                    return
                mode = stat.S_IRUSR | stat.S_IWUSR  \
                       | stat.S_IRGRP | stat.S_IROTH
                host.send_file(sock, tmp_file.name, COM_PARAMETERS, mode)
                # send 'STOP' tag
                sock.send('STOP\n')
                self._log.debug("%s: send 'STOP'" % host.get_name())
                host.receive_ok(sock)
            except socket.error, (errno, strerror):
                self._log.error("Cannot send conf_parameters: %s" % strerror)
            except CommandException, error:
                self._log.error("Cannot send conf_parameters: %s" % error)
            finally:
                if sock is not None:
                    sock.close()


    def get_env_plane_output(self):
        """ get all 'up' interfaces and check if their address mask
            corresponds to Platine hosts """
        # create the socket object to get the interface list
        sck = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # prepare the struct variable
        names = array.array('B', '\0' * 4096)

        # get the list from ioctl
        bytelen = struct.unpack('iL',
                                fcntl.ioctl(sck.fileno(),
                                            0x8912, # SIOCGIFCONF
                                            struct.pack('iL', 4096,
                                            names.buffer_info()[0])))[0]

        # convert it to string
        namestr = names.tostring()

        # return the interfaces as array
        ifaces = [namestr[i:i+32].split('\0', 1)[0]
                  for i in range(0, bytelen, 32)]

        # now get ip address of each interface and compare to hosts address
        for ifname in ifaces:
            # skip localhost
            if ifname == 'lo':
                continue

            addr = socket.inet_ntoa(fcntl.ioctl(sck.fileno(),
                                                0x8915,  # SIOCGIFADDR
                                                struct.pack('256s', ifname[:15])
                                                )[20:24])
            # check for a host with the same /24 mask
            for host in self._model.get_hosts_list():
                haddr = host.get_ip_address()
                haddr = haddr.rsplit(".", 1)[0]
                if addr.startswith(haddr):
                    return addr
            # check for a host with the same /16 mask
            for host in self._model.get_hosts_list():
                haddr = host.get_ip_address()
                haddr = haddr.rsplit(".", 2)[0]
                if addr.startswith(haddr):
                    return addr

        return None

    def run(self):
        """ main loop that manages the events on Platine Manager """
        if self._command is not None:
            self._command.start()
        while True:
            self._event_manager.wait(None)
            self._log.debug("event: " + self._event_manager.get_type())
            if self._event_manager.get_type() == 'deploy_platform':
                self.deploy_platform()
            elif self._event_manager.get_type() == 'start_platform':
                self.start_platform()
            elif self._event_manager.get_type() == 'stop_platform':
                self.stop_platform()
            elif self._event_manager.get_type() == 'quit':
                self.close()
                return
            else:
                self._log.warning("Controller: unknown event '" + \
                                  self._event_manager.get_type() + \
                                  "' received")
            self._event_manager.clear()

    def close(self):
        """ close the controller """
        self._log.debug("Controller: close")

        if self._server is not None:
            self._log.debug("Controller: close command server")
            self._server.stop()
        self._log.debug("Controller: close hosts")
        for host in self._hosts + self._ws:
            host.close()
        self._log.debug("Controller: hosts closed")

        self._log.debug("Controller: close environment plane")
        if self._env_plane is not None:
            self._env_plane.close()
        self._log.debug("Controller: environment plane closed")

        self._log.debug("Controller: closed")

    def deploy_platform(self):
        """ deploy Platine platform """
        # check that all component are stopped
        if self._model.is_running():
            self._log.warning("Some components are still running")

        self._log.info("Deploy Platine platform")

        try:
            self.update_deploy_config()
            for host in self._hosts + self._ws:
                self._log.info("Deploying " + host.get_name().upper())
                if not host.get_name().lower() in self._deploy_config.sections():
                    self._log.warning("No information for %s deployment, "
                                      "host will be disabled" % host.get_name())
                    host.disable()
                    continue
                host.deploy(self._deploy_config)
        except CommandException:
            self._log.error("Platine platform failed to deploy")
            self._event_manager_response.set('resp_deploy_platform', 'fail')
            return

        self._log.info("Platine platform deployed")

        # tell the GUI event manager that platine installation is over
        self._event_manager_response.set('resp_deploy_platform', 'done')

    def start_platform(self):
        """ start Platine platform """
        # check that we have at least env_plane, sat, gw and one st
        if not self._model.main_hosts_found():
            self._log.info("not enough component to start Platine: " \
                           "you will need at least a satellite, a gateway, " \
                           "a ST and the environment plane")
            self._event_manager_response.set('resp_start_platform', 'fail')
            return False

        # check if some components are still running (should not happen)
        if self._model.is_running():
            self._log.warning("Some components are still running")
            self._event_manager_response.set('resp_start_platform', 'fail')
            return False

        self._log.info("Start Platine platform")

        # get the environment plane controllers address
        # TODO rustine... à supprimer dès que possible !
        addr = self.get_env_plane_output()
        self._log.debug("find address %s for environment plane controllers" %
                        addr)
        content = self.update_com_parameters(addr)

        # create the base directory for configuration files
        # the configuration is shared between all runs in a scenario
        try:
            self.update_deploy_config()
            for host in self._hosts:
                name = host.get_name()
                component = name.lower()
                if component.startswith('st'):
                    component = 'st'
                # modify the com_parameters for environment agent
                # TODO suite de la rustine... à supprimer également dès que possible !
                self.send_com_parameters(host, content)

                self._log.info("Configuring " + host.get_name().upper())
                # create the host directory
                host_path = os.path.join(self._model.get_scenario(),
                                         host.get_name().lower())
                conf_file = os.path.join(host_path, 'core.conf')
                if not os.path.exists(conf_file):
                    if not self._model.is_default():
                        self._log.warning("The host %s did not exists when the "
                                          "current scenario was created. The "
                                          "default configuration will be used" %
                                          host.get_name())
                    if not os.path.isdir(host_path):
                        os.mkdir(host_path, 0755)
                    default_path = os.path.join(DEFAUL_PATH, component)
                    shutil.copy(os.path.join(default_path, 'core.conf'),
                                conf_file)
                #TODO try to simplify file deployment
                scenario = self._model.get_scenario()
                conf_files = [os.path.join(scenario, 'core_global.conf'),
                              os.path.join(scenario, 'topology.conf'),
                              conf_file]
                host.configure(conf_files,
                               1, 1, self._deploy_config,
                               self._model.get_dev_mode())
#TODO uncomment lines below and remove line above when the environment plane
#     will accept strings as scenario and run
#                               self._model.get_scenario(),
#                               self._model.get_run())
                # configure modules
                self.configure_modules(host)

            # configure tools on workstations
            for ws in self._ws:
                self._log.info("Configuring " + ws.get_name().upper())
                # create the WS directory
                ws_path = os.path.join(self._model.get_scenario(),
                                       ws.get_name().lower())
                if not os.path.isdir(ws_path):
                    os.mkdir(ws_path, 0755)
                ws.configure_ws(self._deploy_config, self._model.get_dev_mode())
        except (OSError, IOError), (errno, strerror):
            self._log.error("Failed to create directory '%s': %s" %
                            (host_path, strerror))
            self._event_manager_response.set('resp_start_platform', 'fail')
            return False
        except CommandException:
            self._log.error("Platine platform failed to configure")
            self._event_manager_response.set('resp_start_platform', 'fail')
            return False

        try:
            self._log.info("Starting Environment Plane")
            # set the ProbeController options
            config = self._model.get_conf()
            frame_duration = config.get_frame_duration()
            self._model.get_env_plane().set_options('probe',
                                                    '-f ' + frame_duration)
            self._env_plane.start()
            for host in self._hosts:
                self._log.info("Starting " + host.get_name().upper())
                host.start_stop('START')
        except CommandException:
            self._log.error("Platine platform failed to start")
            self._event_manager_response.set('resp_start_platform', 'fail')
            return False

        self._log.info("Platine platform started")

        # tell the GUI event manager that platine installation is over
        self._event_manager_response.set('resp_start_platform', 'done')
        return True

    def stop_platform(self):
        """ stop Platine platform """
        # check that at least one component is running
        if not self._model.is_running():
            self._log.warning("Platine platform is already stopped")

        self._log.info("Stop Platine platform")

        try:
            self._log.info("Stopping Environment Plane")
            self._env_plane.stop()
            for host in self._hosts:
                self._log.info("Stopping " + host.get_name().upper())
                host.start_stop('STOP')
        except CommandException:
            self._log.error("Platine platform failed to stop")
            self._event_manager_response.set('resp_stop_platform', 'fail')
            return False

        # save the environment plane results into the correct path
        # everything is saved in $HOME/.platine/scenario_1/run_1
        # TODO modify the environment plane to allow string as scenario and run
        if not 'HOME' in os.environ:
            self._log.error("cannot get $HOME environment variable, "
                            "could not save environment plane data")
        else:
            src = os.path.join(os.environ['HOME'],
                               ".platine/scenario_1/run_1")
            dst = os.path.join(self._model.get_scenario(),
                               self._model.get_run())
            try:
                copytree(src, dst)
            except Exception, msg:
                self._log.error("Cannot save environment plane data: %s" %
                                str(msg))

        self._log.info("Platine platform stopped")

        # tell the GUI event manager that platine installation is over
        self._event_manager_response.set('resp_stop_platform', 'done')
        return True

    def update_deploy_config(self):
        """ Update deployment configuration."""
        # used to deploy tests, this should have been done on startup
        ini_file = DEFAULT_INI_FILE
        if not 'HOME' in os.environ:
            self._log.warning("cannot get $HOME environment variable, "
                              "default deploy file will be used")
        else:
            ini_file = os.path.join(os.environ['HOME'],
                                    ".platine/deploy.ini")
            if not os.path.exists(ini_file):
                self._log.debug("cannot find file %s, " \
                                  "copy default" % ini_file)
                try:
                    shutil.copy(DEFAULT_INI_FILE,
                                ini_file)
                except IOError, msg:
                    self._log.warning("failed to copy %s configuration file "
                                      "in '%s': %s, default deploy file will "
                                      "be used"
                                      % (DEFAULT_INI_FILE, ini_file, msg))
                    ini_file = DEFAULT_INI_FILE

        try:
            self._deploy_config = ConfigParser.SafeConfigParser()
            if len(self._deploy_config.read(ini_file)) == 0:
                self._log.error("Cannot find file '%s'" % ini_file)
                raise CommandException
        except ConfigParser.Error, msg:
            self._log.error("Cannot find file '%s': %s" % (ini_file, msg))
            raise CommandException

    def start_server(self):
        """ start the command server """
        self._server = Plop(('0.0.0.0', CMD_PORT), CommandServer,
                            self)
        self._log.info("listening for command on port %d" % CMD_PORT)
        self._server.run()

    def configure_modules(self, host):
        """ send the module configuration on hosts """
        # connect to command server and send the configure command
        self._log.info("Deploy modules on %s" % host.get_name())
        try:
            sock = host.connect_command('CONFIGURE')
            if sock is None:
                return

            # send 'DATA' tag
            sock.send('DATA\n')
            self._log.debug("%s: send 'DATA'" % host.get_name())

            stream = Stream(sock, self._log)
            stream.send_dir(os.path.join(self._model.get_scenario(), 'plugins'),
                            '/etc/platine/plugins')

            # send 'DATA_END' tag
            sock.send(DATA_END)
            self._log.debug("%s: send '%s'" %
                            (host.get_name(), DATA_END.strip()))
            host.receive_ok(sock)
            # send 'STOP' tag
            sock.send('STOP\n')
            self._log.debug("%s: send 'STOP'" % host.get_name())

        except socket.error, (errno, strerror):
            self._log.error("Cannot contact %s command server: %s" %
                            (host.get_name(), strerror))
        except CommandException:
            self._log.error("cannot deploy plugins configuration")
            raise
        finally:
            if sock is not None:
                sock.close()




##### TEST #####
# TODO thread to run the main loop in order to find hosts
if __name__ == '__main__':
    from platine_manager_core.loggers.manager_log import ManagerLog
    from platine_manager_core.platine_model import Model
    import time
    import sys

    try:
        LOGGER = ManagerLog('debug', True, True, True)
        MODEL = Model(LOGGER)

        CONTROLLER = Controller(MODEL, '_platine._tcp', LOGGER, False)
        CONTROLLER.start()
        time.sleep(2)
        if not CONTROLLER.is_alive():
            LOGGER.error("controller failed to start")
            sys.exit(1)

        EVT_MGR = MODEL.get_event_manager()
        EVT_MGR.set('start_platform')
        EVT_MGR.set('stop_platform')
        EVT_MGR.set('quit')
    except:
        LOGGER.error("test failed")
        sys.exit(1)
        
    sys.exit(0)

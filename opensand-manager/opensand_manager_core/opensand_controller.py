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
opensand_controller.py - thread that configure, install, start, stop
                        and get status of all OpenSAND processes
"""

import threading
import os
import shutil
import ConfigParser
import time

from opensand_manager_core.utils import OPENSAND_PATH, ST, GW

from opensand_manager_core.my_exceptions import CommandException, ModelException
from opensand_manager_core.controller.service_listener import OpenSandServiceListener
from opensand_manager_core.controller.environment_plane import EnvironmentPlaneController
from opensand_manager_core.controller.tcp_server import Plop, CommandServer

DEFAULT_INI_FILE = OPENSAND_PATH + "deploy.ini"
CMD_PORT = 5656
DATA_END = 'DATA_END\n'

class Controller(threading.Thread):
    """ controller that controll all hosts """
    def __init__ (self, model, service_type, manager_log, interactive=False):
        try:
            threading.Thread.__init__(self)
            self.setName("Controller")
            self._model = model
            self._log = manager_log
            self._event_manager = self._model.get_event_manager()
            self._event_manager_response = self._model.get_event_manager_response()
            self._env_plane = EnvironmentPlaneController(model, manager_log)
            self._hosts = []
            self._ws = []
            self._server = None
            self._command = None

            # The configuration of deployment
            self._deploy_config = None

            if not 'HOME' in os.environ:
                self._log.warning("cannot get $HOME environment variable, "
                                  "default deploy file will be used")
            else:
                ini_file = os.path.join(os.environ['HOME'],
                                        ".opensand/deploy.ini")
                if not os.path.exists(ini_file):
                    self._log.debug("cannot find file %s, " \
                                    "copy default" % ini_file)
                    try:
                        shutil.copy(DEFAULT_INI_FILE,
                                    ini_file)
                    except IOError, msg:
                        self._log.warning("failed to copy %s configuration file"
                                          " in '%s': %s, default deploy file "
                                          "will be used"
                                          % (DEFAULT_INI_FILE, ini_file, msg))

            # create the service browser here because we need hosts as argument
            # but it will be started with gtk main loop
            OpenSandServiceListener(self._model, self._hosts, self._ws,
                                    self._env_plane, service_type, self._log)
  
            if interactive:
                self._command = threading.Thread(None, self.start_server,
                                                 "CommandServer",
                                                 (), {})
        except Exception:
            self.close()
            raise

    def run(self):
        """ main loop that manages the events on OpenSAND Manager """
        if self._command is not None:
            self._command.start()
        while True:
            self._event_manager.wait(None)
            self._log.debug("event: " + self._event_manager.get_type())
            res = 'fail'
            if self._event_manager.get_type() == 'set_scenario':
                self._model.load()
                res = 'done'
                self._event_manager_response.set('resp_set_scenario', res)
            elif self._event_manager.get_type() == 'update_config':
                self._model.update_config()
                res = 'done'
                self._event_manager_response.set('resp_update_config', res)
            elif self._event_manager.get_type() == 'deploy_platform':
                if self.deploy_platform():
                    res = 'done'
                self._event_manager_response.set('resp_deploy_platform', res)
            elif self._event_manager.get_type() == 'deploy_files':
                self._event_manager_response.set('deploy_files')
                if self.deploy_files():
                    res = 'done'
                self._event_manager_response.set('resp_deploy_files', res)
            elif self._event_manager.get_type() == 'start_platform':
                if self.start_platform():
                    res = 'done'
                self._event_manager_response.set('resp_start_platform', res)
            elif self._event_manager.get_type() == 'stop_platform':
                if self.stop_platform():
                    res = 'done'
                self._event_manager_response.set('resp_stop_platform', res)
            elif self._event_manager.get_type() == 'quit':
                self.close()
                return
            else:
                self._log.warning("Controller: unknown event '" + \
                                  self._event_manager.get_type() + \
                                  "' received")
            self._event_manager.clear()
            self._log.debug("event %s cleared" % (self._event_manager.get_type()))

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
            self._env_plane.cleanup()
        self._log.debug("Controller: environment plane closed")
        if self._server is not None:
            try:
                self._server.shutdown()
            except:
                pass

        self._log.debug("Controller: closed")

    def deploy_platform(self):
        """ deploy OpenSAND platform """
        # check that all component are stopped
        if self._model.all_running():
            self._log.warning("All components are running, do not deploy")
            return False
        if self._model.is_running():
            self._log.warning("Some components are still running  while "
                              "trying to deploy: %s" %
                              self._model.running_list())

        self._log.info("Deploying OpenSAND platform...")

        threads = []
        errors = []
        try:
            self.update_deploy_config()
            for host in self._hosts + self._ws:
                host_name = host.get_name().lower()
                self._log.debug("Deploying " + host.get_name().upper())
                if not host_name  in self._deploy_config.sections():
                    component = host_name
                    if component.startswith(ST):
                        component = ST
                    if component.startswith(GW):
                        component = GW
                    host_name = component

                    if not host_name in self._deploy_config.sections():
                        self._log.warning("No information for %s deployment, "
                                      "host will be disabled" % host.get_name())
                        host.disable()
                        continue
                for machine in host.get_ordered_machines():
                    thread = threading.Thread(None, machine.deploy, "Deploy%s" %
                                              host_name,
                                              (self._deploy_config, errors), {})
                    thread.start()
                    threads.append(thread)
        except CommandException:
            self._log.error("OpenSAND platform failed to deploy")
            return False
        finally:
            for thread in threads:
                thread.join()

        if len(errors) > 0:
            self._log.error("OpenSAND platform failed to deploy")
            return False

        self._log.info("OpenSAND platform deployed")

        return True

    def deploy_files(self):
        """ send the  files on host """
        if self._model.all_running():
            self._log.warning("All components are running, do not deploy files")
            return False
        # check that all component are stopped
        if self._model.is_running():
            self._log.warning("Some components are still running  while "
                              "deploying files: %s" %
                              self._model.running_list())

        threads = []
        errors = []
        try:
            # first get global configuration files
            files = self._model.get_deploy_files()
            all_files = self._model.get_all_files()
            self._log.debug("Deploy files from scenario %s" %
                            self._model.get_scenario())
            for host in self._hosts + self._ws:
                name = host.get_name()
                # do a copy, not only reference
                dep = list(files)
                if host.first_deploy():
                    dep = all_files

                dep += host.get_deploy_files()
                if len(dep) > 0:
                    self._log.info("%s: deploy simulation files" % name)
                else:
                    continue

                for machine in host.get_ordered_machines():
                    thread = threading.Thread(None, machine.deploy_modified_files,
                                              "DeployFiles%s" % name,
                                              (dep, errors), {})
                    threads.append(thread)
                    thread.start()
        except CommandException, msg:
            self._log.error("Simulation files installation failed")
            return False
        finally:
            # join threads
            for thread in threads:
                thread.join()

        if len(errors) > 0:
            self._log.error("Simulation files installation failed")
            return False

        self._model.set_deployed()

        return True

    def start_platform(self):
        """ start OpenSAND platform """
        # check that we have at least env_plane, sat, gw and one st
        if not self._model.main_hosts_found():
            self._log.info("not enough component to start OpenSAND: " \
                           "you will need at least a satellite, a gateway " \
                           "and a ST")
            return False

        # check if some components are still running (should not happen)
        if self._model.is_running():
            self._log.warning("Some components are still running  while "
                              "trying to start: %s" %
                              self._model.running_list())
            return False

        self._log.info("Start OpenSAND platform")

        # create the base directory for configuration files
        # the configuration is shared between all runs in a scenario
        threads = []
        errors = []
        try:
            self.update_deploy_config()
            self._log.info("Configuring hosts...")
            for host in self._hosts:
                name = host.get_name()
                component = name.lower()
                if component.startswith(ST):
                    component = ST
                if component.startswith(GW):
                    component = GW

                
                self._log.debug("Configuring " + host.get_name().upper())
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
                    default_path = os.path.join(OPENSAND_PATH, component)
                    shutil.copy(os.path.join(default_path, 'core.conf'),
                                conf_file)
                scenario = self._model.get_scenario()
                # the list of files to send
                conf_files = [os.path.join(scenario, 'core_global.conf'),
                              os.path.join(scenario, 'topology.conf'),
                              conf_file]
                # the list of modules configuration files to send
                # first get host modules
                modules_dir = os.path.join(host_path, 'plugins')
                modules = []
                if os.path.isdir(modules_dir):
                    modules = [modules_dir, # host specific modules
                               os.path.join(scenario, 'plugins')] # global modules

                for machine in host.get_ordered_machines():
                    thread = threading.Thread(None, machine.configure, "Configure%s" %
                                              name,
                                              (conf_files, modules,
                                               self._deploy_config,
                                               self._model.get_dev_mode(), errors),
                                              {})
                    threads.append(thread)
                    thread.start()
                    time.sleep(0.5)

            # configure tools on workstations
            if len(self._ws) > 0:
                self._log.info("Configuring ws...")
            for ws in self._ws:
                self._log.debug("Configuring " + ws.get_name().upper())
                # create the WS directory
                ws_path = os.path.join(self._model.get_scenario(),
                                       ws.get_name().lower())
                if not os.path.isdir(ws_path):
                    os.mkdir(ws_path, 0755)
                for machine in ws.get_ordered_machines():
                    thread = threading.Thread(None, machine.configure_ws, "Configure%s" %
                                              ws.get_name(),
                                              (self._deploy_config,
                                               self._model.get_dev_mode(), errors),
                                             {})
                    threads.append(thread)
                    thread.start()

        except (OSError, IOError), error:
            self._log.error("Failed to handle configuration %s" %
                            (error))
            return False
        except CommandException:
            self._log.error("OpenSAND platform failed to configure")
            return False
        finally:
            # join threads
            for thread in threads:
                thread.join()

        if len(errors) > 0:
            self._log.error("OpenSAND platform failed to configure")
            return False

        self._log.info("Platform configured")

        try:
            for host in self._hosts + self._ws:
                self._log.info("Starting " + host.get_name().upper())
                host.start_stop('START %s' % host.get_interface_type())
        except (ModelException, CommandException), err:
            msg = "OpenSAND platform failed to start"
            if self._model.get_dev_mode():
                msg += ", deploy OpenSAND before starting it"
            self._log.error(msg)
            return False

        self._log.info("OpenSAND platform started")

        return True

    def stop_platform(self):
        """ stop OpenSAND platform """
        # check that at least one component is running
        if not self._model.is_running():
            self._log.info("OpenSAND platform is already stopped")

        self._log.info("Stop OpenSAND platform")

        for host in self._hosts + self._ws:
            try:
                self._log.info("Stopping " + host.get_name().upper())
                host.start_stop('STOP')
            except CommandException:
                self._log.error("Failure when trying to stop %s" %
                                host.get_name().upper())

        # save the environment plane results into the correct path
        # everything is saved in scenario/run
        if self._model.is_collector_functional():
            self._log.info("Save the environment plane outputs")
            # TODO this is not a response to a View event, maybe just rename
            #      event manager reponse into view_event_manager and
            #      event_manager into controller_event_manager
            self._event_manager_response.set('probe_transfer')
            dst = os.path.join(self._model.get_scenario(),
                               self._model.get_run())
            
            def done(status='done'):
                self._event_manager_response.set('resp_probe_transfer',
                                                 status)
            
            self._env_plane.transfer_from_collector(dst, done)

        self._log.info("OpenSAND platform stopped")

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
                                    ".opensand/deploy.ini")
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
                            self, self._model, self._log)
        self._log.info("listening for command on port %d" % CMD_PORT)
        self._server.run()
        self._server = None


    def get_env_plane_controller(self):
        """ return the environment plane controller """
        return self._env_plane


##### TEST #####
# TODO thread to run the main loop in order to find hosts
if __name__ == '__main__':
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model
    import time
    import sys

    try:
        LOGGER = ManagerLog('debug', True, True, True)
        MODEL = Model(LOGGER)

        CONTROLLER = Controller(MODEL, '_opensand._tcp', LOGGER, False)
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

#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 CNES
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
opensand_shell_manager.py - OpenSAND manager utils for shell
"""


import threading
import time
import gobject

from opensand_manager_core.opensand_model import Model
from opensand_manager_core.model.event_manager import EventManager
from opensand_manager_core.opensand_controller import Controller
from opensand_manager_core.controller.host import HostController
from opensand_manager_core.loggers.manager_log import ManagerLog
from opensand_manager_core.loggers.levels import MGR_WARNING
from opensand_manager_core.controller.tcp_server import Plop, CommandServer


INIT_ITER=50
SERVICE = "_opensand._tcp"
EVT_TIMEOUT = 10


class ShellManager(object):
    """
    OpenSAND manager shell interface
    """
    def __init__(self):
        self._model = None
        self._controller = None

        self._disable_remote_logs = None

        # the workstations controllers
        self._ws_ctrl = []

        self._loop = None

        self._event_response_handler = None
        self._event_resp = None

        self._command_server = None
        self._command = None

    def load(self, log_level=MGR_WARNING, service=SERVICE,
                 remote_logs=False, with_ws=False,
                 command_server=(False, 5656)):
        """
        load the shell manager interface:
            log_level:      the level of logs that will be printed in shell
            service:        the Avahi service to listen for hosts
            remote_logs:    whether remote hosts will send their logs to the
                            collector
            with_ws:        create controllers for workstations
            daemonize:      run as a daemon and start a command server
            command_server: a tuple with first element to indicate if we start
                            the command server and the second element containing
                            the port to listen
                            If you don't want to be stuck listening, do not
                            enable command server here and call the
                            launch_command_server function
        """ 
        self._disable_remote_logs = remote_logs
        self._log = ManagerLog(log_level, True, False, False, 'ShellManager')
        # create the OpenSAND model
        self._model = Model(self._log)
        # create the OpenSAND controller
        self._controller = Controller(self._model, service, self._log,
                                      False)
        self._controller.start()
        self._event_response_shell = EventManager("ShellEventManager")
        self._event_response_handler = \
            EventResponseHandler(self._model.get_event_manager_response(),
                                 self._event_response_shell,
                                 self._model,
                                 self._log)
        self._event_response_handler.start()
        # Launch the mainloop for service_listener
        self._loop = Loop()
        self._loop.start()
        self._log.info(" * Wait for Model and Controller initialization")
        count = 0
        while count < INIT_ITER and \
              (not self._model.main_hosts_found() or
               not self._model.is_collector_functional()):
            count += 1
            time.sleep(0.1)
        if not self._model.main_hosts_found():
            self._log.error(" * Main hosts not found, quit")
            raise ShellMgrException("Main hosts not found")
        self._log.info(" * Main hosts found, platform is initialized")
        if not self._model.is_collector_functional():
            self._log.warning(" * The collector is not found at the moment. "
                              "Consider starting it to get logs and statistics")
        if with_ws:
            self.create_ws_controllers()
        CommandServer._shutdown = self.close
        if command_server[0]:
            self._start_command_server(command_server[1])


    def close(self):
        """ stop the service listener """
        self._log.info(" * Closing...")
        self._log.debug(" * Close workstation controllers")
        for ws_ctrl in self._ws_ctrl:
            ws_ctrl.close()
        self._log.debug(" * Workstation controllers closed")
        if self._model is not None:
            self._log.debug(" * Close model")
            self._model.close()
        if self._event_response_handler is not None and \
           self._event_response_handler.is_alive():
            self._log.debug(" * Join event reponse handler")
            self._event_response_handler.join()
        if self._loop is not None:
            self._log.debug(" * Close mainloop")
            self._loop.close()
            self._loop.join()
        if self._command_server is not None:
            # kill the command server in a thread because this won't stop if
            # server try to kill itself
            stop = threading.Thread(None, self._command_server.stop,
                                    "StopCommandServer", (), {})
            stop.start()
        if self._controller is not None:
            self._log.debug(" * Close controller")
            self._controller.join()
        if self._command is not None:
            self._log.debug(" * Close command server")
            self._command.join()
            
    def launch_command_server(self, port):
        """ launch a threaded command server """
        if self._command_server is not None:
            self._log.error("Command server is already running")

        self._command = threading.Thread(None, self._start_command_server,
                                         "CommandServer", (port), {})
        self._command.start()


    def _start_command_server(self, port):
        """ start the command server """
        self._command_server = Plop(('0.0.0.0', port), CommandServer,
                               self._controller, self._model, self._log)
        self._log.info("listening for command on port %d" % port)
        self._command_server.run()
        

    def save_scenario(self, path):
        """ save the current scenario """
        self._model.set_scenario(path)

    def default_scenario(self):
        """ get to the default sceanrio """
        self._model.set_scenario("")
        self._model.set_run("")

    def stop_opensand(self):
        """ stop the OpenSAND testbed """
        evt = self._model.get_event_manager()
        evt.set('stop_platform')
        if not self._event_response_shell.wait(EVT_TIMEOUT):
            raise ShellMgrException("Timeout when stopping platform")
        if self._event_response_shell.get_type() != "stopped":
            self._event_response_shell.clear()
            raise ShellMgrException("wrong event response %s when stopping "
                                    "platform" %
                                    self._event_response_shell.get_type())
        self._log.info(" * %s event received" %
                       self._event_response_shell.get_type())
        if self._event_response_shell.get_text() != 'done':
            self._event_response_shell.clear()
            raise ShellMgrException("cannot stop platform")
        self._event_response_shell.clear()
        time.sleep(1)

    def start_opensand(self):
        """ start the OpenSAND testbed """
        evt = self._model.get_event_manager()
        evt.set('start_platform')
        if not self._event_response_shell.wait(EVT_TIMEOUT):
            raise ShellMgrException("timeout when starting platform")
        if self._event_response_shell.get_type() != "started":
            self._event_response_shell.clear()
            raise ShellMgrException("wrong event response %s when starting "
                                    "platform" %
                                    self._event_response_shell.get_type())
        self._log.info(" * %s event received" %
                       self._event_response_shell.get_type())
        if self._event_response_shell.get_text() != 'done':
            self._event_response_shell.clear()
            raise ShellMgrException("cannot start platform")
        self._event_response_shell.clear()
        time.sleep(1)

        if self._disable_remote_logs:
            # disable remote logs this reduce consumption
            env_plane_ctrl = self._controller.get_env_plane_controller()
            for program in env_plane_ctrl.get_programs():
                env_plane_ctrl.enable_logs(False, program)

    def create_ws_controllers(self):
        """ create controllers for WS because the manager
            controller does not handle it """
        for ws_model in self._model.get_workstations_list():
            new_ws = HostController(ws_model, self._log, None)
            self._ws_ctrl.append(new_ws)



class Loop(threading.Thread):
    """ the mainloop for service_listener """
    _main_loop = None
    def run(self):
        """ run the loop """
        try:
            gobject.threads_init()
            Loop._main_loop = gobject.MainLoop()
            Loop._main_loop.run()
        except KeyboardInterrupt:
            self.close()
            raise

    def close(self):
        """ stop the loop """
        Loop._main_loop.quit()
        

class EventResponseHandler(threading.Thread):
    """
    Get response events from hosts controllers
    We need this intermediate handler to eliminate parasite events
    """
    def __init__(self, event_manager_response,
                       event_response_tests, model,
                 log):
        threading.Thread.__init__(self)
        self._evt_resp = event_manager_response
        self._evt_tests = event_response_tests
        self._model = model
        self._log = log
        
    def run(self):
        """ main loop that manages the events responses """
        while True:
            self._evt_resp.wait(None)
            event_type = self._evt_resp.get_type()

            self._log.info(" * event response: " + event_type)

            if event_type == "resp_deploy_platform":
                pass

            elif event_type == "deploy_files":
                self._log.debug(" * deploying files")

            elif event_type == "resp_deploy_files":
                self._log.debug(" * files deployed")

            elif event_type == "resp_start_platform":
                count = 0
                val = str(self._evt_resp.get_text())
                while not self._model.all_running() and count < 10:
                    time.sleep(0.5)
                    count += 1
                if count >= 10:
                    val = "fail"
                self._evt_tests.set('started', val)

            elif event_type == "resp_stop_platform":
                count = 0
                while self._model.is_running() and count < 10:
                    time.sleep(0.5)
                    count += 1
                if count >= 10:
                    val = "fail"
                self._evt_tests.set('stopped',
                                    str(self._evt_resp.get_text()))
            
            elif event_type == "probe_transfer":
                self._log.debug(" * transfering probes")

            elif event_type == "resp_probe_transfer":
                self._log.debug(" * probes transfered")

            elif event_type == 'quit':
                # quit event
                self.close()
                return
            else:
                self._log.warning(" * Response Event Handler: unknown event " \
                                  "'%s' received" % event_type)

            self._evt_resp.clear()

    def close(self):
        """ close the event response handler """
        self._log.debug(" * Response Event Handler: closed")


class ShellMgrException(Exception):
    """ error during tests """
    def __init__(self, descr):
        Exception.__init__(self)
        self.msg = descr

    def __repr__(self):
        return self.msg

    def __str__(self):
        return repr(self)



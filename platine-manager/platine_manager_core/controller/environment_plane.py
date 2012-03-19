#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
environment_plane.py - controller that configure, install, start, stop
                       and get status of Environment Plane processes
"""

import threading
import signal

from platine_manager_core.my_exceptions import CommandException
from platine_manager_core.controller.environment_plane_process import EnvironmentPlaneProcess

class EnvironmentPlaneController():
    """ controller which implements the client that connect in order to get
        environment plane state and that commands the environment plane """
    def __init__(self, env_plane_model, manager_log):
        self._env_plane_model = env_plane_model
        self._log = manager_log
        self._started_list = [] # the list of started components
        self._process_list = EnvironmentPlaneProcess(env_plane_model,
                                                     manager_log)
        self._stop = threading.Event()
        self._state_handler = threading.Thread(None, self.refresh_state,
                                               None, (), {})

        signal.signal(signal.SIGALRM, self.sig_handler)
        self._state_handler.start()

    def sig_handler(self, signum, frame):
        """ handle SIGALRM because environment plane send it on error """
        self._log.warning("Signal %s received, stop environment plane" %
                          str(signum))
        self.stop()

    def close(self):
        """ close the host connections """
        self._log.debug("Environment plane: close")
        self._log.debug("Environment plane: join state client")
        self._stop.set()
        self._state_handler.join()
        self._log.debug("Environment plane: state client joined")
        self._log.debug("Environment plane: closed")

    def start(self):
        """ start the environment plane """
        if not self._process_list.is_initialized():
            error = "The Environment Plane is broken, please fix the " + \
                    "problem and restart the Manager"
            self._log.error(error)
            raise CommandException(error)

        if self._process_list.is_running():
            self._log.error("Some process are already started")
            raise CommandException("Some process are already started")

        try:
            self._process_list.start()
        except Exception:
            raise

    def stop(self):
        """ stop the environment plane """
        if not self._process_list.is_initialized():
            error = "The Environment Plane is broken, please fix the " + \
                    "problem and restart the Manager"
            self._log.error(error)
            raise CommandException(error)

        try:
            self._process_list.stop()
        except Exception:
            raise

    def refresh_state(self):
        """ update process list """
        try:
            self._process_list.load()
        except Exception:
            self._log.warning("Failed to initialize the Environment Plane")

        self.update_state(True)
        while not self._stop.isSet():
            # check program state to detect crashes
            self._process_list.update(True)
            self.update_state()
            self._stop.wait(1.0)

    def update_state(self, first = False):
        """ send the status of each component for
            which the state has changed """
        new_compo_list = self._process_list.get_components()

        self._started_list.sort()
        new_compo_list.sort()
        if first or self._started_list != new_compo_list:
            # update the component list
            self._started_list = new_compo_list
            self._log.debug("component list has changed: " +
                            str(self._started_list))
            self._env_plane_model.set_started(self._started_list)
            try:
                self._process_list.serialize()
            except Exception:
                self._log.error("error when serializing process list")


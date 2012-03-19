#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
environment_plane.py - Model for Platine environment plane controllers
"""

import threading

class EnvironmentPlaneModel:
    """ Model for environement plane controllers """
    def __init__(self, manager_log):
        #  key       process name        state options
        self._env_plane_controllers = {
           'error' : ['ErrorController', None, "-d"], #"-d -t0"],
           'probe' : ['ProbeController', None, "-f"], #"-f -t0"],
           'event' : ['EventController', None, "-d"]  #"-d -t0"]
        }

        self._log = manager_log

        self._lock = threading.Lock()

    def _set_(self, key, index, value):
        """ set the value at desired index for controller specified by key """
        self._lock.acquire()
        self._env_plane_controllers[key][index] = value
        self._lock.release()

    def _get_(self, key, index):
        """get the value at desired index for controller specified by key """
        self._lock.acquire()
        val = self._env_plane_controllers[key][index]
        self._lock.release()
        return val

    def get_process(self, key):
        """ get a process name """
        return self._get_(key, 0)

    def get_state(self, key):
        """ get a process state """
        return self._get_(key, 1)

    def set_started(self, started_list):
        """ set specified states to True """
        val = False
        if started_list == None:
            val = None

        for key in self._env_plane_controllers.keys():
            self._set_(key, 1, val)
        if started_list is not None:
            for name in started_list:
                find = False
                for key in self._env_plane_controllers.keys():
                    if name == self.get_process(key):
                        self._set_(key, 1, True)
                        find = True
                        break
                if not find:
                    self._log.warning("Environment Plane: component '" +
                                      key + "' does not belong to model")

    def set_state(self, key, state):
        """ set a process state """
        self._set_(key, 1, state)

    def get_options(self, key):
        """ get the process options """
        return self._get_(key, 2)

    def set_options(self, key, value):
        """ set the process options values """
        self._set_(key, 2, value)

    def get_list(self):
        """ get the process list """
        self._lock.acquire()
        process_list = self._env_plane_controllers.keys()
        self._lock.release()
        return process_list

    def get_states(self):
        """ get all the process states """
        ret = []
        for item in self._env_plane_controllers :
            ret.append([item, self.get_state(item)])
        return ret

    def is_running(self):
        """ check if at least one process is running """
        for item in self._env_plane_controllers:
            if self.get_state(item):
                return True
        return False

    def get_process_name(self, key):
        """ return the process name """
        return self._get_(key, 0)


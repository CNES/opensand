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
environment_plane_process.py - handle the environment plane process
"""

import threading
import subprocess
import shlex
import pickle
import os
import signal

from platine_manager_core.my_exceptions import ModelException

class EnvironmentPlaneProcess():
    """ handle the process list """
    def __init__(self, env_plane_model, manager_log):
        self._process_lock = threading.Lock()
        self._process_list = {}
        self._env_plane_model = env_plane_model
        self._init = False
        self._log = manager_log
        self._processfile = ''

    def load(self):
        """ initialize the process list """
        self._process_lock.acquire()
        # read the process file
        if not 'HOME' in os.environ:
            raise ModelException("cannot get HOME environment variable")

        # TODO faire un fichier commun à tous pour éviter qu'un autre utilisateur
        #      voie les process éteints alors qu'ils ont été lancés par quelqu'un d'autre !
        self._processfile = os.environ['HOME'] + "/.platine/env_plane_process"

        try:
            process_file = open(self._processfile, 'rb')
        except IOError, (errno, strerror):
            self._log.debug("unable to read the file '%s' (%d: %s). " \
                            "keep an empty process list" %
                            (self._processfile, errno, strerror))
            self._init = True
            self._log.debug("process list is initialized")
        else:
            try:
                self._process_list = pickle.load(process_file)
            except EOFError:
                self._log.info("process file is empty")
                self._process_list = {}
                self._init = True
                self._log.debug("process list is initialized")
            except pickle.PickleError, error:
                self._log.error("unable to load the process list: " +
                                str(error))
                raise
            else:
                self._init = True
                self._log.debug("process list is initialized")
            finally:
                process_file.close()
        finally:
            self._process_lock.release()
            self.update(True)

    def is_initialized(self):
        """ check if the process list is initialized """
        return self._init

    def start(self):
        """ load the binary configuration file and start programs """
        self._process_lock.acquire()

        for key in self._env_plane_model.get_list():
            name = self._env_plane_model.get_process_name(key)
            options = self._env_plane_model.get_options(key)
            cmd_line = "/usr/bin/%s %s" % (name, options)
            self._log.info("Start " + name)
            self._log.debug("command line: " + cmd_line)
            cmd = shlex.split(cmd_line)
            self._log.debug("command line split: " + str(cmd))
            with open("/dev/null") as null:
                process = subprocess.Popen(cmd, stdout=null,
                                           stderr=null, close_fds=True)
            self._process_list[name] = process

        self._process_lock.release()

        try:
            self.serialize()
        except Exception:
            raise

    def serialize(self):
        """ serialize the new process list in order to keep it in case
            of daemon restart """
        self._process_lock.acquire()
        try:
            process_file = open(self._processfile, 'wb')
            pickle.dump(self._process_list, process_file)
        except IOError, (errno, strerror):
            self._log.error("unable to create %s file (%d: %s)" %
                            (self._processfile, errno, strerror))
            raise
        except pickle.PickleError, error:
            self._log.error("unable to serialize process list: " + str(error))
            raise
        else:
            process_file.close()
        finally:
            self._process_lock.release()

    def stop(self):
        """ stop all the process """
        # check if all binaries are correctly stopped
        if len(self._process_list) == 0:
            self._log.warning("all process are already stopped")
            return

        self._process_lock.acquire()

        for name in self._process_list.keys():
            process = self._process_list[name]
            self._log.info("kill %s process" % name)
            try:
                process.terminate()
                process.wait()
            except OSError, (errno, strerror):
                # No child processes error reported when stopping while
                # manager has been restarted => ignore it
                if errno != 10:
                    self._log.warning("Error when terminating %s: %s" %
                                     (name, strerror))

        self._process_list = {}
        self._process_lock.release()

        try:
            self.serialize()
        except Exception:
            pass

    def update(self, check = False):
        """ update the list of started components """
        self._process_lock.acquire()

        for name in self._process_list.keys():
            running = True
            process = self._process_list[name]
            try:
                os.kill(process.pid, signal.SIGCONT)
            except OSError:
                # process does not exist anymore
                # (other error may occurred so be careful)
                running = False
            else:
                if check:
                    proc = subprocess.Popen("ps -edf | \grep " + \
                                            str(process.pid),
                                            shell = True,
                                            stdout = subprocess.PIPE)
                    ret = proc.stdout.read()
                    proc.stdout.close()
                    proc.wait()

                    if ret.find(name) < 0:
                        running = False

            if not running:
                del self._process_list[name]
                self._log.info("Assume that process %s is stopped" % name)

        self._process_lock.release()

    def get_components(self):
        """ return the components of the process list """
        return self._process_list.keys()


    def is_running(self):
        """ check if some components are running """
        if len(self._process_list) > 0:
            return True
        else:
            return False

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
process_list.py - handle a list of processus
"""

import threading
import subprocess
import pickle
import logging
import ConfigParser
import shlex
import os
import signal

#macros
LOGGER = logging.getLogger('sand-daemon')
PROCESS_FILE = "/var/cache/sand-daemon/process"
START_INI = "/var/cache/sand-daemon/start.ini"


class ProcessList():
    """ handle the process list """

    # the ProcessList class attributes are shared between command and state
    # threads
    _process_lock = threading.Lock()
    _stop = threading.Event()
    _process_list = {}
    _init = False
    _wait = None
    _callbacks = []

    def __init__(self):
        pass

    def reset(self):
        ProcessList._process_lock.acquire()
        ProcessList._process_list = {}
        ProcessList._stop.set()
        if ProcessList._wait is not None:
            ProcessList._wait.join()
        ProcessList._stop.clear()
        ProcessList._init = False
        ProcessList._process_lock.release()

    def load(self):
        """ initialize the process list """
        ProcessList._process_lock.acquire()
        # read the process file
        try:
            process_file = open(PROCESS_FILE, 'rb')
        except IOError, (errno, strerror):
            LOGGER.debug("unable to read the file '%s' (%d: %s). "
                         "Keep an empty process list" %
                         (PROCESS_FILE, errno, strerror))
            ProcessList._init = True
            LOGGER.debug("process list is initialized")
        else:
            try:
                ProcessList._process_list = pickle.load(process_file)
            except EOFError:
                LOGGER.info("process file is empty")
                ProcessList._process_list = {}
                ProcessList._init = True
                LOGGER.debug("process list is initialized")
            except pickle.PickleError, error:
                LOGGER.error("unable to load the process list: " + str(error))
                raise
            else:
                ProcessList._init = True
                LOGGER.debug("process list is initialized")
            finally:
                process_file.close()
        finally:
            ProcessList._process_lock.release()
            self.update(True)

    def is_initialized(self):
        """ check if the process list is initialized """
        return ProcessList._init

    def start(self):
        """ load the binary configuration file and start programs """
        parser = ConfigParser.SafeConfigParser()
        if len(parser.read(START_INI)) == 0:
            LOGGER.error("unable to read %s file", START_INI)
            raise IOError

        ProcessList._process_lock.acquire()

        sections = parser.sections()
        ProcessList._process_list = {}
        for section in sections:
            try:
                # warning first argument of option should be the program
                cmd_line = parser.get(section, "command")
                LOGGER.info("start " + section)
                LOGGER.debug("command line: " + str(cmd_line))
                cmd = shlex.split(cmd_line)
                LOGGER.debug("command line splitted: " +
                                     str(cmd))
                if parser.has_option(section, 'ld_library_path'):
                    ld_library_path = parser.get(section, 'ld_library_path')
                    os.environ['LD_LIBRARY_PATH'] = ld_library_path
                    LOGGER.info('Library path: %s' % ld_library_path) 
                process = subprocess.Popen(cmd, close_fds=True)
                process.prog_name = section
                if 'LD_LIBRARY_PATH' in os.environ:
                    del os.environ['LD_LIBRARY_PATH']
                ProcessList._process_list[section] = process
                # wait until process is stopped to avoid zombie
                # process if it crashes or returns
                ProcessList._wait = threading.Thread(None, self.wait_process,
                                                     None, (process,), {})
                ProcessList._wait.start()
            except ConfigParser.Error, error:
                LOGGER.error("error when reading binaries configuration " + \
                             "(%s) stop all process", str(error))
                ProcessList._process_lock.release()
                self.stop()
                raise
            except OSError:
                ProcessList._process_lock.release()
                self.stop()
                raise

        ProcessList._process_lock.release()

        try:
            self.serialize()
        except Exception:
            raise

    def serialize(self):
        """ serialize the new process list in order to keep it in case
            of daemon restart """
        ProcessList._process_lock.acquire()
        try:
            process_file = open(PROCESS_FILE, 'wb')
            pickle.dump(ProcessList._process_list, process_file)
        except IOError, (errno, strerror):
            LOGGER.error("unable to create %s file (%d: %s)" % (PROCESS_FILE,
                          errno, strerror))
            raise
        except pickle.PickleError, error:
            LOGGER.error("unable to serialize process list: " + str(error))
            process_file.close()
            raise
        else:
            process_file.close()
        finally:
            ProcessList._process_lock.release()

    def stop(self):
        """ stop all the process """
        # check if all binaries are already stopped
        if len(ProcessList._process_list) == 0:
            LOGGER.info("all process are already stopped")
            return

        ProcessList._process_lock.acquire()

        for name in ProcessList._process_list.keys():
            process = ProcessList._process_list[name]
            LOGGER.info("terminate %s process", name)

            # kill process if terminate fails else we block on wait()
            kill = threading.Thread(None, self.check_terminate,
                                    None, (process,), {})
            kill.start()
            try:
                process.terminate()
                process.wait()
            except OSError, (errno, strerror):
                # No child processes error reported when stopping while
                # manager has been restarted => ignore it
                if errno != 10: 
                    LOGGER.warning("Error when terminating %s: %s" %
                                   (name, strerror))
                    pass

            for callback in ProcessList._callbacks:
                callback(process)

        ProcessList._stop.set()
        if ProcessList._wait is not None:
            ProcessList._wait.join()
        if kill is not None:
            kill.join()
        ProcessList._stop.clear()

        ProcessList._process_list = {}
        ProcessList._process_lock.release()

        try:
            self.serialize()
        except:
            pass

    def update(self, check = False):
        """ update the list of started components """
        ProcessList._process_lock.acquire()

        for name in ProcessList._process_list.keys():
            running = True
            process = ProcessList._process_list[name]
            try:
                os.kill(process.pid, signal.SIGCONT)
            except OSError:
                # process does not exist anymore 
                # (other error may occured so be careful)
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
                del ProcessList._process_list[name]
                for callback in ProcessList._callbacks:
                    callback(process)
                LOGGER.info("assume that process %s is stopped", name)
#            else:
#                LOGGER.debug("process '%s' is running", name)

        ProcessList._process_lock.release()

    def find_process(self, attr, value):
        """ find the process whose attribute attr equals value """

        with ProcessList._process_lock:
            for process in ProcessList._process_list.itervalues():
                if getattr(process, attr, None) == value:
                    return process

        return None

    def get_processes_attr(self, attr):
        """ return a list of the processes’ specified attribute """

        result = []
        with ProcessList._process_lock:
            for process in ProcessList._process_list.itervalues():
                value = getattr(process, attr, None)
                if value is not None:
                    result.append(value)

        return result

    def get_components(self):
        """ return the components of the process list """
        return ProcessList._process_list.keys()


    def is_running(self):
        """ check if some components are running """
        if len(ProcessList._process_list) > 0:
            return True
        else:
            return False

    def wait_process(self, process):
        """ wait that a process stops """
        while not ProcessList._stop.is_set() and \
              process.returncode is None:
            process.poll()
            ProcessList._stop.wait(1)
        if not ProcessList._stop.is_set():
            LOGGER.info("process with pid %s returned %s" %
                        (process.pid, process.returncode))
        ProcessList._wait = None

    def check_terminate(self, process):
        """ if terminate does not stop process in 5 seconds, kill it """
        ProcessList._stop.wait(5)
        process.poll()
        if process.returncode is not None:
            process.kill()

    def register_end_callback(self, callback):
        """ registers a callback to be called when a process is stopped """
        ProcessList._callbacks.append(callback)


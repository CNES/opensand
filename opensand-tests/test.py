#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 TAS
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
# Author: Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>

"""
test.py - run the OpenSAND automatic tests
"""

import traceback
from threading import Thread, Lock
import glob
import sys
import os
import time
import re
from optparse import OptionParser, IndentedHelpFormatter
import textwrap
import ConfigParser
import socket
import shlex
import subprocess
import shutil

from opensand_manager_core.loggers.levels import MGR_WARNING, MGR_INFO, MGR_DEBUG
from opensand_manager_core.my_exceptions import CommandException
from opensand_manager_core.utils import copytree, red, blue, green, yellow
from opensand_manager_shell.opensand_shell_manager import ShellManager, \
                                                          BaseFrontend, \
                                                          SERVICE

# TODO SCPC tests are performed with MODCOD 7 but it would be better to use
# MODCOD 28. The problem is that the default MODCOD file is for RCS and contains
# MODCOD ids for RCS that are smaller than 7
# we should add elements to get other simulation files


class IndentedHelpFormatterWithNL(IndentedHelpFormatter):
    """ parse '\n' in option parser help
        From Tim Chase via Google group comp.lang.python """
    def format_description(self, description):
        """ format the text description for help message """
        if not description:
            return ""
        desc_width = self.width - self.current_indent
        indent = " " * self.current_indent
        # the above is still the same
        bits = description.split('\n')
        formatted_bits = [
          textwrap.fill(bit,
            desc_width,
            initial_indent=indent,
            subsequent_indent=indent)
          for bit in bits]
        result = "\n".join(formatted_bits) + "\n"
        return result

    def format_option(self, option):
        """ The help for each option consists of two parts:
              * the opt strings and metavars
              eg. ("-x", or "-fFILENAME, --file=FILENAME")
              * the user-supplied help string
              eg. ("turn on expert mode", "read data from FILENAME")

            If possible, we write both of these on the same line:
              -x    turn on expert mode

            But if the opt string list is too long, we put the help
            string on a second line, indented to the same column it would
            start in if it fit on the first line.
              -f FILENAME, --file=FILENAME
                  read data from FILENAME
        """
        result = []
        opts = self.option_strings[option]
        opt_width = self.help_position - self.current_indent - 2
        if len(opts) > opt_width:
            opts = "%*s%s\n" % (self.current_indent, "", opts)
            indent_first = self.help_position
        else: # start help on same line as opts
            opts = "%*s%-*s  " % (self.current_indent, "", opt_width, opts)
            indent_first = 0
        result.append(opts)
        if option.help:
            help_text = self.expand_default(option)
            # Everything is the same up through here
            help_lines = []
            for para in help_text.split("\n"):
                help_lines.extend(textwrap.wrap(para, self.help_width))
                # Everything is the same after here
            result.append("%*s%s\n" % (
                indent_first, "", help_lines[0]))
            result.extend(["%*s%s\n" % (self.help_position, "", line)
                for line in help_lines[1:]])
        elif opts[-1] != "\n":
            result.append("\n")
        return "".join(result)

class TestScenario:
    TEST_BASE = "base"
    TEST_NON_BASE = "nonbase"
    TEST_OTHER = "other"
    
    """ Test scenario data """
    def __init__(self, name, path, parent = None):
        self._name = name
        self._path = path
        self._parent = parent
        
        self._tests = {TestScenario.TEST_BASE: [],
                       TestScenario.TEST_NON_BASE: [],
                       TestScenario.TEST_OTHER: []
                      }
        
    def __repr__(self):
        enrich = "enrich, " if self.is_enrich() else ""
        return "{name: %s, %spath: %s}" % (self._name, enrich, self._path)
    
    def __str__(self):
        return repr(self)
 
    def get_name(self):
        return self._name
    
    def get_path(self):
        return self._path
    
    def get_parent(self):
        return self._parent
    
    def is_enrich(self):
        return (not self._parent is None)
    
    def get_base_tests(self):
        return self._tests[TestScenario.TEST_BASE]
    
    def get_non_base_tests(self):
        return self._tests[TestScenario.TEST_NON_BASE]

    def get_other_tests(self):
        return self._tests[TestScenario.TEST_OTHER]

class Test:
    """ Test data """
    
    """ Test status """
    READY = 0
    SUCCESSFUL = 1
    FAILED = 2
    
    def __init__(self, name, path):
        self._name = name
        self._path = path
        self._status = Test.READY
        self._error = ""
        self._lastlogs = []
        
    def __repr__(self):
        status = {Test.READY: "ready",
                  Test.SUCCESSFUL: "successful",
                  Test.FAILED: "failed"}
        errtxt = ",error %s" % self._error if 0 < len(self._error) else ""
        txt = "{name: %s, status: %s, path: %s%s}" % (self._name, 
                                                      status[self._status], 
                                                      self._path,
                                                      errtxt)
        return txt
    
    def __str__(self):
        return repr(self)
    
    def reset(self):
        self._status = Test.READY
        self._error = ""
        self._lastlogs = []

    def get_name(self):
        return self._name
    
    def get_path(self):
        return self._path
    
    def get_status(self):
        return self._status
    
    def set_status(self, status):
        self._status = status

    def has_error(self):
        return 0 < len(self._error)
    
    def get_error(self):
        return self._error
    
    def set_error(self, message):
        self._error = message
        
    def get_last_logs(self):
        return self._lastlogs
    
class TestConsole:
    
    def __init__(self):
        self._mutex = Lock()
        self._msg = []
        
    def _add_msg(self, error, key, message):
        """ add message """
        self._msg.append([error, key, message])

    def _has_msg(self, error = None):
        """ check message presence """
        if error is None:
            return (0 < len(self._msg))
        
        for m in self._msg:
            if m[0] == error:
                return True
        return False
    
    def _build_msg(self, error = None):
        """ clone obj """
        copy = {}
        for m in self._msg:
            if not error is None and m[0] != error:
                continue
            if m[1] not in copy:
                copy[m[1]] = []
            if error is None:
                copy[m[1]].append([m[0], m[2]])
            else:
                copy[m[1]].append(m[2])
            
        return copy
 
    def clear(self):
        """ clear message """
        self._mutex.acquire()
        self._msg = []
        self._mutex.release()

    def output(self, key, message):
        """ add message to output """
        self._mutex.acquire()
        self._add_msg(False, key, message)
        self._mutex.release()

    def error(self, key, message):
        """ add message to error """
        self._mutex.acquire()
        self._add_msg(True, key, message)
        self._mutex.release()
         
    def has_message(self):
        """ return message presence """
        self._mutex.acquire()
        stat = self._has_msg()
        self._mutex.release()
        return stat
        
    def has_output(self):
        """ return output presence """
        self._mutex.acquire()
        stat = self._has_msg(False)
        self._mutex.release()
        return stat
    
    def has_error(self):
        """ return error presence """
        self._mutex.acquire()
        stat = self._has_msg(True)
        self._mutex.release()
        return stat
    
    def get_message(self):
        """ return a copy of message """
        self._mutex.acquire()
        copy = self._build_msg()
        self._mutex.release()
        return copy
    
    def get_output(self):
        """ return a copy of output """
        self._mutex.acquire()
        copy = self._build_msg(False)
        self._mutex.release()
        return copy
        
    def get_error(self):
        """ return a copy of error """
        self._mutex.acquire()
        copy = self._build_msg(True)
        self._mutex.release()
        return copy

class TestManager(ShellManager):
    
    """ Log level for the OpenSAND platform """
    QUIET = MGR_WARNING
    VERBOSE = MGR_INFO
    DEBUG = MGR_DEBUG
    
    """ Enrich folder """
    ENRICH_FOLDER = "enrich"
    
    def __init__(self, loglvl = QUIET, lastlogs = False):
        """ initialize the Test Manager """
        ShellManager.__init__(self)
        
        # Set default parameters
        self._debug = False
        self._loglvl = loglvl
        self._lastlogs = lastlogs

        self._service = None
        self._workstation = False
        
        self._displayonly = False
        self._waitstarttime = 2
        self._waitstarttries = 5
        self._basetesttries = 2
        
        # Initialize platform manager
        self._frontend = BaseFrontend()
        self._hosts = {
            "st": [],
            "gw": [],
            "sat": [],
            "*": []
        }
        self._runninghosts = []
        self._base = None
        
        # Initialize scenarios
        self._folder = "./tests/"
        self._scenarios = None
        
        # Set internal parameters
        self._threads = []

    def _trace_debug(self, message):
        """ Trace debug message """
        if self._debug:
            print message

    def _trace_info(self, message):
        """ Trace informative message """
        if self._loglvl != TestManager.QUIET:
            print message
        self._log.info(" * %s " % message)
        
    def _trace_warning(self, message):
        """ Trace warning message """
        #print red("WARNING: ") + message
        self._log.warning(" * %s" % message)
        
    def _trace_error(self, message):
        """ Trace error message """
        print red("ERROR: ") + message
        self._log.error(" * %s" % message)

    def prepare_platform(self, service = SERVICE, workstation = False,
                         displayonly = False):
        """ initialize and prepare the OpenSAND platform """
        
        # Save arguments
        self._service = service        
        self._workstation = workstation
        
        self._displayonly = displayonly

        # Prepare platform
        try:
            self.load(log_level = self._loglvl, 
                      service = self._service,
                      with_ws = self._workstation,
                      remote_logs = True,
                      frontend = self._frontend
                     )
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to prepare the OpenSAND platform
            self._trace_error("Unable to prepare the OpenSAND platform: " + str(ex))
            
            return False
        
        # Stop platform
        self._stop_platform()

        # Save base configuration
        self._base = self._model.get_scenario()
        
        # Update hosts of the platform OpenSAND
        self.update_platform_hosts()
        
        # Check the SAT count
        n = len(self._hosts["sat"])
        if n == 0:
            self._trace_error("No SAT is available")
            return False
        elif 1 < n:
            dummy = map(lambda hostname: str(hostname), self._hosts["sat"])
            self._traceError("More than one SAT is available: %s" 
                             % str(dummy))
            return False
 
        # Check the GW count
        n = len(self._hosts["gw"])
        if n == 0:
            self._trace_error("No GW is available")
            return False
        elif 1 < n:
            # TODO: This case has to be handled
            dummy = map(lambda hostname: str(hostname), self._hosts["gw"])
            self._traceError("More than one GW is available: %s"
                             % str(dummy))
            return False
        
        # Check the ST count
        n = len(self._hosts["st"])
        if n == 0:
            self._trace_error("No ST is available")
            return False
        
        return True
    
    def close_platform(self):
        """ close the OpenSAND platform """
        
        # Close pending threads
        if 0 < len(self._threads):
            self._trace_info("Stopping pending threads")
            for thread in self._threads:
                thread.join()
                self._threads.remove(thread)
            self._trace_info("Threads stopped")
        
        # Close platform
        try:
            self.close()
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to close the OpenSAND platform
            self._trace_error("Unable to close the OpenSAND platform: " + str(ex))
            
            return False
        
        # Close hosts status
        for key in self._hosts:
            self._hosts[key] = []
        self._runninghosts = []
            
        return True
    
    def update_platform_hosts(self):
        """ update each host of the OpenSAND platform """
         
        # Close hosts status
        for key in self._hosts:
            self._hosts[key] = []
        
        # Get hosts
        hosts = []
        try:
            hosts.extend(self._model.get_hosts_list())
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to get hosts
            self._trace_warning("Unable to get hosts: " + str(ex))
        
        # Sort hosts
        hosttypes = list(self._hosts.keys())
        hosttypes.remove("*")
        for host in hosts:
            hostname = str(host.get_name())
            hosttype = "*"
            for key in hosttypes:
                if hostname.lower().startswith(key):
                    hosttype = key
                    break
            self._hosts[hosttype].append(hostname)

        # Display host
        for hosttype in self._hosts:
            if len(self._hosts[hosttype]) <= 0:
                continue
            msg = ""
            for hostname in self._hosts[hosttype]:
                msg += " " + hostname
             
            print "{:<15} {:}".format(blue(hosttype.upper()) + ":", msg)

    def _start_platform(self):
        """ start the OpenSAND platform """
        
        try:
            ShellManager.start_opensand(self)
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            return False
        
        # wait to be sure the plateform is started
        started = False
        waitstarttry = 0
        comment = ""
        while not started and waitstarttry < self._waitstarttries:
            self._trace_debug("Sleep %s sec to waiting for the platform "
                             "OpenSAND starting%s" 
                             % (str(self._waitstarttime), comment))
            time.sleep(self._waitstarttime)
            
            waitstarttry += 1
            comment = " (try n°%s)" % str(waitstarttry + 1)
            
            # Initialize hosts status
            self._update_platform_hosts_status()
            
            # Check hosts have started
            started = True
            hosts = []
            for hosttype in self._hosts:
                for hostname in self._hosts[hosttype]:
                    hosts.append(hostname.lower())
            for hostname in hosts:
                if not hostname in self._runninghosts:
                    started = False
                    break
        
        return True

    def _clear_platform_hosts_status(self):
        """ clear hosts status of the OpenSAND platform """
        
        # Clear plateform hosts
        self._runninghosts = []
        
    def _initializePlatformHostsStatus(self):
        """ initialize hosts status of the OpenSAND platform """
                
        # Clear plateform status
        self._clear_platform_hosts_status()
        
        # Get running hosts
        try:
            self._runninghosts.extend(self._model.running_list())
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to get running hosts
            self._trace_warning("Unable to get running hosts: " + str(ex))
            
        self._runninghosts = map(lambda x: x.lower(), self._runninghosts)
        
    def _update_platform_hosts_status(self, display = False):
        """ update hosts status of the OpenSAND platform """
        time.sleep(5)
        
        # Get old hosts
        runninghosts = self._runninghosts
        
        # Get current running hosts
        self._runninghosts = []      
        try:
            self._runninghosts.extend(self._model.running_list())
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to get running hosts
            self._trace_warning("Unable to get running hosts: " + str(ex))
            
        self._runninghosts = map(lambda x: x.lower(), self._runninghosts)
        
        # Check hosts
        for hosttype in self._hosts:
            if hosttype != "*" and len(self._hosts[hosttype]) <= 0:
                # No host
                self._trace_warning("No host of type '%s' is detected" % hosttype)

            for hostname in self._hosts[hosttype]:
                old = (hostname.lower() in runninghosts)
                new = (hostname.lower() in self._runninghosts)
                
                if not old and new:
                    # Host have started
                    self._trace_info("Host '%s' have started" % hostname)
                    if display:
                        print "Host '%s' have started" % hostname
                        
                elif old and not new:
                    # Host may have crashed
                    self._trace_warning("Host '%s' may have crashed" % hostname)
                    
                elif new:
                    # Host is running
                    self._trace_info("Host '%s' is running" % hostname)
                    if display:
                        print "Host '%s' is running" % hostname
                        
                else:
                    # Host is stopped
                    self._trace_warning("Host '%s' is stopped" % hostname)
                    if display:
                        print "Host '%s' is stopped" % hostname

    def _stop_platform(self):
        """ stop the OpenSAND platform """
        
        try:
            ShellManager.stop_opensand(self)
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            return False
        
        # wait until effectively stopped
        for i in range(10):
            if not list(self._model.running_list()):
                break
            time.sleep(1)
        if i==9:
            self._trace_warning("Host is has not stopped!")

        # Clear hosts status
        self._clear_platform_hosts_status()
        
        return True

    def prepare_scenarios(self, folder = "./tests/", 
                          regexptest = None, regexptype = None):
        """ list and filter the test scenarios to pass """
        
        # Save arguments
        self._folder = folder
        
        # Prepare path
        basepath = os.path.join(self._folder, "base")
        typespath = os.path.join(basepath, "types")
        configspath = os.path.join(basepath, "configs")
        otherspath = os.path.join(self._folder, "other")

        # Prepare test types
        types = []
        try:
            types.extend(os.listdir(typespath))
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to get test types
            self._trace_warning("Unable to get test types: " + str(ex))
            pass
        if len(types) <= 0:
            # No test type to execute
            self._trace_warning("No test type to execute")

            return False
        
        # Prepare test configurations
        configs = []
        try:
            configs.extend(glob.glob(configspath + "/*"))
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to get test configurations
            self._trace_error("Unable to get test configurations: " + str(ex))
            
            return False
        if len(configs) <= 0:
            # No test configuration to execute
            self._trace_warning("No test configuration to execute")
            
            return False
        
        # Prepare other tests
        others = []
        try:
            others.extend(glob.glob(otherspath + "/*"))
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to get other tests
            self._trace_error("Unable to get other test configurations: " + str(ex))
            
            return False
        
        # Prepare scenarios
        self._scenarios = []
        enrichconfigspath = os.path.join(configspath,
                                         TestManager.ENRICH_FOLDER)
        i = 0
        while i < len(configs):
            path = configs[i]
            
            if not os.path.isdir(path):
                del configs[i]
                continue
            
            if os.path.basename(path) == TestManager.ENRICH_FOLDER:
                del configs[i]
                continue
            
            # Add scenario
            name = os.path.basename(path)
            scenario = TestScenario(name, path)
            self._add_tests_to_scenario(scenario, types, typespath)
            self._scenarios.append(scenario)
            i += 1
            
            # Get enrich scenarios
            enrichscenarios = self._prepare_enrich_scenarios(scenario, 
                                                           enrichconfigspath)
            if not enrichscenarios is None:
                self._scenarios.extend(enrichscenarios)
        
        # Sort tests and scenarios
        for scenario in self._scenarios:
            scenario.get_base_tests().sort(key=lambda x:x.get_name())
            scenario.get_non_base_tests().sort(key=lambda x:x.get_name())
        self._scenarios.sort(key=lambda x:x.get_name())
 
        # Prepare other scenarios
        i = 0
        while i < len(others):
            path = others[i]
            
            if not os.path.isdir(path):
                del others[i]
                continue
            
            # Add other scenario
            name = os.path.basename(path)
            scenario = TestScenario(name, "")
            scenario.get_other_tests().append(Test(name, path))
            self._scenarios.append(scenario)
            i += 1

        # Apply the regular expression filtering to scenarios
        if not regexptest is None and regexptest != "":
            i = 0
            while i < len(self._scenarios):
                # Check configuration name
                name = self._scenarios[i].get_name()
                matched = False
                try:
                    matched = re.search(regexptest, name)
                except KeyboardInterrupt:
                    raise
                except Exception as ex:
                    # Unable to match name with regular expression
                    self._trace_warning("Unable to match scenario name with "
                                       "regular expression: " + str(ex))
                    pass
                if matched:
                    i += 1
                    continue
         
                # Remove scenario
                self._scenarios.pop(i)
 
        # Apply the regular expression filtering to tests
        if not regexptype is None and regexptype != "":
            j = 0
            while j < len(self._scenarios):
                
                # Get scenario
                scenario = self._scenarios[j]
                
                # Check base tests
                i = 0
                while i < len(scenario.get_base_tests()):
                    # Check configuration name
                    name = scenario.get_base_tests()[i].get_name()
                    matched = False
                    try:
                        matched = re.search(regexptype, name)
                    except KeyboardInterrupt:
                        raise
                    except Exception as ex:
                        # Unable to match name with regular expression
                        self._trace_warning("Unable to match base test name with "
                                           "regular expression: " + str(ex))
                        pass
                    if matched:
                        i += 1
                        continue
         
                    # Remove test
                    scenario.get_base_tests().pop(i)
                 
                # Check non base tests
                i = 0
                while i < len(scenario.get_non_base_tests()):
                    # Check configuration name
                    name = scenario.get_non_base_tests()[i].get_name()
                    matched = False
                    try:
                        matched = re.search(regexptype, name)
                    except KeyboardInterrupt:
                        raise
                    except Exception as ex:
                        # Unable to match name with regular expression
                        self._trace_warning("Unable to match non base test name with "
                                           "regular expression: " + str(ex))
                        pass
                    if matched:
                        i += 1
                        continue
         
                    # Remove test
                    scenario.get_non_base_tests().pop(i)
                  
                # Check other tests
                i = 0
                while i < len(scenario.get_other_tests()):
                    # Check configuration name
                    name = scenario.get_other_tests()[i].get_name()
                    matched = False
                    try:
                        matched = re.search(regexptype, name)
                    except KeyboardInterrupt:
                        raise
                    except Exception as ex:
                        # Unable to match name with regular expression
                        self._trace_warning("Unable to match other test name with "
                                           "regular expression: " + str(ex))
                        pass
                    if matched:
                        i += 1
                        continue
         
                    # Remove test
                    scenario.get_other_tests().pop(i)
                
                # Check scenario and remove it if required
                if len(scenario.get_base_tests()) <= 0 \
                   and len(scenario.get_non_base_tests()) <= 0 \
                   and len(scenario.get_other_tests()) <= 0:
                    self._scenarios.pop(j)
                else:
                    j += 1

        if len(configs) <= 0:
            # No test configuration to execute
            self._trace_warning("No test scenario to execute")
            
            return False
       
        return True
    
    def _add_tests_to_scenario(self, scenario, testnames, testspath):
        """ check tests are base or non base tests and add them to 
        scenario """
        
        for name in testnames:
            test = Test(name, os.path.join(testspath, name))
            files = []
            try:
                files = glob.glob(test.get_path())
            except KeyboardInterrupt:
                raise
            except Exception as ex:
                # Unable to get test files
                self._trace_warning("Unable to get test '%s' files for scenario '%s'"
                                   % (test.get_name(), scenario.get_name()))
        
            i = 0
            while i < len(files) and not files[i].endswith(".xslt"):
                i += 1
            if i < len(files):
                scenario.get_non_base_tests().append(test)
            else:
                scenario.get_base_tests().append(test)
       
    def _prepare_enrich_scenarios(self, parent, enrichconfigspath): 
        """ list the enrich test scenario to pass """
        
        # Search enrich test configurations
        enrichscenarios = []
        enrichconfigs = []
        try:
            enrichconfigs = glob.glob(enrichconfigspath + "/*")
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to get enrich test configuration"
            self._trace_warning("Unable to get enrich '%s' test "
                               "configurations: %s" 
                               % (parent.get_name(), str(ex)))
               
        basetestnames = {}
        for test in parent.get_base_tests():
            basetestnames[test.get_name()] = test
        nonbasetestnames = {}
        for test in parent.get_non_base_tests():
            nonbasetestnames[test.get_name()] = test
            
        for conf in enrichconfigs:
            if os.path.basename(conf).startswith("scenario"):
                continue
            if not os.path.isdir(conf):
                continue
             
            # Generate enrich test name
            pos = conf.find(TestManager.ENRICH_FOLDER)
            path = conf[pos + 1 + len(TestManager.ENRICH_FOLDER):]
            enrichname = parent.get_name()
            for dummy in path.split("/"):
                enrichname += "_" + dummy

            # Check enrich test is allowed
            path = os.path.join(conf, "ignored")
            if not os.path.isfile(path):
                continue
            with open(path, "r") as lines:
                ignored = False
                for line in lines:
                    line = line.strip()
                      
                    # ignore can contain position in the test name
                    # to check for the specific pattern
                    items = line.split(',')
                    if len(items) == 1:
                        ignored = (0 <= enrichname.find(items[0]))
                    else:
                        pos = int(items[1])
                        dummy = enrichname.split('_')
                        ignored = (pos < len(dummy) and dummy[pos] == items[0])
                    if ignored:
                        break
                if ignored:
                    continue
                
            # Add enrich scenario
            enrichscenario = TestScenario(enrichname, conf, parent)
            enrichscenarios.append(enrichscenario)

            # Check enrich name is accepted
            path = os.path.join(conf, "accepts")
            if not os.path.isfile(path):
                # Copy tests from parent scenario
                enrichscenario.get_base_tests().extend(parent.get_base_tests())
                enrichscenario.get_non_base_tests().extend(parent.get_non_base_tests())
                continue
            
            # Check accepted tests are compliant with parents cenario tests
            with open(path, "r") as lines:
                for accepted in lines:
                    test = accepted.strip()
                    if test in basetestnames:
                        enrichscenario.get_base_tests().append(basetestnames[test])
                    elif test in nonbasetestnames:
                        enrichscenario.get_non_base_tests().append(nonbasetestnames[test])
        
        return enrichscenarios
    
    def clean_scenarios(self):
        """ clean the test scenarios """
        
        # Remove scenarios files
        try:
            path = "/tmp/opensand_tests/last_errors"
            if os.path.exists(path):
                os.remove(path)
            path = "/tmp/opensand_tests/result"
            if os.path.exists(path):
                os.remove(path)
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            # Unable to clean scenarios
            self._trace_error("Unable to clean scenarios: " + str(ex))
            
            return False
        
        return True
            
    def run_scenarios(self, first_scenario=0):
        """ run the test scenarios """
        
        if not self._displayonly:
            # Modify the service in the test library
            path = os.path.join(self._folder, ".lib/opensand_tests.py")
            content = ""
            try:
                with open(path, "r") as flib:
                    for line in flib:
                        if line.startswith("TYPE ="):
                            content += "TYPE = \"%s\"\n" % self._service
                        else:
                            content += line
                with open(path, "w") as flib:
                    flib.write(content)
            except KeyboardInterrupt:
                raise
            except Exception as ex:
                # Unable to edit the test libraries
                self._trace_error("Unable to edit the test libraries: %s"
                                 % str(ex))
            
        # Initilaize test results
        results = {}
        def add_result(results, scenario, test, success):
            if not test in results:
                results[test] = {}
                
            values = scenario.split("_")
            if 4 <= len(values):
                values[2] = values[2] + "_" + values[3]
                del values[3]
                
            for val in values:
                if val not in results[test]:
                    results[test][val] = [0, 0]
                if success:
                    results[test][val][0] += 1
                else:
                    results[test][val][1] += 1
 
        # Evaluate scenarios count by test category (config vs other)
        nconfigscenarios = 0
        notherscenarios = 0
        for scenario in self._scenarios:
            
            if 0 < len(scenario.get_base_tests()) \
               or 0 < len(scenario.get_non_base_tests()):
                nconfigscenarios += 1
                
            if 0 < len(scenario.get_other_tests()):
                notherscenarios += 1

        # Run scenarios tests (base and non-base)
        i = 0
        for scenario in self._scenarios:
            
            if len(scenario.get_base_tests()) <= 0 \
               and len(scenario.get_non_base_tests()) <=0:
                continue
            
            i += 1
            self._log.info(" * [%s/%s] Test %s with base configuration" 
                            % (str(i), str(nconfigscenarios + notherscenarios),
                               scenario.get_name()))
            if self._loglvl == TestManager.QUIET:
                print "[%s/%s] Test configuration %s" % \
                        (str(i), str(nconfigscenarios + notherscenarios), 
                         blue(scenario.get_name()))
                sys.stdout.flush()
            
            if i < first_scenario:
                print "Ignoring this scenario..."
                continue

            if not self._displayonly:    
                # Initialize the model
                self._model.set_scenario(self._base)
                self._model.set_run("")
            
                # Build scenario model
                initobj = self._build_scenario_model(scenario)
                if initobj is None:
                    self._trace_warning("Unable to deploy the scenario '%s'"
                                       % scenario.get_name())
            
            # Check base tests existence
            if 0 < len(scenario.get_base_tests()):
                
                self._trace_debug("Start '%s' base tests execution" 
                                 % scenario.get_name())
                
                if not self._displayonly:
                    # Load scenario model
                    self._model.set_scenario(initobj)
                
                    # Start the OpenSAND platform
                    self._model.set_run("base_tests")
                    if not self._start_platform():
                        self._trace_error("Unable to start the OpenSAND "
                                         "platform")
                        continue
            
                # Run base tests
                for test in scenario.get_base_tests():
                    
                    if self._displayonly:
                        msg = "Test %s " % test.get_name()
                        print "{:<40} ".format(msg)
                        sys.stdout.flush()
                        continue

                    self._log.info(" * Start %s" % test.get_name())
                    if self._loglvl == TestManager.QUIET:
                        msg = "Start %s " % test.get_name()
                        print "{:<40} ".format(msg),
                        sys.stdout.flush()
                    
                    # Run the base test
                    testtry = 1
                    detail = ""
                    self._run_test(scenario, test)
                    
                    # Check retry
                    while test.get_status() != Test.SUCCESSFUL \
                          and testtry < self._basetesttries:
                        
                        # Update data
                        testtry += 1
                        detail = " at try n°%s" % str(testtry)
                        
                        # Stop the OpenSAND platform
                        self._stop_platform()
                        
                        # Start the OpenSAND platform
                        self._start_platform()
                        
                        # Re-run the base test
                        self._run_test(scenario, test)
                        
                    if test.get_status() == Test.SUCCESSFUL:
                        self._log.info(" * Test %s successful%s" %
                                       (test.get_name(), detail))
                        if self._loglvl == TestManager.QUIET:
                            print "{:>7}{:}".format(green("SUCCESS"), detail)
                            sys.stdout.flush()
                            
                    elif test.get_status() == Test.READY:
                        if self._loglvl == TestManager.QUIET:
                            print "{:>7}\nNo test launched for {:}".format( 
                                red("ERROR"), test.get_name() + detail)
                            sys.stdout.flush()
                        else:
                            self._log.error(" * No test launched for %s%s" 
                                            % (test.get_name(), detail))
                            
                    else:
                        if self._loglvl == TestManager.QUIET:
                            print "{:>7}\n{:}".format(red("ERROR"),
                                                      test.get_error() + detail)
                            sys.stdout.flush()
                        else:
                            self._log.error(" * Test %s failed: %s%s" 
                                            % (test.get_name(), 
                                               test.get_error(),
                                               detail))
                    
                    # Add result
                    add_result(results,
                               scenario.get_name(),
                               test.get_name(),
                               test.get_status() == Test.SUCCESSFUL)
                           
                    if 0 < len(test.get_last_logs()):
                        self.__save_last_logs(scenario, test)

                    # Update status
                    self._update_platform_hosts_status()
                        
                if not self._displayonly:
                    # Stop the OpenSAND platform
                    if not self._stop_platform():
                        self._trace_warning("Unable to stop the OpenSAND "
                                           "platform")
                
                self._trace_debug("End '%s' base tests execution"
                                 % scenario.get_name())
                
            # Check non base tests existence
            if 0 < len(scenario.get_non_base_tests()):
                
                self._trace_debug("Start '%s' non base tests execution"
                                 % scenario.get_name())
                
                # Run non base tests
                for test in scenario.get_non_base_tests():
                    
                    if self._displayonly:
                        msg = "Test %s " % test.get_name()
                        print "{:<40} ".format(msg)
                        sys.stdout.flush()
                        continue
                    
                    self._log.info(" * Start %s" % test.get_name())
                    if self._loglvl == TestManager.QUIET:
                        msg = "Start %s " % test.get_name()
                        print "{:<40} ".format(msg),
                        sys.stdout.flush()

                    # Load scenario model
                    self._model.set_scenario(initobj)
                    
                    # Build the non base test model
                    if self._build_test_model(scenario, test) is None:
                        self._trace_warning("Unable to deploy the non base "
                                           "test '%s' for scenario '%s'"
                                           % (test.get_name(),
                                              scenario.get_name()))
                    
                    # Start the OpenSAND platform
                    self._model.set_run("non_base_test_" + test.get_name())
                    self._start_platform()
                    
                    # Run the non base test
                    self._run_test(scenario, test)

                    # Update status
                    self._update_platform_hosts_status()
                    
                    # Stop the OpenSAND platform
                    self._stop_platform()
                    
                    if test.get_status() == Test.SUCCESSFUL:
                        self._log.info(" * Test %s successful" % test.get_name())
                        if self._loglvl == TestManager.QUIET:
                            print "{:>7}".format(green("SUCCESS"))
                            sys.stdout.flush()
                            
                    elif test.get_status() == Test.READY:
                        if self._loglvl == TestManager.QUIET:
                            print "{:>7}\nNo test launched for {:}".format(
                                red("ERROR"), test.get_name())
                            sys.stdout.flush()
                        else:
                            self._log.error(" * No test launched for %s" 
                                            % test.get_name())
                            
                    else:
                        if self._loglvl == TestManager.QUIET:
                            print "{:>7}\n{:}".format(red("ERROR"),
                                                     test.get_error())
                            sys.stdout.flush()
                        else:
                            self._log.error(" * Test %s failed: %s" 
                                            % (test.get_name(), test.get_error()))
 
                    # Add result
                    add_result(results,
                               scenario.get_name(),
                               test.get_name(),
                               test.get_status() == Test.SUCCESSFUL)
                    
                    if 0 < len(test.get_last_logs()):
                        self.__save_last_logs(scenario, test)

                self._trace_debug("End '%s' non base tests execution"
                                 % scenario.get_name())
        
        # Run scenarios tests (other)
        for scenario in self._scenarios:
            
            if len(scenario.get_other_tests()) <= 0:
                continue
            i += 1
            
            self._log.info(" * [%s/%s] Test %s" 
                            % (str(i), str(nconfigscenarios + notherscenarios),
                               scenario.get_name()))
            if self._loglvl == TestManager.QUIET:
                print "[%s/%s] Test %s" % \
                        (str(i), str(nconfigscenarios + notherscenarios), 
                         blue(scenario.get_name()))
                sys.stdout.flush()
             
            if not self._displayonly:    
                # Initialize the model
                self._model.set_scenario(self._base)
                self._model.set_run("")
                
                # Build scenario model
                initobj = self._base
                
            self._trace_debug("Start '%s' other tests execution"
                             % scenario.get_name())
                
            # Run other tests
            for test in scenario.get_other_tests():
                    
                if self._displayonly:
                    msg = "Test %s " % test.get_name()
                    print "{:<40} ".format(msg)
                    sys.stdout.flush()
                    continue
                    
                self._log.info(" * Start %s" % test.get_name())
                if self._loglvl == TestManager.QUIET:
                    msg = "Start %s " % test.get_name()
                    print "{:<40} ".format(msg),
                    sys.stdout.flush()

                # Load scenario model
                self._model.set_scenario(initobj)
                
                # Build the other test model
                if self._build_test_model(scenario, test) is None:
                    self._trace_warning("Unable to deploy the other "
                                       "test '%s' for scenario '%s'"
                                       % (test.get_name(),
                                          scenario.get_name()))
                    
                # Start the OpenSAND platform
                self._model.set_run("other_test_" + test.get_name())
                self._start_platform()
                
                # Run the non base test
                self._run_test(scenario, test)
                
                # Update status
                self._update_platform_hosts_status()
                    
                # Stop the OpenSAND platform
                self._stop_platform()
                    
                if test.get_status() == Test.SUCCESSFUL:
                    self._log.info(" * Test %s successful" % test.get_name())
                    if self._loglvl == TestManager.QUIET:
                        print "{:>7}".format(green("SUCCESS"))
                        sys.stdout.flush()
                            
                elif test.get_status() == Test.READY:
                    if self._loglvl == TestManager.QUIET:
                        print "{:>7}\nNo test launched for {:}".format(
                            red("ERROR"), test.get_name())
                        sys.stdout.flush()
                    else:
                        self._log.error(" * No test launched for %s" 
                                        % test.get_name())
                            
                else:
                    if self._loglvl == TestManager.QUIET:
                        print "{:>7}\n{:}".format(red("ERROR"),
                                                 test.get_error())
                        sys.stdout.flush()
                    else:
                        self._log.error(" * Test %s failed: %s" 
                                        % (test.get_name(), test.get_error()))
 
                # Add result
                add_result(results,
                          scenario.get_name(),
                          test.get_name(),
                          test.get_status() == Test.SUCCESSFUL)
                  
                if 0 < len(test.get_last_logs()):
                    self.__save_last_logs(scenario, test)

            self._trace_debug("End '%s' other tests execution"
                             % scenario.get_name())
       
        # Finalize the test report
        if 0 < len(results):
            
            # Print header
            print "+-----------+-----------+-----------+-----------+"
            line = "| {:>9} | {:>9} | {:>9} | {:>9} |"
            print line.format("TEST", green(" SUCCESS "), red("  ERROR  "),
                              "% error")

            # Print test results
            for testname in results:
                print "+-----------+-----------+-----------+-----------+"
                print "| {:^45} |".format(testname)
                print "+-----------+-----------+-----------+-----------+"
                
                # Print scenario results
                for name in results[testname]:
                    nSuccess = results[testname][name][0]
                    nError = results[testname][name][1]
                    percent = int(100 * float(nError) / \
                                  float(nSuccess + nError))
                    if percent == 100:
                        percent = red("     %s " % percent )
                    elif percent > 75:
                        percent = red("      %s " % percent )
                    elif percent == 0:
                        precent = green("       %s " % percent)
                    else:
                        percent = str(percent)
                    print line.format(name,
                                      results[testname][name][0],
                                      results[testname][name][1],
                                      percent)
                        
            print "+-----------+-----------+-----------+-----------+"

        if not self._displayonly:
            # Restore the service in the test library
            path = os.path.join(self._folder, ".lib/opensand_tests.py")
            content = ""
            try:
                with open(path, "r") as flib:
                    for line in flib:
                        if line.startswith("TYPE ="):
                            content += "TYPE = \"%s\"\n" % SERVICE
                        else:
                            content += line
                with open(path, "w") as flib:
                    flib.write(content)
            except Exception as ex:
                pass
 
        return True
    
    def __save_last_logs(self, scenario, test):
        """ Save or display last logs """

        if self._lastlogs:
            print
            print "\n".join(test.get_last_logs())
        else:
            try:
                if not os.path.exists('/tmp/opensand_tests'):
                    os.mkdir('/tmp/opensand_tests')
                with open('/tmp/opensand_tests/last_errors', 'a') as output:
                    output.write(yellow("Results for configuration %s, test %s:\n" %
                                 (scenario.get_name(), test.get_name()), True))
                    output.write("\n".join(test.get_last_logs()))
            except KeyboardInterrupt as ex:
                raise
            except Exception as ex:
                self._trace_warning("Unable to save last logs: %s" % str(ex))
                
    def __build_model(self, name, folder):
        """ build the model on the OpenSAND platform """
        
        # Copy model reference directory
        path = os.path.join(folder, "scenario_" + name)
        try:
            if os.path.exists(path):
                shutil.rmtree(path)
            os.makedirs(path)
            copytree(self._model.get_scenario(), path)
        except KeyboardInterrupt:
            raise
        except (OSError, IOError), (_, strerror):
            self._trace_error("Unable to create model directory '%s': %s"
                             % (path, str(strerror)))
            return None
        except Exception as ex:
            self._trace_error("Unable to create model directory '%s': %s"
                             % (path, str(ex)))
            return None
        
        # Update model with XSLT files for hosts
        self._model.set_scenario(path)
        for host in self._model.get_hosts_list() + [self._model]:
            
            hostname = host.get_component().lower()
            xslt = os.path.join(folder, "core_%s.xslt" % hostname)
            
            # specific case for terminals if there is a XSLT
            # for a specific one
            if hostname == "st":
                for st in os.listdir(path):
                    if not st.startswith("st"):
                        continue
                    st_id = st[2:]
                    if host.get_instance() != st_id:
                        continue
                    xslt_path = os.path.join(folder, "core_st%s.xslt" % st_id)
                    if os.path.exists(xslt_path):
                        xslt = xslt_path
                        break
                    
            # TODO: specific case for gateways if there is a XSLT
            #       for a specific one
           
            if not os.path.exists(xslt):
                continue
            
            conf = host.get_advanced_conf().get_configuration()
            try:
                conf.transform(xslt)
            except KeyboardInterrupt:
                raise
            except Exception as ex:
                self._trace_error("Unable to update host model "
                                 "configuration: '%s'" % str(ex))
                return None
                
        # Update mode with XSLT files for topology
        topo = self._model.get_topology()
        conf = topo.get_configuration()
        xslt = os.path.join(folder, 'topology.xslt')
        # specific case for terminals if there is a XSLT for a specific
        # one
        if os.path.exists(xslt):
            try:
                conf.transform(xslt)
                # update the IP address if we add some elements
                topo.update_hosts_address()
            except KeyboardInterrupt:
                raise
            except Exception as ex:
                self._trace_error("Unable to update topology model "
                                 "configuration: '%s'" % str(ex))
                return None
        
        return self._model.get_scenario()

    def _build_scenario_model(self, scenario):
        """ build the test scenario model on the OpenSAND platform """
        
        obj = None
        
        # Load parent scenario if required
        if scenario.is_enrich():
            parent = scenario.get_parent()
            obj = self.__build_model(parent.get_name(), parent.get_path())
            if obj is None:
                self._trace_error("Unable to build model of parent scenario "
                                 "'%s of scenario '%s'" % (parent.get_name(),
                                                           scenario.get_name()))
                return None
        
        # Then, load scenario
        obj = self.__build_model(scenario.get_name(), scenario.get_path())
        if obj is None:
            self._trace_error("Unable to build model of scenario '%s'" 
                             % scenario.get_name())

        return obj
    
    def _build_test_model(self, scenario, test):
        """ deploy the test configuration on the OpenSAND platform """
        
        # Load test
        obj = self.__build_model(test.get_name(), test.get_path())
        if obj is None:
            self._trace_error("Unable to build model of test '%s' of "
                             "scenario '%s'" % (test.get_name(),
                                                scenario.get_name()))
        
        return obj
    
    def _run_test(self, scenario, test):
        """ run the scenario test """
        
        # Reset test results
        test.reset()
        
        # Get ordered host folders
        orders = glob.glob(test.get_path() + '/*')
        
        # Remove common folders from list (not mandatory)
        path = os.path.join(test.get_path(), "files")
        if path in orders:
            orders.remove(path)
        for path in orders:
            if not os.path.isdir(path):
                orders.remove(path)
            elif path.startswith('scenario'):
                orders.remove(path)
        orders.sort()
        
        # Check ordered hosts
        if len(orders) <= 0:
            return
        
        # Prepare console
        console = TestConsole()
        
        # Save and check running hosts list
        running = list(self._model.running_list())
        current = list(running)
        current = map(lambda x: x.upper(), current)
        msg = ""
        for key in self._hosts:
            for host in self._hosts[key]:
                hostname = host.upper()
                msg += "%s" % hostname
                
                if hostname in current:
                    msg += " is running\n"
                    current.remove(hostname)
                else:
                    msg += " is stopped\n"
        for hostname in current:
            msg += "%s is started\n" % hostname
        msg = msg.rstrip("\n")
        self._trace_info(msg)
        
        # Launch test for each ordered host
        for path in orders:
  
            # Get host name
            names = os.path.basename(path).split("_", 1)
            if len(names) < 2:
                self._trace_info("Skip path %s as it has a wrong format" % path)
                continue
            hostname = names[1].upper()
            conf_file = os.path.join(path, "configuration")
            if not os.path.exists(conf_file):
                self._trace_debug("Skip path %s as it does not contain scenario" %
                                 path)
                continue
        
            self._trace_info("Launch command on %s" % hostname)
            
            # read the command section in configuration
            config = ConfigParser.SafeConfigParser()
            if len(config.read(conf_file)) == 0:
                msg =  "Unable to load configuration in '%s'" % path
                console.error("Configuration", msg)
                break

            try:
                config.set('prefix', 'source', self._folder)
            except:
                pass

            try:
                # Check if this is a local test
                if hostname == "TEST":
                    self._launch_local(scenario.get_name(),
                                       test.get_name(),
                                       config,
                                       path,
                                       console)
                else:
                    self._launch_remote(scenario.get_name(),
                                        hostname,
                                        config,
                                        path,
                                        console)
                
                # Wait for test to initialize or stop on host
                time.sleep(0.5)
            except KeyboardInterrupt as ex:
                raise
            except Exception as ex:
                console.error("Test routine", str(ex))
                break
            
        # Check errors
        if not console.has_error():
            test.set_status(Test.SUCCESSFUL)
        else:
            test.set_status(Test.FAILED)
            #TODO get the test output
            
            # Format error message
            testerrors = console.get_message()
            msg = ""
            for key in testerrors:
                for err in testerrors[key]:
                    msg += "%s: %s\n" % (key.upper(), str(err[1]))
            msg = msg.rstrip("\n")
            
            # Save error message
            test.set_error(msg)
        
            # Get last logs
            for host in self._model.get_hosts_list():
                msg = self.get_last_logs(host.get_name())
                test.get_last_logs().append(msg)
         
        # Wait for pending tests to stop
        for thread in self._threads:
            self._trace_info("Waiting for a test thread to stop")
            thread.join(120)
            if thread.is_alive():
                self._trace_error("Cannot stop a thread, we may have errors")
            else:
                self._trace_info("Thread stopped")
            self._threads.remove(thread)
        
        # Check running hosts
        current = list(self._model.running_list())
        i = 0
        while i < len(current):
            if current[i] in running:
                # Host is still running
                running.remove(current[i])
                current.pop(i)
            else:
                # Host started
                msg = "The host '%s' may have started during the test" \
                      % current[i]
                self._trace_info(msg)
                i +=1
        for host in running:
            # Host crashed
            msg = "The host '%s' may have crashed during the test" \
                  % host
            self._trace_warning(msg)
            
    def _launch_local(self, test_name, type_name, config, path, console):
        """ launch a local test """
        
        # check if we have to get stats in /tmp/opensand_tests/stats before
        stats_dst = ''
        try:
            stats_dst = config.get('test_command', 'stats')
        except:
            pass
        
        if stats_dst != '':
            # we need to stop OpenSAND to get statistics in scenario folder
            if not self._stop_platform():
                raise TestError("Test", 
                                "Cannot stop platform to get statistics")
            
            stats = os.path.dirname(path)
            stats = os.path.join(stats, 'scenario_%s/other_test_%s/' %
                                 (test_name, type_name))
            
            try:
                if not os.path.exists(stats_dst):
                    os.mkdir(stats_dst)
                else:
                    shutil.rmtree(stats_dst)
                copytree(stats, stats_dst)
            except KeyboardInterrupt as ex:
                raise
            except Exception as ex:
                raise TestError("Test",
                                "Cannot copy stats directory: %s"
                                % str(ex))

        cmd = ''
        ret = 0
        try:
            cmd = config.get('test_command', 'exec')
            cmd += " " + test_name
            ret = config.getint('test_command', 'return')
        except ConfigParser.Error, err:
            raise TestError("Configuration",
                            "Error when parsing configuration: %s" %
                            (err))
        except KeyboardInterrupt as ex:
            raise
        except Exception as ex:
            raise TestError("Configuration",
                            "Unexpected error when parsing configuration: %s" %
                            str(ex))

        command = os.path.join(self._folder, cmd)
        cmd = shlex.split(command)
        if not os.path.exists('/tmp/opensand_tests'):
            os.mkdir('/tmp/opensand_tests')
        
        try:
            with open('/tmp/opensand_tests/result', 'a') as output:
                output.write("\n")
                process = subprocess.Popen(cmd, close_fds=True,
                                           stdout=output,
                                           stderr=subprocess.STDOUT)
                
                while process.returncode is None:
                    process.poll()
                    time.sleep(1)
                
                if process.returncode is None:
                    process.kill()

                process.wait()
                
                if process.returncode != ret:
                    raise TestError("Local",
                                    "Test returned '%s' instead of '%s'" 
                                    % (process.returncode, ret))
        except KeyboardInterrupt as ex:
            raise
        except TestError as ex:
            raise
        except Exception as ex:
            raise TestError("Local",
                            "Unexpected error when launching command: %s"
                            % str(ex))

        if stats_dst != '':
            # Restart the OpenSAND platform
            if not self._start_platform():
                raise TestError("Test", 
                                "Cannot restart platform after getting statistics")

    def _launch_remote(self, test_name, host_name, config, path, console):
        """ launch the test on a remote host """
        
        # get the host controller
        host_ctrl = None
        found = False
        if not self._workstation and host_name.startswith("WS"):
            # use ST instead => remove WS name and replace name
            host_name = host_name.split("_", 1)[0]
            host_name = host_name.replace("WS", "ST")

        if host_name.startswith("WS"):
            for ctrl in self._ws_ctrl:
                if ctrl.get_name() == host_name:
                    host_ctrl = ctrl
                    break
        else:
            for ctrl in self._controller._hosts:
                if ctrl.get_name() == host_name:
                    host_ctrl = ctrl
                    break

        if host_ctrl is None:
            raise TestError("Configuration", "Cannot find host %s" % host_name)

        # iterate over all host machines
        for machine_name,machine_ctrl in host_ctrl.get_machines().iteritems():
            # ignore PHY machines (they do not have to ping
            if 'phy' in machine_name:
                continue
            # deploy the test files
            # the deploy section has the same format as in deploy.ini file so
            # we can directly use the deploy fonction from hosts
            try:
                machine_ctrl.deploy(config)
            except CommandException as msg:
                raise TestError("Configuration", "Cannot deploy host %s: %s" %
                                (machine_name, msg))
            except KeyboardInterrupt as ex:
                raise
            except Exception as ex:
                raise TestError("Configuration", "Unexpected error when "
                                "deploying host %s: %s" %
                                (machine_name, str(ex)))

            cmd = ''
            wait = False
            ret = 0
            try:
                # TODO give test name in cmd argument !
                cmd = config.get('command', 'exec')
                cmd += " " + test_name
                wait = config.get('command', 'wait')
                if wait.lower() == 'true':
                    wait = True
                else:
                    wait = False
                ret = config.get('command', 'return')
            except ConfigParser.Error, err:
                raise TestError("Configuration",
                                "Error when parsing configuration in %s : %s" %
                                (path, err))
            except KeyboardInterrupt as ex:
                raise
            except Exception as ex:
                raise TestError("Configuration",
                                "Unexpected error when parsing configuration in %s : %s" %
                                (path, err))
                
            # if wait is True we need to wait the host response before launching the
            # next command so we don't need to connect the host in a thread
            if wait:
                self.__connect_host(machine_ctrl, cmd, ret, console)
            else:
                connect = Thread(target=self.__connect_host,
                                 args=(machine_ctrl, cmd, ret, console))
                self._threads.append(connect)
                connect.start()
            
    def __connect_host(self, machine_ctrl, cmd, ret, console):
        """ connect the host and launch the command,
            and exception is raised if the test return is not ret
            and complete the self._error dictionnary """
            
        err = None
        try:
            sock = machine_ctrl.connect_command('TEST')
            if sock is None:
                err = "cannot connect host"
                raise TestError("Configuration", err)
            
            # increase the timeout value because tests could be long
            sock.settimeout(200)
            sock.send("COMMAND %s\n" % cmd)
            result = sock.recv(512).strip()
            console.output(machine_ctrl.get_name().upper(),
                        "Test returns %s on %s, expected is %s" %
                         (result, machine_ctrl.get_name(), ret))
            if result != ret:
                err = "Test returned '%s' instead of '%s'" \
                      % (result, ret)
                raise Exception(err)
            
        except CommandException, msg:
            err = "%s: %s" % (machine_ctrl.get_name(), msg)
        except socket.error, msg:
            err = "%s: %s" % (machine_ctrl.get_name(), str(msg))
        except socket.timeout:
            err = "%s: Timeout" % machine_ctrl.get_name()
        except Exception as ex:
            err = "%s: %s" % (machine_ctrl.get_name(), str(ex))
        finally:
            if sock:
                sock.close()
            if err is not None:
                #console.error(host_ctrl.get_name().upper(), err)
                console.error("Remote", err)

class TestError(Exception):
    """ error during tests """
    def __init__(self, step, descr):
        Exception.__init__(self)
        self.step = step
        self.msg = descr

    def __repr__(self):
        return '\tCONTEXT: %s\tMESSAGE: %s' % (self.step, self.msg)

    def __str__(self):
        return repr(self)


if __name__ == '__main__':
    ### parse options
    # TODO option to get available test types with descriptions
    # TODO option to specify the installed plugins
    opt_parser = OptionParser(formatter=IndentedHelpFormatterWithNL())
    opt_parser.add_option("-v", "--verbose", action="store_true",
                          dest="verbose", default=False,
                          help="Enable verbose mode (print OpenSAND status)")
    opt_parser.add_option("-d", "--debug", action="store_true",
                          dest="debug", default=False,
                          help="Print all the debug information (more "
                          "output than -v)")
    opt_parser.add_option("-w", "--enable_ws", action="store_true",
                          dest="ws", default=False,
                          help="Enable workstations for launching tests "
                               "binaries")
    opt_parser.add_option("-l", "--list", action="store_true",
                          dest="list", default=False,
                          help="List all types of test and tests matching "
                               "with the command line")
    opt_parser.add_option("-i", "--init", dest="first", default=0, type="int",
                          help="ignore tests before this test")
    opt_parser.add_option("-e", "--test", dest="test", default=None,
                          help="launch some tests in particular (regexp)")
    opt_parser.add_option("-y", "--type", dest="type", default=None,
                          help="Launch only one type of test (regexp)")
    opt_parser.add_option("-s", "--service", dest="service",
                          default=SERVICE,
                          help="Listen for OpenSAND entities "\
                               "on the specified service type with format: " \
                               "_name._transport_protocol")
    opt_parser.add_option("-a", "--last_logs", action="store_true",
                          dest="last_logs", default=False,
                          help="Show last logs on host upon failure")
    opt_parser.add_option("-f", "--folder", dest="folder",
                          default='./tests/',
help="specify the root folder for tests configurations\n"
"The root folder should contains the following subfolders and files:\n"
"  base/+-configs/+-test_name/core_HOST.xslt\n"
"                 |-enrich/+-enrich_name/+-core_HOST.xslt\n"
"                                        |-accepts (optional)\n"
"                                        |-ignored (optional)\n"
"                                        |-enrich_name/...(iterative)\n"
"       |-types/test_type/+-order_host/configuration\n"
"                         |-core_HOST.xslt (optional)\n"
"  other/test_type/+-order_host/configuration\n"
"                  |-configs/test_name/core_HOST.xslt (optional)\n"
" and the scripts and configuration for the tests, this can be any files as"
" they will be specified in the configuration files.\n"
" Usually we add a files folder in test_type folder containing them\n"

" with configuration containing the same sections that deploy.ini and a command "
" part for remote hosts:\n"
"  [command]\n"
"  # the command line to execute the test\n"
"  exec={command line}\n"
"  # whether the daemon should get a return code\n"
"    directly or if it should wait for other actions\n"
"  wait=True/False\n"
"  # the code that test script should return\n"
"  return=[0:255]\n"
" and configuration contining the following elements for local tests\n"
"  [itest_command]\n"
"  # the command line to execute the test\n"
"  exec={command line}\n"
"  # the code that test script should return\n"
"  return=[0:255]\n"
"  # the directory to copy statistics on local host\n"
"  stats={dir}\n"

"All the files to copy are stored into the files \n"
"directory, as the path is specified, the files\n"
"directory can be whichever directory you want in\n"
"the test directory\n"
"In core_HOST.xslt HOST can be gw, sat, global, st, stX (with X the ST id)\n"
"order_host is first the order for test launch (00, 01, 02, ...) and host "
"the host on which test should be run among (sat, gw, stX (with X the ST id), "
"wsY_Z (with Y the workstation id and Z its name), test (if test is launched "
"on local host, stat (if test is launched on local host and needs statistics, "
"be careful, stat test stops the platform, no more test should be run after). "
"The enrich folders contains new scenario that will complete base ones. "
"The accept file contains the types that are allowed for this enrichment.")
    (options, args) = opt_parser.parse_args()

    tests = None
    if options.test is not None:
        tests = options.test.split(',')
    
    # Get types
    types = None
    if options.type is not None:
        types = options.type.split(',')
        
    # Initialize the test manager
    lvl = TestManager.QUIET
    if options.debug:
        lvl = TestManager.DEBUG
    elif options.verbose:
        lvl = TestManager.VERBOSE
        
    if lvl == TestManager.QUIET:
        print "Initialization, please wait..."
    
    mgr = TestManager(lvl, options.last_logs)
    code = 0
    
    try:
        # Clean scenarios
        mgr.clean_scenarios()
    
        # Prepare tests scenarios
        if not mgr.prepare_scenarios(folder = options.folder,
                                    regexptest = options.test,
                                    regexptype = options.type):
            raise TestError("Scenarios preparation", "")
        
        # Prepare the OpenSAND platform
        if not mgr.prepare_platform(service = options.service, 
                                   workstation = options.ws,
                                   displayonly = options.list):
            raise TestError("Platform preparation", "")

        # Run tests scenarios
        mgr.run_scenarios(first_scenario=options.first)
    
        if lvl == TestManager.QUIET:
            print "Closure, please wait..."
        
    except KeyboardInterrupt:
        if lvl == TestManager.QUIET:
            print "Interrupted, please wait..."
        code = 1
    except TestError as error:
        code = 1
    except Exception as ex:
        print "\n\n##### TRACEBACK #####"
        traceback.print_tb(sys.exc_info()[2])
        print red("Internal error: %s" % str(ex))
        code = 1

    # Close paltform
    mgr.close_platform()
    sys.exit(code)

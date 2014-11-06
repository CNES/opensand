#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
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
test.py - run the OpenSAND automatic tests
"""

import traceback
import threading
import glob
import sys
import os
import time
import gobject
from optparse import OptionParser, IndentedHelpFormatter
import textwrap
import ConfigParser
import socket
import shlex
import subprocess
import shutil

from opensand_manager_core.opensand_model import Model
from opensand_manager_core.model.event_manager import EventManager
from opensand_manager_core.opensand_controller import Controller
from opensand_manager_core.controller.host import HostController
from opensand_manager_core.loggers.manager_log import ManagerLog
from opensand_manager_core.loggers.levels import MGR_WARNING, MGR_INFO, MGR_DEBUG
from opensand_manager_core.my_exceptions import CommandException
from opensand_manager_core.utils import copytree

# TODO get logs on error
# TODO rewrite this as this is now too long
#      maybe create classes that will simplify the code
SERVICE = "_opensand._tcp"
ENRICH_FOLDER = "enrich"
EVT_TIMEOUT = 10

class Loop(threading.Thread):
    """ the mainloop for service_listener """
    _main_loop = None
    def run(self):
        """ run the loop """
        try:
            gobject.threads_init()
            self._main_loop = gobject.MainLoop()
            self._main_loop.run()
        except KeyboardInterrupt:
            self.close()
            raise

    def close(self):
        """ stop the loop """
        self._main_loop.quit()
        

class EventResponseHandler(threading.Thread):
    """
    Get response events from hosts controllers
    We need this intermediate handler to eliminate parasite events
    """
    def __init__(self, event_manager_response, event_response_tests, model,
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

   

class IndentedHelpFormatterWithNL(IndentedHelpFormatter):
    """ parse '\n' in option parser help
        From Tim Chase via Google group comp.lang.python """
    def format_description(self, description):
        """ format the text description for help message """
        if not description:
            return ""
        desc_width = self.width - self.current_indent
        indent = " "*self.current_indent
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

class Test:
    """ the elements to launch a test """
    def __init__(self):
        self._model = None
        self._controller = None

        ### parse options

# TODO option to get available tess types with descriptions
# TODO option to specify the installed plugins
        opt_parser = OptionParser(formatter=IndentedHelpFormatterWithNL())
        opt_parser.add_option("-v", "--verbose", action="store_true",
                              dest="verbose", default=False,
                              help="enable verbose mode (print OpenSAND status)")
        opt_parser.add_option("-d", "--debug", action="store_true",
                              dest="debug", default=False,
                              help="print all the debug information (more "
                              "output than -v)")
        opt_parser.add_option("-w", "--enable_ws", action="store_true",
                              dest="ws", default=False,
                              help="enable verbose mode (print OpenSAND status)")
        opt_parser.add_option("-t", "--type", dest="type", default=None,
                              help="launch only one type of test")
        opt_parser.add_option("-l", "--test", dest="test", default=None,
                              help="launch one test in particular (use test "
                              "names from the same folder (separated by ',') "
                              "and set the --type option)")
        opt_parser.add_option("-s", "--service", dest="service",
                              default=SERVICE,
                              help="listen for OpenSAND entities "\
                                   "on the specified service type with format: " \
                                   "_name._transport_protocol")
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

        # TODO regex in selt._test and self._type for regen* or transp* for ex
        self._test = None
        self._type = None
        if options.test is not None:
            self._test = options.test.split(',')
        if options.type is not None:
            self._type = options.type.split(',')
        self._folder = options.folder
        self._service = options.service
        self._ws_enabled = options.ws

        # the threads to join when test is finished
        self._threads = []
        # the error returned by a test
        self._error = []
        self._last_error = ""
        # the workstations controllers
        self._ws_ctrl = []

        self._loop = None

        ### create the logger
        self._quiet = True

        # Create the Log View that will only log in standard output
        lvl = MGR_WARNING
        if options.debug:
            lvl = MGR_DEBUG
            self._quiet = False
        elif options.verbose:
            lvl = MGR_INFO
            self._quiet = False
        else:
            print "Initialization please wait..."
        self._log = ManagerLog(lvl, True, False, False, 'PtTest')
        self._event_response_handler = None
        self._event_resp = None

        try:
            self.load()
            self._base = self._model.get_scenario()
            self.run_base()
            self._model.set_scenario(self._base)
            self.run_other()
            if len(self._last_error) > 0:
                raise TestError('Last error: ', str(self._last_error))
            if self._test is not None and len(self._test):
                raise TestError("Configuration", "The following tests were not "
                                "found %s" % self._test)
            if self._type is not None and len(self._type):
                raise TestError("Configuration", "The following types were not "
                                "found %s" % self._type)

        except TestError as err:
            if self._quit:
                print "%s: %s" % (err.step, err.msg)
            else:
                self._log.error(" * %s: %s" % (err.step, err.msg))
            raise
        except KeyboardInterrupt:
            if self._quiet:
                print "Interrupted: please wait..."
            else:
                self._log.info(" * Interrupted: please wait...")
            raise
        except Exception, msg:
            if self._quit:
                print "Internal error:" + str(msg)
            else:
                self._log.error(" * internal error while testing: " + str(msg))
            raise
        finally:
            # reset the service in the test library
            lib = os.path.join(self._folder, '.lib/opensand_tests.py')
            buf = ""
            try:
                with open(lib, 'r') as flib:
                    for line in flib:
                        if line.startswith("TYPE ="):
                            buf += 'TYPE = "%s"\n' % SERVICE
                        else:
                            buf += line
                            with open(lib, 'w') as flib:
                                flib.write(buf)
            except IOError, msg:
                pass

            self.close()


    def load(self):
        """ prepare OpenSAND for the test """
        # create the OpenSAND model
        self._model = Model(self._log)
        # create the OpenSAND controller
        self._controller = Controller(self._model, self._service, self._log,
                                      False)
        self._controller.start()
        self._event_response_test = EventManager("TestEventsManager")
        self._event_response_handler = \
            EventResponseHandler(self._model.get_event_manager_response(),
                                 self._event_response_test,
                                 self._model,
                                 self._log)
        self._event_response_handler.start()
        # Launch the mainloop for service_listener
        self._loop = Loop()
        self._loop.start()
        self._log.info(" * Wait for Model and Controller initialization")
        time.sleep(5)
        self._log.info(" * Consider Model and Controller as initialized")
        if self._ws_enabled:
            self.create_ws_controllers()

    def close(self):
        """ stop the service listener """
        try:
            self.stop_opensand()
        except:
            pass
        self._log.info(" * Stopping threads")
        for thread in self._threads:
            thread.join()
            self._threads.remove(thread)
        self._log.info(" * Threads stopped")
        self._log.info(" * Close workstation controllers")
        for ws_ctrl in self._ws_ctrl:
            ws_ctrl.close()
        self._log.info(" * Workstation controllers closed")
        if self._model is not None:
            self._log.info(" * Close model")
            self._model.close()
            self._log.info(" * Model closed")
        if self._event_response_handler is not None and \
           self._event_response_handler.is_alive():
            self._log.info(" * Join event reponse handler")
            self._event_response_handler.join()
            self._log.info(" * Event reponse handler joined")
        if self._loop is not None:
            self._log.info(" * Close mainloop")
            self._loop.close()
            self._loop.join()
            self._log.info(" * Mainloop stopped")
        if self._controller is not None:
            self._log.info(" * Close controller")
            self._controller.join()
            self._log.error(" * Controller stopped")

    def run_base(self):
        """ launch the tests with base configurations """
        if not self._model.main_hosts_found():
            raise TestError("Initialization", "main hosts not found")
        
        base = os.path.join(self._folder, 'base')
        types_path = os.path.join(base, 'types')
        configs_path = os.path.join(base, 'configs')
        # if the test is launched with the type option
        # check if it is a supported type
        types_init = os.listdir(types_path)
        types = list(types_init)
        for folder in types_init:
            if not os.path.isdir(os.path.join(types_path, folder)):
                types.remove(folder)

        # get type if only some types should be run
        if self._type is not None:
            found = 0
            desired = []
            for folder in types_init:
                if folder in self._type:
                    desired.append(folder)
                    found += 1

            types = desired
            for folder in desired:
                self._type.remove(os.path.basename(folder))
        if len(types) == 0:
            return

        # modify the service in the test library
        lib = os.path.join(self._folder, '.lib/opensand_tests.py')
        buf = ""
        try:
            with open(lib, 'r') as flib:
                for line in flib:
                    if line.startswith("TYPE ="):
                        buf += 'TYPE = "%s"\n' % self._service
                    else:
                        buf += line
            with open(lib, 'w') as flib:
                flib.write(buf)
        except IOError, msg:
            raise TestError("Configuration",
                            "Cannot edit test libraries: %s" % msg)

        test_paths_init = glob.glob(configs_path + '/*')
        test_paths = list(test_paths_init)
        for test in test_paths_init:
            if not os.path.isdir(test):
                test_paths.remove(test)
            if os.path.basename(test) == ENRICH_FOLDER:
                test_paths.remove(test)

        for test_path in test_paths:
            self.stop_opensand()
            self._model.set_scenario(self._base)
            self.run_enrich(test_path, "", types_path, types) 
            
        # stop the platform
        self.stop_opensand()

    def run_enrich(self, test_path, enrich, types_path, types, accepts=[],
                   ignored={}):
        """ run a test for a specific type or this type enriched """

        # create a new list to avoid modifying reference
        accepts = list(accepts)
        ignored = dict(ignored)
        test_name = os.path.basename(test_path)
        if enrich != "":
            pos = enrich.find(ENRICH_FOLDER)
            path = enrich[pos + 1 + len(ENRICH_FOLDER):]
            new = path.split("/")
            for name in new:
                test_name += "_" + name
            accept_path = os.path.join(enrich, "accepts")
            if os.path.exists(accept_path):
                with open(accept_path) as accept_list:
                    for accepted in accept_list:
                        accepts.append(accepted.strip())
            ignored_path = os.path.join(enrich, "ignored")
            if os.path.exists(ignored_path):
                with open(ignored_path) as ignored_list:
                    for ignore in ignored_list:
                        ignore = ignore.strip()
                        info = ignore.split(',')
                        # ignore can containa position in the test name
                        # to check for the specific pattern
                        if len(info) == 1:
                            ignored[ignore] = None
                        else:
                            ignored[info[0]] = info[1]
                            
        # check if test should be ignored
        for ig in ignored:
            if ignored[ig] == None:
                if test_name.find(ig) >= 0:
                    return
            else:
                try:
                    if test_name.split('_')[int(ignored[ig])] == ig:
                        return
                except Exception:
                    pass

        if enrich != "":
            self.new_scenario(enrich, test_name)
        else:
            # get the new configuration from base configuration
            # and create scenario
            self.new_scenario(test_path, test_name)
            
        init_scenario = self._model.get_scenario()

        if self._test is None or test_name in self._test:
            self._log.info(" * Test %s with base configuration" % test_name)
            if self._quiet:
                print "Test configuration \033[1;34m%s\033[0m" % test_name
                sys.stdout.flush()

            if self._test is not None:
                self._test.remove(test_name)
            # start the platform
            self.start_opensand()

            nonbase = []
            # get test_type folders
            test_types = map(lambda x: os.path.join(types_path, x), types)
            for test_type in test_types:
                # check for XSLT files
                orders = glob.glob(test_type + '/*')
                found = False
                for elt in orders:
                    if elt.endswith('.xslt'):
                        found = True
                        break
                if found:
                    # the base configuration should be updated for this
                    # test, do it later
                    nonbase.append(test_type)
                    continue

                if len(accepts) == 0 or os.path.basename(test_type) in accepts:
                    self.launch_test_type(test_type, test_name)

            # now launch tests for non based configuration, stop platform
            # between each run
            for test_type in nonbase:
                self.stop_opensand()

                self._model.set_scenario(init_scenario)
    
                if len(accepts) == 0 or os.path.basename(test_type) in accepts:
                    # update configuration
                    self.new_scenario(test_type, test_name)
                    self.start_opensand()
                    self.launch_test_type(test_type, test_name)
                    
        self.stop_opensand()
            
        # we restart from the test scenario that will be enriched
        self._model.set_scenario(init_scenario)
        if enrich == "":
            base = os.path.join(self._folder, 'base')
            configs_path = os.path.join(base, 'configs')
            enrich = os.path.join(configs_path, ENRICH_FOLDER)
        for folder in glob.glob(enrich + "/*"):
            if os.path.basename(folder).startswith("scenario"):
                continue

            if os.path.isdir(folder):
                self.stop_opensand()
                # we restart from the test scenario that will be enriched
                # for each new path
                self._model.set_scenario(init_scenario)
                self.run_enrich(test_path, folder, types_path, types,
                                accepts, ignored) 


    def run_other(self):
        """ launch the tests for non base configuration """
        if not self._model.main_hosts_found():
            raise TestError("Initialization", "main hosts not found")

        types_path = os.path.join(self._folder, 'other')
        # if the test is launched with the type option
        # check if it is a supported type
        types_init = os.listdir(types_path)
        types = list(types_init)
        for folder in types_init:
            if not os.path.isdir(os.path.join(types_path, folder)):
                types.remove(folder)

        # get type if only some types should be run
        if self._type is not None:
            found = 0
            desired = []
            for folder in types_init:
                if folder in self._type:
                    desired.append(folder)
                    found += 1

            types = desired
            for folder in desired:
                self._type.remove(os.path.basename(folder))
        if len(types) == 0:
            return

        # modify the service in the test library
        lib = os.path.join(self._folder, '.lib/opensand_tests.py')
        buf = ""
        try:
            with open(lib, 'r') as flib:
                for line in flib:
                    if line.startswith("TYPE ="):
                        buf += 'TYPE = "%s"\n' % self._service
                    else:
                        buf += line
            with open(lib, 'w') as flib:
                flib.write(buf)
        except IOError, msg:
            raise TestError("Configuration",
                            "Cannot edit test libraries: %s" % msg)

        # get test_type folders
        test_types = map(lambda x: os.path.join(types_path, x), types)
        test_paths = []
        for test_type in test_types:
            configs_path = os.path.join(test_type, 'configs')
            if os.path.exists(configs_path):
                test_paths = glob.glob(configs_path + '/*')

            self._log.info(" * Test %s" % os.path.basename(test_type))
            if self._quiet:
                print "Test \033[1;34m%s\033[0m" % os.path.basename(test_type)
                sys.stdout.flush()

            for test_path in test_paths:
                test_name = os.path.basename(test_path)
                if self._test is not None:
                    if test_name not in self._test:
                        continue
                    self._test.remove(test_name)
                self.stop_opensand()
                self._model.set_scenario(self._base)
                # get the new configuration from base configuration
                # and create scenario
                self.new_scenario(test_path, test_name)

                # start the platform
                self.start_opensand()

                self.launch_test_type(test_type, os.path.basename(test_path))

            # if there is not configs folder we only have default configuration,
            # so there is not test_path, launch test with base configuration
            if len(test_paths) == 0 and self._test is None:
                self.stop_opensand()
                self._model.set_scenario(self._base)
                # get the new configuration from base configuration
                # and create scenario
                self.new_scenario(test_type, "default")

                # start the platform
                self.start_opensand()

                self.launch_test_type(test_type, "default")

        # stop the platform
        self.stop_opensand()

    def launch_test(self, path, test_name):
        """ initialize the test: load configuration, deploy files
            then contact the distant host in a thread and launch
            the desired command """
        names = os.path.basename(path).split("_", 1)
        if len(names) < 2:
            self._log.info(" * Skip path %s as it has a wrong format" % path)
            return

        host_name = names[1].upper()
        conf_file = os.path.join(path, 'configuration')
        if not os.path.exists(conf_file):
            self._log.debug(" * Skip path %s as it does not contain scenario" %
                           path)
            return
        self._log.info(" * Launch command on %s" % host_name)
        # read the command section in configuration
        config = ConfigParser.SafeConfigParser()
        if len(config.read(conf_file)) == 0:
            raise TestError("Configuration",
                            "Cannot load configuration in %s" % path)

        try:
            config.set('prefix', 'source', self._folder)
        except:
            pass

        # check if this is a local test
        if host_name == "TEST":
            self.launch_local(test_name, config, path)
        else:
            self.launch_remote(test_name, host_name, config, path)
        return True


    def launch_remote(self, test_name, host_name, config, path):
        """ launch the test on a remote host """
        # get the host controller
        host_ctrl = None
        found = False
        if not self._ws_ctrl and host_name.startswith("WS"):
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

        # deploy the test files
        # the deploy section has the same format as in deploy.ini file so
        # we can directly use the deploy fonction from hosts
        try:
            host_ctrl.deploy(config)
        except CommandException as msg:
            raise TestError("Configuration", "Cannot deploy host %s: %s" %
                            (host_name, msg))

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
        # if wait is True we need to wait the host response before launching the
        # next command so we don't need to connect the host in a thread
        if wait:
            self.connect_host(host_ctrl, cmd, ret)
        else:
            connect = threading.Thread(target=self.connect_host,
                                       args=(host_ctrl, cmd, ret))
            self._threads.append(connect)
            connect.start()

    def launch_local(self, test_name, config, path=None):
        """ launch a local test """
        # check if we have to get stats in /tmp/opensand_tests/stats before
        stats_dst = ''
        try:
            stats_dst = config.get('test_command', 'stats')
        except:
            pass

        if stats_dst != '':
            # we need to stop OpenSAND to get statistics in scenario folder
            try:
                self.stop_opensand()
            except:
                raise TestError("Test", "Cannot stop platform to get statistics")
            stats = os.path.dirname(path)
            stats = os.path.join(stats, 'scenario_%s/default/' % test_name)

            if not os.path.exists(stats_dst):
                os.mkdir(stats_dst)
            else:
                shutil.rmtree(stats_dst)
            copytree(stats, stats_dst)

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

        command = os.path.join(self._folder, cmd)
        cmd = shlex.split(command)
        if not os.path.exists('/tmp/opensand_tests'):
            os.mkdir('/tmp/opensand_tests')
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
                self._log.info(" * Test returns %s, expected is %s" %
                               (process.returncode, ret))
                self._error.append("Test returned '%s' instead of '%s'" %
                                   (process.returncode, ret))


    def connect_host(self, host_ctrl, cmd, ret):
        """ connect the host and launch the command,
            and exception is raised if the test return is not ret
            and complete the self._error dictionnary """
        err = None
        try:
            sock = host_ctrl.connect_command('TEST')
            if sock is None:
                err = "%s: cannot connect host" % host_ctrl.get_name()
                raise TestError("Configuration", err)
            # increase the timeout value because tests could be long
            sock.settimeout(200)
            sock.send("COMMAND %s\n" % cmd)
            result = sock.recv(512).strip()
            self._log.info(" * Test returns %s on %s, expected is %s" %
                           (result, host_ctrl.get_name(), ret))
        except CommandException, msg:
            err = "%s: %s" % (host_ctrl.get_name(), msg)
        except socket.error, msg:
            err = "%s: %s" % (host_ctrl.get_name(), str(msg))
        except socket.timeout:
            err = "%s: Timeout" % host_ctrl.get_name()
        except Exception, msg:
            err = "Unknown exception: msg"
        finally:
            if sock:
                sock.close()
            if err is not None:
                if not self._quiet:
                    self._log.error(" * " +  err)
                self._error.append(err)
                return

        if result != ret:
            self._error.append("Test returned '%s' instead of '%s' on %s" %
                               (result, ret, host_ctrl.get_name()))

    def stop_opensand(self):
        """ stop the OpenSAND testbed """
        evt = self._model.get_event_manager()
        evt.set('stop_platform')
        if not self._event_response_test.wait(EVT_TIMEOUT):
            raise TestError("Initialization", "timeout when stopping platform")
        if self._event_response_test.get_type() != "stopped":
            self._event_response_test.clear()
            raise TestError("Initialization", "wrong event response %s when "
                                              "stopping platform" %
                                              self._event_response_test.get_type())
        self._log.info(" * %s event received" % self._event_response_test.get_type())
        if self._event_response_test.get_text() != 'done':
            self._event_response_test.clear()
            raise TestError("Initialization", "cannot stop platform")
        self._event_response_test.clear()
        time.sleep(1)

    def start_opensand(self):
        """ start the OpenSAND testbed """
        evt = self._model.get_event_manager()
        evt.set('start_platform')
        if not self._event_response_test.wait(EVT_TIMEOUT):
            raise TestError("Initialization", "timeout when starting platform")
        if self._event_response_test.get_type() != "started":
            evt = self._event_response_test.get_type()
            self._event_response_test.clear()
            raise TestError("Initialization", "wrong event response %s when "
                                              "starting platform" % evt)
        self._log.info(" * %s event received" % self._event_response_test.get_type())
        if self._event_response_test.get_text() != 'done':
            self._event_response_test.clear()
            raise TestError("Initialization", "cannot start platform")
        self._event_response_test.clear()
        time.sleep(1)

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

# TODO at the moment we can not configure modules !!
    def new_scenario(self, test_path, test_name):
        """ create a new scenario for tests and apply XSLT transformation """
        # get the new configuration from base configuration
        # and create scenario
        path = os.path.join(test_path, 'scenario_%s' % test_name)
        try:
            if os.path.exists(path):
                shutil.rmtree(path)
            os.makedirs(path)
            copytree(self._model.get_scenario(), path)
        except (OSError, IOError), (_, strerror):
            raise TestError("Configuration",
                            "Cannot create scenario %s: %s" % (path, strerror))

        # update the configuration corresponding to the test with XSLT files
        self._model.set_scenario(path)
        for host in self._model.get_hosts_list() + [self._model]:
            conf = host.get_advanced_conf().get_configuration()
            xslt = os.path.join(test_path, 'core_%s.xslt' %
                                host.get_component().lower())
            # specific case for terminals if there is a XSLT for a specific
            # one
            if host.get_component().lower() == 'st':
                for st in os.listdir(path):
                    if st.startswith('st'):
                        st_id = st[2:]
                        if host.get_instance() != st_id:
                            continue
                        xslt_path = os.path.join(test_path, 'core_st%s.xslt' % st_id)
                        if os.path.exists(xslt_path):
                            xslt = xslt_path
            if os.path.exists(xslt):
                try:
                    conf.transform(xslt)
                except Exception, error:
                    raise TestError("Configuration",
                                    "Update Configuration: %s" % error)
                

    def launch_test_type(self, test_path, test_name):
        """ launch tests on a type """
        type_name = os.path.basename(test_path)
        self._log.info(" * Start %s" % type_name)
        # in quiet mode print important output on terminal
        if self._quiet:
            msg = "Start %s " % type_name
            if len(msg) < 40:
                msg = msg + " " * (40 - len(msg))
            print msg,
            sys.stdout.flush()
        self._error = []
        # wait to be sure the plateform is started
        time.sleep(2)
        # get order_host folders
        orders = glob.glob(test_path + '/*')
        # remove common folders from list (not mandatory)
        if os.path.join(test_path, 'files') in orders:
            orders.remove(os.path.join(test_path, 'files'))
        for path in orders:
            if path.startswith('scenario'):
                orders.remove(path)
        orders.sort()
        launched = False
        for host in orders:
            if not os.path.isdir(host):
                continue
            try:
                # check that we effectively lauched a test
                launched = True
                if self.launch_test(host, test_name):
                    # wait for test to initialize or stop on host
                    time.sleep(0.5)
            except Exception, msg:
                self._error.append(str(msg))
                self._last_error = str(msg)
                break
            if len(self._error) > 0:
                self._last_error = self._error[len(self._error) - 1]
                break
        for thread in self._threads:
            # wait for pending tests to stop
            self._log.info(" * waiting for a test thread to stop")
            thread.join(120)
            if thread.is_alive():
                self._log.error(" * cannot stop a thread, we may have "
                                "errors")
            else:
                self._log.info(" * thread stopped")
            self._threads.remove(thread)
        if len(self._error) > 0:
            #TODO get the test output
            # in quiet mode print important output on terminal
            if self._quiet:
                print "    \033[91mERROR\033[0m " + str(self._error)
                sys.stdout.flush()
            else:
                self._log.error(" * Test %s failed: %s" %
                                (type_name, str(self._error)))
            self._last_error = self._error[len(self._error) - 1]
            # continue on other tests
        elif not launched:
            # in quiet mode print important output on terminal
            if self._quiet:
                print "    \033[91mERROR\033[0m no test launched for %s" % \
                      type_name
                sys.stdout.flush()
            else:
                self._log.error(" * No test launched for %s" % type_name)
            self._last_error = "No test launched for %s" %  type_name
            # continue on other tests
        else:
            self._log.info(" * Test %s successful" % type_name)
            # in quiet mode print important output on terminal
            if self._quiet:
                print "    \033[92mSUCCESS\033[0m"
                sys.stdout.flush()


class TestError(Exception):
    """ error during tests """
    def __init__(self, step, descr):
        Exception.__init__(self)
        self.step = step
        self.msg = descr

    def __repr__(self):
        return '\tCONTEXT: %s\n\tMESSAGE: %s' % (self.step, self.msg)

    def __str__(self):
        return repr(self)



if __name__ == '__main__':
    try:
        TEST = Test()
    except KeyboardInterrupt:
        sys.exit(1)
    except Exception, error:
        print "\n\n##### TRACEBACK #####"
        traceback.print_tb(sys.exc_info()[2])
        print "\033[91mError: %s \033[0m" % str(error)
        sys.exit(1)

    print "All tests successfull"
    sys.exit(0)

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

# TODO get logs on error
# TODO rewrite this as this is now too long
#      maybe create classes that will simplify the code
ENRICH_FOLDER = "enrich"


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

class Test(ShellManager):
    """ the elements to launch a test """
    def __init__(self):
        ShellManager.__init__(self)

        ### parse options
# TODO option to get available test types with descriptions
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
                              help="enable workstations for launching tests "
                                   "binaries")
        opt_parser.add_option("-t", "--type", dest="type", default=None,
                              help="launch only one type of test")
        opt_parser.add_option("-l", "--test", dest="test", default=None,
                              help="launch one test in particular (use test "
                              "names from the same folder (separated by ',') "
                              "and set the --type option)")
        opt_parser.add_option("-r", "--regexp", dest="regexp", default=None,
                              help="launch all test that contain this regexp" 
                              "in particular (use test names from the same "
                              "folder and set the --type option)")
        opt_parser.add_option("-s", "--service", dest="service",
                              default=SERVICE,
                              help="listen for OpenSAND entities "\
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

        # TODO regex in selt._test and self._type for regen* or transp* for ex
        self._test = None
        self._regexp = None
        self._type = None
        if options.test is not None:
            self._test = options.test.split(',')
        if options.regexp is not None:
            self._regexp = options.regexp
        if options.type is not None:
            self._type = options.type.split(',')
        self._folder = options.folder

        self._service = options.service
        self._ws_enabled = options.ws

        # the threads to join when test is finished
        self._threads = []
        # the error returned by a test
        self._error = {}
        self._last_error = ""
        self._show_last_logs = options.last_logs
        self._stopped = True
        self._result_ping = {}
        self._result_iperf = {}
        self._result_ping_high = {}

        self._quiet = True
        lvl = MGR_WARNING
        if options.debug:
            lvl = MGR_DEBUG
            self._quiet = False
        elif options.verbose:
            lvl = MGR_INFO
            self._quiet = False
        else:
            print "Initialization please wait..."

        try:
            self._frontend = BaseFrontend()
            self.clean_files()
            self.load(log_level=lvl, service=options.service,
                      with_ws=options.ws,
                      remote_logs=True,
                      frontend=self._frontend)
            self.stop_opensand()
            self._base = self._model.get_scenario()
            self.run_base()
            self._model.set_scenario(self._base)
            self.run_other()
            if len(self._last_error) > 0:
                raise TestError('Last error: ', self._last_error)
            if self._test is not None and len(self._test):
                raise TestError("Configuration", "The following tests were not "
                                "found %s" % self._test)
            if self._regexp is not None and self._regexp == "":
                raise TestError("Configuration", "The following types were not "
                                "found %s" % self._regexp)
            if self._type is not None and len(self._type):
                raise TestError("Configuration", "The following types were not "
                                "found %s" % self._type)

        except TestError as err:
            if self._quiet:
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
            if self._quiet:
                print "Internal error: " + str(msg)
            else:
                self._log.error(" * internal error while testing: " + str(msg))
            raise
        else:
            if self._quiet:
                print green("All tests are successfull", True)
            else:
                self._log.info(" * All tests successfull")
        finally:
           
            if self._result_ping or self._result_iperf or self._result_ping_high:
                print "+-----------|-----------|-----------|-----------+"
                line = "| {:>9} | {:>9} | {:>9} | {:>9} |"
                print line.format("TEST", green(" SUCCESS "), red("  ERROR  "),
                                 "% error")
                if self._result_ping:
                    print "+-----------|-----------|-----------|-----------+"
                    print "|                     ping                      |"
                    print "+-----------+-----------+-----------+-----------|"
                    for test in self._result_ping.keys():
                        line = "| {:>9} | {:>9} | {:>9} | {:>9} |"
                        percent = int(float(self._result_ping[test][1]) / \
                                (self._result_ping[test][0] +
                                 self._result_ping[test][1]) * 100)
                        if percent == 100:
                            percent = red("     %s " % percent )
                        elif percent > 75:
                            percent = red("      %s " % percent )
                        elif percent < 10 :
                            percent = green("       %s " % percent)
                        else:
                            percent = str(percent)
                        print line.format(test, self._result_ping[test][0],
                                          self._result_ping[test][1], 
                                          percent)
                if self._result_iperf:
                    print "+-----------|-----------|-----------|-----------+"
                    print "|                     iperf                     |"
                    print "+-----------+-----------+-----------+-----------|"
                    for test in self._result_iperf.keys():
                        line = "| {:>9} | {:>9} | {:>9} | {:>9} |"
                        percent = int(float(self._result_iperf[test][1]) / \
                                (self._result_iperf[test][0] +
                                 self._result_iperf[test][1]) * 100)
                        if percent == 100:
                            percent = red("     %s " % percent )
                        elif percent > 75:
                            percent = red("      %s " % percent )
                        elif percent < 10 :
                            percent = green("       %s " % percent)
                        else:
                            percent = str(percent)
                        print line.format(test, self._result_iperf[test][0],
                                          self._result_iperf[test][1], 
                                          percent)
                if self._result_ping_high:
                    print "+-----------|-----------|-----------|-----------+"
                    print "|                    ping_high                  |"
                    print "+-----------+-----------+-----------+-----------|"
                    for test in self._result_ping_high.keys():
                        line = "| {:>9} | {:>9} | {:>9} | {:>9} |"
                        percent = int(float(self._result_ping_high[test][1]) / \
                                (self._result_ping_high[test][0] +
                                 self._result_ping_high[test][1]) * 100)
                        if percent == 100:
                            percent = red("     %s " % percent )
                        elif percent > 75:
                            percent = red("      %s " % percent )
                        elif percent < 10 :
                            percent = green("       %s " % percent)
                        else:
                            percent = str(percent)
                        print line.format(test, self._result_ping_high[test][0],
                                          self._result_ping_high[test][1], 
                                          percent)
                print "+-----------+-----------+-----------+"

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
            except IOError:
                pass
            self.close()

    def close(self):
        """ stop the service listener """
        try:
            self.stop_opensand()
        except:
            # if we get a timeout here, we will block because
            # we will close model, and thus quit event_mgr_reponse
            # however, stop process calls it but everything
            # will be closed
            pass
        self._log.info(" * Stopping threads")
        for thread in self._threads:
            thread.join()
            self._threads.remove(thread)
        self._log.info(" * Threads stopped")
        ShellManager.close(self)
        
    def clean_files(self):
        """ clean local files """
        try:
            os.remove('/tmp/opensand_tests/last_errors')
            os.remove('/tmp/opensand_tests/result')
        except:
            pass

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
            test_name = os.path.basename(test_path)
            test_exist = False
            if self._test is not None:
                for name in self._test:
                    if name.startswith(test_name):
                        test_exist = True
            if self._regexp is not None:
                if re.search(self._regexp, test_name) is not None:
                    test_exist = True
            if (self._test is None and self._regexp is None) or test_exist:
                self._model.set_scenario(self._base)
                # reset run values
                self._model.set_run("")
                self.run_enrich(test_path, "", types_path, types)
 
        time.sleep(1)

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
                print "Test configuration %s" % blue(test_name, True)
                sys.stdout.flush()

            if self._test is not None:
                self._test.remove(test_name)
            # start the platform
            self._model.set_run("base_tests")
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
                    self._model.set_run("non_base_test_" +
                                        os.path.basename(test_type))
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
                print "Test %s" % blue(os.path.basename(test_type), True)
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

                self._model.set_run("other_test_" +
                                    os.path.basename(test_type))
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

                self._model.set_run("other_test_" +
                                    os.path.basename(test_type))
                # start the platform
                self.start_opensand()

                self.launch_test_type(test_type, "default")

        # stop the platform
        self.stop_opensand()

    def launch_test(self, path, test_name, type_name):
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
            self.launch_local(test_name, type_name, config, path)
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

    def launch_local(self, test_name, type_name, config, path=None):
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
            stats = os.path.join(stats, 'scenario_%s/other_test_%s/' %
                                 (test_name, type_name))

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
                self._error["Local"] = ("Test returned '%s' instead of '%s'" %
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
                self._error[host_ctrl.get_name().upper()] = err
                return

        if result != ret:
            self._error[host_ctrl.get_name().upper()] = \
                    "Test returned '%s' instead of '%s'" % (result, ret)

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
                
        #Topology
        conf = self._model.get_topology().get_configuration()
        xslt = os.path.join(test_path, 'topology.xslt')
        # specific case for terminals if there is a XSLT for a specific
        # one
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
            print "{:<40} ".format(msg),
            sys.stdout.flush()
        self._error = {}
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
                if self.launch_test(host, test_name, type_name):
                    # wait for test to initialize or stop on host
                    time.sleep(0.5)
            except Exception, msg:
                self._error["Test routine"] = str(msg)
                self._last_error = str(msg)
                break
            if len(self._error) > 0:
                err = ""
                for host in self._error:
                    err += "%s: %s, " % (host.upper(), str(self._error[host]))
                err = err.rstrip(", ")
                self._last_error = err
                # get last logs
                last_logs = ""
                for host in self._model.get_hosts_list():
                    last_logs += self.get_last_logs(host.get_name())
                if self._show_last_logs:
                    print
                    print last_logs
                else:
                    if not os.path.exists('/tmp/opensand_tests'):
                        os.mkdir('/tmp/opensand_tests')
                    with open('/tmp/opensand_tests/last_errors', 'a') as output:
                        output.write(yellow("Results for configuration %s, test %s:\n" %
                                    (test_name, type_name), True))
                        output.write(last_logs)
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
        success = False
        if len(self._error) > 0:
            #TODO get the test output
            # in quiet mode print important output on terminal
            err = ""
            for host in self._error:
                err += "%s: %s, " % (host, str(self._error[host]))
            err = err.rstrip(", ")
            if self._quiet:
                print "{:>7} {:}".format(red("ERROR"), err)
                sys.stdout.flush()
            else:
                self._log.error(" * Test %s failed: %s" %
                                (type_name, err))
            self._last_error = err
            # continue on other tests
        elif not launched:
            # in quiet mode print important output on terminal
            if self._quiet:
                print "{:>7} no test launched for {:}".format(red("ERROR"),
                                                              type_name)
                sys.stdout.flush()
            else:
                self._log.error(" * No test launched for %s" % type_name)
            self._last_error = "No test launched for %s" %  type_name
            # continue on other tests
        else:
            success = True
            self._log.info(" * Test %s successful" % type_name)
            # in quiet mode print important output on terminal
            if self._quiet:
                print "{:>7}".format(green("SUCCESS"))
                sys.stdout.flush()
            
        test_restult = {}
        values = test_name.split("_")
        if len(values) >= 4:
            values[2] = values[2] + "_" + values[3]
            del values[3]
            
            for val in values:
                if val not in self._result_ping and type_name == "ping":
                    self._result_ping[val] = [int(success), int(not success)]
                elif val not in self._result_iperf and type_name == "iperf":
                    self._result_iperf[val] = [int(success), int(not success)]
                elif val not in self._result_ping_high and type_name == "ping_high":
                    self._result_ping_high[val] = [int(success), int(not success)]
                
                else:
                    if type_name == "ping":
                        self._result_ping[val] = [self._result_ping[val][0] + int(success),
                        self._result_ping[val][1] + int(not success)]
                    elif type_name == "iperf":
                        self._result_iperf[val] = [self._result_iperf[val][0] + int(success),
                        self._result_iperf[val][1] + int(not success)]
                    elif type_name == "ping_high":
                        self._result_ping_high[val] = [self._result_ping_high[val][0] + int(success),
                        self._result_ping_high[val][1] + int(not success)]
                
                    
                
        if not self._stopped and not self._model.all_running():
            # need to check stopped because for local tests we may stop the
            # plateform
            msg = "Some hosts may have crashed, still running: %s" % \
                  self._model.running_list()
            self._log.warning(" * %s" % msg)
            if self._quiet:
                print red(msg)
                sys.stdout.flush()
            self._last_error = msg


    def start_opensand(self):
        """ override start function """
        self._stopped = False
        ShellManager.start_opensand(self)

    def stop_opensand(self):
        """ override stop function """
        self._stopped = True
        ShellManager.stop_opensand(self)

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
    try:
        TEST = Test()
    except KeyboardInterrupt:
        sys.exit(1)
    except Exception, error:
        print "\n\n##### TRACEBACK #####"
        traceback.print_tb(sys.exc_info()[2])
        print red("Error: %s" % str(error))
        sys.exit(1)

    sys.exit(0)

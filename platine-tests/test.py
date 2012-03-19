#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
test.py - run the Platine automatic tests
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

from platine_manager_core.platine_model import Model
from platine_manager_core.platine_controller import Controller
from platine_manager_core.controller.host import HostController
from platine_manager_core.loggers.manager_log import ManagerLog
from platine_manager_core.my_exceptions import CommandException

# TODO fonction pour récupérer les logs sur erreur
# TODO fonction pour nettoyer les scénarios
SERVICE = "_platine._tcp"

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

        opt_parser = OptionParser(formatter=IndentedHelpFormatterWithNL())
        opt_parser.add_option("-v", "--verbose", action="store_true",
                              dest="verbose", default=False,
                              help="enable verbose mode (print Platine status)")
        opt_parser.add_option("-d", "--debug", action="store_true",
                              dest="debug", default=False,
                              help="print all the debug information (more "
                              "output than -v)")
        opt_parser.add_option("-t", "--type", dest="type", default=None,
                              help="launch only one type of test")
        opt_parser.add_option("-s", "--service", dest="service",
                              default=SERVICE,
                              help="listen for Platine entities "\
                                   "on the specified service type with format: " \
                                   "_name._transport_protocol")
        opt_parser.add_option("-f", "--folder", dest="folder",
                              default='./tests/',
help="specify the root folder for tests configurations\n"
"The root folder should contains the following subfolders and files:\n"
"  test_type/test_name/+-order_host/configuration\n"
"                      |-scenario/+-core_global.conf\n"
"                                   (optionnal)\n"
"                                 |-host/core.conf\n"
"                                   (optionnal)\n"
" and the scripts and configuration for the tests, this can be any files as "
"they will be specified in the configuration files\n"

"with configuration containing the same sections that deploy.ini and a command "
"part:\n"
"  [command]\n"
"  # the command line to execute the test\n"
"  exec={command line}\n"
"  # whether the daemon should get a return code\n"
"    directly or if it should wait for other actions\n"
"  wait=True/False\n"
"  # the code that test script should return\n"
"  return=[0:255]\n"
"All the files to copy are stocked into the files \n"
"directory, as the path is specified, the files\n"
"directory can be whichever directory you want in\n"
"the test directory")
        (options, args) = opt_parser.parse_args()

        self._type = options.type
        self._folder = options.folder
        self._service = options.service

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
        lvl = 'error'
        if options.debug:
            lvl = 'debug'
            self._quiet = False
        elif options.verbose:
            lvl = 'info'
            self._quiet = False
        else:
            print "Initialization please wait..."
        self._log = ManagerLog(lvl, True, False, False, 'PtTest')

        try:
            self.load()
            self.run()
            if len(self._last_error) > 0:
                raise TestError('Last error: ', str(self._last_error))
        except TestError as err:
            self._log.error("%s: %s" % (err.step, err.msg))
            raise
        except KeyboardInterrupt:
            if self._quiet:
                print "Interrupted: please wait..."
            self._log.info("Interrupted: please wait...")
            self.close()
            raise
        except Exception, msg:
            self._log.error("internal error while testing: " + str(msg))
            raise
        finally:
            self.close()


    def load(self):
        """ prepare Platine for the test """
        # create the Platine model
        self._model = Model(self._log)
        # create the Platine controller
        self._controller = Controller(self._model, self._service, self._log,
                                      False)
        self._controller.start()
        # Launch the mainloop for service_listener
        self._loop = Loop()
        self._loop.start()
        self._log.info(" * Wait for Model and Controller initialization")
        time.sleep(5)
        self._log.info(" * Consider Model and Controller as initialized")
        self.create_ws_controllers()

    def close(self):
        """ stop the service listener """
        for thread in self._threads:
            thread.join(10)
            self._threads.remove(thread)
        try:
            self.stop_platine()
        except:
            pass
        for ws_ctrl in self._ws_ctrl:
            ws_ctrl.close()
        if self._model is not None:
            self._model.close()
        if self._loop is not None:
            self._loop.close()
            self._loop.join()
        if self._controller is not None:
            self._controller.join()

    def run(self):
        """ launch the tests """
        if not self._model.main_hosts_found():
            raise TestError("Initialization", "main hosts not found")

#TODO faire une matrice des tests avec résultat dans un fichier
#TODO on s'arrete sur erreur: OK ?
        # if the tess is launched with the type option
        # check if it is a supported type
        types_init = os.listdir(self._folder)
        types = list(types_init)
        for folder in types_init:
            if folder.startswith('.'):
                types.remove(folder)
            elif not os.path.isdir(os.path.join(self._folder, folder)):
                types.remove(folder)

        if self._type is not None and self._type not in types:
            raise TestError("Initialization", "test type '%s' is not available,"
                            " supported values are %s" % (self._type, types))
        # get test_type folders
        test_types = glob.glob(self._folder + '/*')
        for test_type in test_types:
            # if the test is launched with the type option
            # only run the corresponding tests
            if self._type is not None and \
               self._type != os.path.basename(test_type):
                continue

            self._log.info(" * Enter %s tests" % os.path.basename(test_type))
            # get test_name folders
            test_names = glob.glob(test_type + '/*')
            for test_name in test_names:
                self.stop_platine()
                if not os.path.exists(os.path.join(test_name, 'scenario')):
                    # skip folders that does not contain a scenario directory
                    self._log.debug("skip folder %s as it does not contain a "
                                    "scenario subfolder" %
                                    os.path.basename(test_name))
                    continue
                self._log.info(" * Start test %s" % os.path.basename(test_name))
                # in quiet mode print important output on terminal
                if self._quiet:
                    print "Start test %s" % os.path.basename(test_name),
                    sys.stdout.flush()
                self._model.set_scenario(os.path.join(test_name, 'scenario'))
                self._error = []
                # start the platform
                self.start_platine()
                # get order_host folders
                orders = glob.glob(test_name + '/*')
                if os.path.join(test_name, 'files') in orders:
                    orders.remove(os.path.join(test_name, 'files'))
                if os.path.join(test_name, 'scenario') in orders:
                    orders.remove(os.path.join(test_name, 'scenario'))
                orders.sort()
                for host in orders:
                    if not os.path.isdir(host):
                        continue
                    try:
                        self.launch_test(host)
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
                    self._log.info("waiting for a test thread to stop")
                    thread.join(120)
                    self._log.info("thread stopped")
                    self._threads.remove(thread)
                if len(self._error) > 0:
                    #TODO get the test output
                    # in quiet mode print important output on terminal
                    if self._quiet:
                        print "    \033[91mERROR\033[0m"
                        sys.stdout.flush()
                    # continue on other tests
                    self._log.error(" * Test %s failed: %s" %
                                    (os.path.basename(test_name),
                                     str(self._error)))
                    self._last_error = self._error[len(self._error) - 1]
                else:
                    self._log.info(" * Test %s successful" %
                                   os.path.basename(test_name))
                    # in quiet mode print important output on terminal
                    if self._quiet:
                        print "    \033[92mSUCCESS\033[0m"
                        sys.stdout.flush()

    def launch_test(self, path):
        """ initialize the test: load configuration, deploy files
            then contact the distant host in a thread and launch
            the desired command """
        names = os.path.basename(path).split("_", 1)
        if len(names) < 1:
            raise TestError("Configuration", "wrong path: %s "
                            "cannot find host name" % path)

        host_name = names[1].upper()
        self._log.info(" * Launch command on %s" % host_name)
        conf_file = os.path.join(path, 'configuration')
        # read the command section in configuration
        config = ConfigParser.SafeConfigParser()
        if len(config.read(conf_file)) == 0:
            raise TestError("Configuration",
                            "Cannot load configuration in %s" % path)

        try:
            config.set('prefix', 'source', self._folder)
        except:
            pass

        # get the host controller
        host_ctrl = None
        if host_name.startswith("WS"):
            for host_ctrl in self._ws_ctrl:
                if host_ctrl.get_name() == host_name:
                    break
        else:
            for host_ctrl in self._controller._hosts:
                if host_ctrl.get_name() == host_name:
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
            cmd = config.get('command', 'exec')
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
            sock.settimeout(120)
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
            err = msg
        finally:
            if sock:
                sock.close()
            if err is not None:
            	self._log.error(err)
                self._error.append(err)
                return

        if result != ret:
            self._error.append("Test returned '%s' instead of '%s' on %s" %
                               (result, ret, host_ctrl.get_name()))

    def stop_platine(self):
        """ stop the Platine testbed """
        evt = self._model.get_event_manager()
        resp = self._model.get_event_manager_response()
        evt.set('stop_platform')
        resp.wait(None)
        if resp.get_type() != "resp_stop_platform":
            resp.clear()
            raise TestError("Initialization", "wrong event response %s when "
                                              "stopping platform" %
                                              resp.get_type())
        if resp.get_text() != 'done':
            resp.clear()
            raise TestError("Initialization", "cannot stop platform")
        resp.clear()
        time.sleep(4)

    def start_platine(self):
        """ start the Platine testbed """
        evt = self._model.get_event_manager()
        resp = self._model.get_event_manager_response()
        evt.set('start_platform')
        resp.wait(None)
        if resp.get_type() != "resp_start_platform":
            evt = resp.get_type()
            resp.clear()
            raise TestError("Initialization", "wrong event response %s when "
                                              "starting platform" % evt)
        if resp.get_text() != 'done':
            resp.clear()
            raise TestError("Initialization", "cannot start platform")
        resp.clear()
        time.sleep(4)

        for host in self._model.get_hosts_list():
            if not host.get_state():
                raise TestError("Initialization", "%s did not start" %
                                host.get_name())

    def create_ws_controllers(self):
        """ create controllers for WS because the manager
            controller does not handle it """
        for ws_model in self._model.get_workstations_list():
            new_ws = HostController(ws_model, self._log)
            self._ws_ctrl.append(new_ws)


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

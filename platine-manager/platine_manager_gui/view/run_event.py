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
run_event.py - the events on the run view
"""

import gobject
import socket

from platine_manager_core.my_exceptions import RunException
from platine_manager_gui.view.run_view import RunView
from platine_manager_gui.view.event_handler import EventReponseHandler

INIT_ITER = 4

class RunEvent(RunView):
    """ Events for the run tab """
    def __init__(self, parent, model, dev_mode, manager_log):
        try:
            RunView.__init__(self, parent, model, manager_log)
        except RunException:
            raise

        #TODO read that in configuration
        event_port = 2612
        error_port = 2611

        self._event_manager = self._model.get_event_manager()
        self._event_response_handler = EventReponseHandler(self,
                            self._model.get_event_manager_response(),
                            self._log)

        self._event_socket = None
        self._error_socket = None
        self._event_id = None
        self._error_id = None
        self._first_refresh = True
        self._refresh_iter = 0
        # at beginning check platform state immediatly, then every 2 seconds
        self.on_timer_status()
        self._timeout_id = gobject.timeout_add(2000, self.on_timer_status)

        self._dev_mode = False

        # start event response handler
        self._event_response_handler.start()

        # Initialize socket for error and event controllers
        try:
            self._event_socket = socket.socket(socket.AF_INET,
                                               socket.SOCK_DGRAM)
            self._event_socket.setsockopt(socket.SOL_SOCKET,
                                          socket.SO_REUSEADDR, 1)
            self._event_socket.bind(('', event_port))
            self._event_id = gobject.io_add_watch(self._event_socket,
                                                  gobject.IO_IN,
                                                  self.on_rcv_event)
        except socket.error , (errno, strerror) :
            self._log.error("error on event controller socket : " + strerror)
            raise
        try :
            self._error_socket = socket.socket(socket.AF_INET,
                                               socket.SOCK_DGRAM)
            self._error_socket.setsockopt(socket.SOL_SOCKET,
                                          socket.SO_REUSEADDR, 1)
            self._error_socket.bind(('', error_port))
            self._error_id = gobject.io_add_watch(self._error_socket,
                                                  gobject.IO_IN,
                                                  self.on_rcv_error)
        except socket.error , (errno, strerror) :
            self._log.error("error on error controller socket : " + strerror)
            raise

        if(dev_mode):
            self._ui.get_widget('dev_mode').set_active(True)
        else:
            # do not show deploy button
            gobject.idle_add(self.hide_deploy_button,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)

    def close(self):
        """ 'close' signal handler """
        self._log.debug("Run Event: close")
        if self._timeout_id != None :
            gobject.source_remove(self._timeout_id)
            self._timeout_id = None

        if self._event_socket != None :
            self._log.debug("Run Event: close event socket")
            gobject.source_remove(self._event_id)
            self._event_socket.close()

        if self._error_socket != None :
            self._log.debug("Run Event: close error socket")
            gobject.source_remove(self._error_id)
            self._error_socket.close()

        if self._event_response_handler.is_alive():
            self._log.debug("Run Event: join response event handler")
            self._event_response_handler.join()
            self._log.debug("Run Event: response event handler joined")

        self._log.debug("Run Event: closed")


    def activate(self, val):
        """ 'activate' signal handler """
        if self._first_refresh:
            return
        if val == False and self._timeout_id != None :
            gobject.source_remove(self._timeout_id)
            self._timeout_id = None
        else :
            # refresh immediatly then periodically
            self.on_timer_status()
            self._timeout_id = gobject.timeout_add(1000, self.on_timer_status)

    def on_rcv_event(self, source, condition):
        """ event socket callback """
        (data, address) = self._event_socket.recvfrom(255)
        info = self.parse_rcv_event(data)
        if info != 0:
            gobject.idle_add(self.show_platine_event, info)
        else:
            gobject.idle_add(self.show_platine_error,
                             "event parsing failed...", 'orange')
        # if this callback return True it will be called again
        return True

    def parse_rcv_event(self, data):
        """ parse a message received on the event socket """
        values = map(str.strip, str(data).split(','))
        # get each value in the event and remove spaces before or
        # after the string
        if len(values) == 7:
            FRSFrame = values[0].lstrip()
            FSM      = values[1].lstrip()
            appli    = values[2].lstrip()
            category = values[3].lstrip()
            type     = values[4].lstrip()
            id       = values[5].lstrip()
            state    = values[6].lstrip()
        elif len(values) == 6:
            FRSFrame = values[0].lstrip()
            FSM      = values[1].lstrip()
            appli    = values[2].lstrip()
            category = values[3].lstrip()
            type     = values[4].lstrip()
            state    = values[5].lstrip()
        else:
            return 0

        # appli format is "application_id" with id = 0 if there is only
        # one possible instance
        name = appli.strip('_0')
        name = name.replace('_', '')
        event = ''

        if type == 'Component_state':
            # state format is "state = id (Unit : N/A)"
            state = state.split()
            if state[2].isdigit() is not True:
                gobject.idle_add(self.show_platine_error,
                                 "can not get component state", 'orange')
                return 0
            if state[2] == '0':
                event = 'Starting'
            elif state[2] == '1':
                event = 'Initializing'
            elif state[2] == '2':
                event = 'running'
            elif state[2] == '3':
                event = 'Terminating'
            else:
                gobject.idle_add(self.show_platine_error,
                                 "bad component state value " + state[2],
                                        'orange')
                return 0
        elif type.startswith('Initialisation_ref'):
            event = 'Initializing'
        elif type == 'Login_received':
            # id format is "StId = id"
            id = id.split()
            if id[2].isdigit() is not True:
                gobject.idle_add(self.show_platine_error,
                                 "can not get component id", 'orange')
                return 0
            event = 'Login Request received for ST' + id[2]
        elif type == 'Login_response':
            # id format is "StId = id"
            id = id.split()
            if id[2].isdigit() is not True:
                gobject.idle_add(self.show_platine_error,
                                 "can not get component id", 'orange')
                return 0
            event = 'Login Response sent to ST' + id[2]
        elif type == 'Login_sent':
            event = 'Login Request sent'
        elif type == 'Login_complete':
            event = 'Login Complete'
        else:
            gobject.idle_add(self.show_platine_error,
                             "unknown event: " + str(data), 'orange')
            return 0

        info = name + ": " + event
        return info

    def on_rcv_error(self, source, condition):
        """ error socket callback """
        (data, address) = self._error_socket.recvfrom(255)
        (error, color) = self.parse_rcv_error(data)
        if error != 0:
            gobject.idle_add(self.show_platine_error, error, color)
        else:
            gobject.idle_add(self.show_platine_error,
                             "error parsing failed...", 'orange')
        # if this callback return True it will be called again
        return True

    def parse_rcv_error(self, data):
        """ parse a message received on the error socket """
        values = map(str.strip, str(data).split(','))
        # get each value in the error and remove spaces before or
        # after the string
        if len(values) == 7:
            FRSFrame = values[0].lstrip()
            FSM      = values[1].lstrip()
            appli    = values[2].lstrip()
            category = values[3].lstrip()
            type     = values[4].lstrip()
            cause    = values[5].lstrip()
            error    = values[6].lstrip()
        elif len(values) == 6:
            FRSFrame = values[0].lstrip()
            FSM      = values[1].lstrip()
            appli    = values[2].lstrip()
            category = values[3].lstrip()
            type     = values[4].lstrip()
            state    = values[5].lstrip()
        else:
            return (0, 0)

        # appli format is "application_id" with id = 0 if there is only
        # one possible instance
        name = appli.strip('_0')
        name = name.replace('_', '')
        error = ''
        color = None

        # category format is Category
        categories = category.split()
        if len(categories) < 1:
            gobject.idle_add(self.show_platine_error,
                             "unable to read category", 'orange')
            return (0, 0)
        category = categories[1].strip('()')
        if category == 'CRITICAL':
            color = 'red'
        elif category == 'MINOR':
            color = 'orange'

        if type.startswith('Initialisation_ref'):
            error = 'Initializing'
        elif type.startswith('Component_initialisation'):
            error = 'Initialization failed'
        else:
            gobject.idle_add(self.show_platine_error,
                             "unknown event: " + str(data), 'orange')
            return (0, 0)

        info = name + ': ' + error
        return (info, color)

    def on_deploy_platine_button_clicked(self, source=None, event=None):
        """ 'clicked' event on deploy Platine button """
        # disable the buttons
        self.disable_start_button(True)
        self.disable_deploy_button(True)

        # tell the hosts controller to deploy Platine on all hosts
        # (the installation will be finished when we will receive a
        # 'resp_deploy_platform' event, the button will be enabled
        # there)
        self._event_manager.set('deploy_platform')

    def on_start_platine_button_clicked(self, source=None, event=None):
        """ 'clicked' event on start Platine button """
        # start or stop the applications ?
        if not self._model.is_running():
            # start the applications

            # disable the buttons
            self.disable_start_button(True)
            self.disable_deploy_button(True)

            # retrieve the current scenario and start
            self.set_run_id()

            # tell the hosts controller to start Platine on all hosts
            # (startup will be finished when we will receive a
            # 'resp_start_platform' event, the button will be enabled there)
            self._event_manager.set('start_platform')

        else:
            # stop the applications

            # tell the hosts controller to stop Platine on all hosts
            # (stop will be finished when we will receive a
            # 'resp_stop_platform' event, the button will be enabled there)
            self._event_manager.set('stop_platform')

    def on_dev_mode_button_toggled(self, source=None, event=None):
        """ 'toggled' event on dev_mode button """
        # enable deploy button
        self._dev_mode = not self._dev_mode
        self.hide_deploy_button(not self._dev_mode)
        self._model.set_dev_mode(self._dev_mode)
        
        #TODO add an option to add hosts from scratch in advanced config ?
        #      ie. copy the Dmon with conf, run it and deploy the files

    def on_timer_status(self):
        """ handler to get Platine status from model periodicaly """
        # determine the current status of the platform at first refresh
        # (ie. just after platine manager startup)
        if self._first_refresh:
            # check if we have main components but
            # do not stay refreshed after INIT_ITER iterations
            if not self._model.main_hosts_found() and \
               self._refresh_iter <= INIT_ITER:
                self._log.debug("platform status is not fully known, " \
                                "wait a little bit more...")
                self._refresh_iter = self._refresh_iter + 1
            else:
                if self._refresh_iter > INIT_ITER:
                    self._log.warning("the mandatory components were not "
                                      "found on the system, the platform "
                                      "won't be able to start")
                    self._log.info("you will need at least a satellite, "
                                   "a gateway, a ST and the environment plane: "
                                   "please deploy the missing component(s), "
                                   "they will be automatically detected")

                # update the label of the 'start/stop platine' button to
                gobject.idle_add(self.set_start_stop_button,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                # check if some components are running
                state = self._model.is_running()
                # if at least one application is started at manager startup,
                # let do as if platine was started
                if state:
                    gobject.idle_add(self.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # disable the 'deploy platine' and 'save config' buttons
                    gobject.idle_add(self.disable_deploy_button, True,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    if self._refresh_iter <= INIT_ITER:
                        self._log.info("Platform is currently started")
                else:
                    gobject.idle_add(self.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # enable the 'deploy platine' and 'save config' buttons
                    gobject.idle_add(self.disable_deploy_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    if self._refresh_iter <= INIT_ITER:
                        self._log.info("Platform is currently stopped")
                # platine manager is now ready to be used
                self._first_refresh = False
                if self._refresh_iter <= INIT_ITER:
                    self._log.info("Platine Manager is now ready, have fun !")

        # update event GUI
        gobject.idle_add(self.set_start_stop_button,
                         priority=gobject.PRIORITY_HIGH_IDLE+20)
        gobject.idle_add(self.update_status)

        # restart timer
        return True

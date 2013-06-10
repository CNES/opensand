#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
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
run_event.py - the events on the run view
"""

import gobject
import os
import gtk

from opensand_manager_core.my_exceptions import RunException
from opensand_manager_gui.view.run_view import RunView
from opensand_manager_gui.view.event_handler import EventReponseHandler
from opensand_manager_gui.view.popup.edit_deploy_dialog import EditDeployDialog
from opensand_manager_gui.view.popup.edit_install_dialog import EditInstallDialog
from opensand_manager_gui.view.popup.infos import yes_no_popup

INIT_ITER = 4

class RunEvent(RunView):
    """ Events for the run tab """
    def __init__(self, parent, model, opensand_view, dev_mode, manager_log):
        try:
            RunView.__init__(self, parent, model, manager_log)
        except RunException:
            raise

        self._event_manager = self._model.get_event_manager()
        self._event_response_handler = EventReponseHandler(self,
                            self._model.get_event_manager_response(),
                            opensand_view,
                            self._log)

        self._opensand_view = opensand_view
        self._first_refresh = True
        self._refresh_iter = 0
        # at beginning check platform state immediatly, then every 2 seconds
        self.on_timer_status()
        self._timeout_id = gobject.timeout_add(2000, self.on_timer_status)

        self._dev_mode = False

        # start event response handler
        self._event_response_handler.start()

        if(dev_mode):
            self._ui.get_widget('dev_mode').set_active(True)
            self._ui.get_widget('options').set_visible(True)
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
        elif val and self._timeout_id is None:
            # refresh immediatly then periodically
            self.on_timer_status()
            self._timeout_id = gobject.timeout_add(1000, self.on_timer_status)

    def on_deploy_opensand_button_clicked(self, source=None, event=None):
        """ 'clicked' event on deploy OpenSAND button """
        # disable the buttons
        self.disable_start_button(True)
        self.disable_deploy_buttons(True)

        # tell the hosts controller to deploy OpenSAND on all hosts
        # (the installation will be finished when we will receive a
        # 'resp_deploy_platform' event, the button will be enabled
        # there)
        self._event_manager.set('deploy_platform')

    def on_install_files_button_clicked(self, source=None, event=None):
        """ 'clicked' event on install files button """
        # disable the buttons
        self.disable_start_button(True)
        self.disable_deploy_buttons(True)

        # tell the hosts controller to install files on all hosts
        # (the installation will be finished when we will receive a
        # 'resp_install_files' event, the button will be enabled
        # there)
        self._event_manager.set('install_files')

    def on_start_opensand_button_clicked(self, source=None, event=None):
        """ 'clicked' event on start OpenSAND button """
        # start or stop the applications ?
        if not self._model.is_running():
            # start the applications

            # retrieve the current scenario and start
            self.set_run_id()
            # check that we won't overwrite an existing run
            scenario = self._model.get_scenario()
            run = self._model.get_run()
            path = os.path.join(scenario, run)
            if os.path.exists(path):
                ret = yes_no_popup("You will overwrite an existing run, "
                                   "continue anyway ?",
                                   "Overwrite run %s" % run,
                                   "gtk-dialog-warning")
                if ret != gtk.RESPONSE_YES:
                    return

            # disable the buttons
            self.disable_start_button(True)
            self.disable_deploy_buttons(True)

            # add a line in the event text views
            self._log.info("***** New run: %s *****" % self._model.get_run(),
                           True, True)

            # tell the hosts controller to start OpenSAND on all hosts
            # (startup will be finished when we will receive a
            # 'resp_start_platform' event, the button will be enabled there)
            self._event_manager.set('start_platform')

            self._opensand_view.global_event("***** New run: %s *****" %
                                             self._model.get_run())
        else:
            # disable the buttons
            self.disable_start_button(True)
            self.disable_deploy_buttons(True)

            # stop the applications

            # tell the hosts controller to stop OpenSAND on all hosts
            # (stop will be finished when we will receive a
            # 'resp_stop_platform' event, the button will be enabled there)
            self._event_manager.set('stop_platform')


    def on_dev_mode_button_toggled(self, source=None, event=None):
        """ 'toggled' event on dev_mode button """
        # enable deploy button
        self._dev_mode = not self._dev_mode
        self.hide_deploy_button(not self._dev_mode)
        self._model.set_dev_mode(self._dev_mode)
        self._ui.get_widget('options').set_visible(self._dev_mode)
        
    def on_timer_status(self):
        """ handler to get OpenSAND status from model periodicaly """
        # determine the current status of the platform at first refresh
        # (ie. just after opensand manager startup)
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
                                   "a gateway and a ST: "
                                   "please deploy the missing component(s), "
                                   "they will be automatically detected")
                
                if not self._model.is_collector_functional():
                    self._log.warning("The OpenSAND collector is not known. "
                                      "The probes will not be available.")

                # update the label of the 'start/stop opensand' button to
                gobject.idle_add(self.set_start_stop_button,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                # check if some components are running
                state = self._model.is_running()
                # if at least one application is started at manager startup,
                # let do as if opensand was started
                if state:
                    gobject.idle_add(self.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # disable the 'deploy opensand' and 'save config' buttons
                    gobject.idle_add(self.disable_deploy_buttons, True,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    if self._refresh_iter <= INIT_ITER:
                        self._log.info("Platform is currently started")
                else:
                    gobject.idle_add(self.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # enable the 'deploy opensand' and 'save config' buttons
                    gobject.idle_add(self.disable_deploy_buttons, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    if self._refresh_iter <= INIT_ITER:
                        self._log.info("Platform is currently stopped")
                # opensand manager is now ready to be used
                self._first_refresh = False
                if self._refresh_iter <= INIT_ITER:
                    self._log.info("OpenSAND Manager is now ready, have fun !")

        # update event GUI
        gobject.idle_add(self.set_start_stop_button,
                         priority=gobject.PRIORITY_HIGH_IDLE+20)
        gobject.idle_add(self.update_status)
        
        # Update simulation state for the main view
        gobject.idle_add(self._opensand_view.on_simu_state_changed,
                         priority=gobject.PRIORITY_HIGH_IDLE+20)

        # restart timer
        return True

    def on_option_deploy_clicked(self, source=None, event=None):
        """ deploy button in options menu clicked """
        self.on_deploy_opensand_button_clicked(source, event)

    def on_option_disable_clicked(self, source=None, event=None):
        """ disable button in options menu clicked """
        # this will raise the appropriate event
        self._ui.get_widget('dev_mode').set_active(False)

    def on_option_edit_clicked(self,  source=None, event=None):
        """ edit button in options menu clicked """
        window = EditDeployDialog(self._model, self._log)
        window.go()

    def on_start_stop_activate(self, source=None, event=None):
        """ start/stop button in action menu clicked """
        self.on_start_opensand_button_clicked(source, event)

    def on_install_activate(self, source=None, event=None):
        """ install button in action menu clicked """
        self.on_install_files_button_clicked(source, event)

    def on_edit_install_activate(self, source=None, event=None):
        """ edit button in action menu clicked """
        window = EditInstallDialog(self._model, self._log)
        window.go()
        window.close()



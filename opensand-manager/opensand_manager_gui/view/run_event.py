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
run_event.py - the events on the run view
"""

import gobject
import os
import gtk
import shutil

from opensand_manager_core.my_exceptions import RunException
from opensand_manager_gui.view.run_view import RunView
from opensand_manager_gui.view.popup.edit_deploy_dialog import EditDeployDialog
from opensand_manager_gui.view.popup.spot_gw_assignment import SpotGwAssignmentDialog
from opensand_manager_gui.view.popup.infos import yes_no_popup

class RunEvent(RunView):
    """ Events for the run tab """
    def __init__(self, parent, model, dev_mode, adv_mode,
                 manager_log, service_type):
        try:
            RunView.__init__(self, parent, model, manager_log, service_type)
        except RunException:
            raise

        self._event_manager = self._model.get_event_manager()

        # initialize to False, the set_active methods on buttons
        # will trigger callbacks that will change that
        self._dev_mode = False
        self._adv_mode = False

        if(adv_mode):
            self._ui.get_widget('option_adv_mode').set_active(True)

        if(dev_mode):
            self._ui.get_widget('dev_mode').set_active(True)
            self._ui.get_widget('deployment').set_visible(True)
        else:
            # do not show deploy button
            gobject.idle_add(self.hide_deploy_button,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)


    def close(self):
        """ 'close' signal handler """
        self._log.debug("Run Event: closed")

    def activate(self, val):
        """ 'activate' signal handler """
        pass

    def on_deploy_opensand_button_clicked(self, source=None, event=None):
        """ 'clicked' event on deploy OpenSAND button """
        # disable the buttons
        self.disable_start_button(True)
        self.disable_deploy_buttons(True)
        self.spin("Deploying...")

        # tell the hosts controller to deploy OpenSAND on all hosts
        # (the installation will be finished when we will receive a
        # 'resp_deploy_platform' event, the button will be enabled
        # there)
        self._event_manager.set('deploy_platform')

    def on_start_opensand_button_clicked(self, source=None, event=None):
        """ 'clicked' event on start OpenSAND button """
        # start or stop the applications ?
        if not self._model.is_running():
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
                                   gtk.STOCK_DIALOG_WARNING)
                if ret != gtk.RESPONSE_YES:
                    return
                # clean path
                shutil.rmtree(path)
            # disable the buttons
            self.disable_start_button(True)
            self.disable_deploy_buttons(True)
            # start the applications
            self.spin("Starting...")

            # add a line in the event text views
            self._log.info("***** New run: %s *****" % self._model.get_run(),
                           True, True)

            # tell the hosts controller to start OpenSAND on all hosts
            # (startup will be finished when we will receive a
            # 'resp_start_platform' event, the button will be enabled there)
            self._event_manager.set('start_platform')

            self._log_view.on_start(self._model.get_run())
        else:
            self.spin("Stopping...")
            # disable the buttons
            self.disable_start_button(True)
            self.disable_deploy_buttons(True)

            # stop the applications

            # tell the hosts controller to stop OpenSAND on all hosts
            # (stop will be finished when we will receive a
            # 'resp_stop_platform' event, the button will be enabled there)
            self._event_manager.set('stop_platform')
            self._log_view.on_stop()
            # TODO if all process crashed we should also call on_stop


    def on_dev_mode_button_toggled(self, source=None, event=None):
        """ 'toggled' event on dev_mode button """
        # enable deploy button
        self._dev_mode = not self._dev_mode
        self.hide_deploy_button(not self._dev_mode)
        self._model.set_dev_mode(self._dev_mode)
        self._ui.get_widget('deployment').set_visible(self._dev_mode)

    def on_option_deploy_clicked(self, source=None, event=None):
        """ deploy button in options menu clicked """
        self.on_deploy_opensand_button_clicked(source, event)

    def on_option_disable_clicked(self, source=None, event=None):
        """ disable button in options menu clicked """
        # this will raise the appropriate event
        self._ui.get_widget('dev_mode').set_active(False)

    def on_option_adv_mode_toggled(self, source=None, event=None):
        """ enable/disable advanced mode """
        self._adv_mode = not self._adv_mode
        self._model.set_adv_mode(self._adv_mode)
        if not self._adv_mode and not self._dev_mode:
            # enable back all hosts, hosts cannot be disabled in non-adv/dev mode
            for host in self._model.get_hosts_list():
                host.enable(True)

    def on_option_edit_clicked(self,  source=None, event=None):
        """ edit button in options menu clicked """
        window = EditDeployDialog(self._log)
        window.go()

    def refresh(self):
        """ refresh run view """
        # update the label of the 'start/stop opensand' button to
        self.set_start_stop_button()
        self.update_status()

    def stop_spinning(self):
        """ stop the spinning button """
        widget = self._ui.get_widget('idle_spinner')
        widget.stop()
        widget = self._ui.get_widget('spinbox')
        widget.hide_all()

    def spin(self, action=""):
        """ spin the spin button """
        widget = self._ui.get_widget('idle_spinner')
        widget.start()
        widget.show_all()
        label = self._ui.get_widget('spinner_label')
        label.show_all()
        label.set_markup(action)
        widget = self._ui.get_widget('spinbox')
        widget.show_all()


    def on_add_spot_gw_assignement_clicked(self, source=None, event=None):
        """ 'clicked' event on add spot button """
        window = SpotGwAssignmentDialog(self._model, self._log)
        window.go()
        


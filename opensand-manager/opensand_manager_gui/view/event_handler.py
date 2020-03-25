#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
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
event_handler.py - handler events response from controller
"""

import threading
import gobject
import time

from opensand_manager_gui.view.popup.progress_dialog import ProgressDialog

class EventResponseHandler(threading.Thread):
    """ Get response events from hosts controllers """
    def __init__(self, event_manager_response, run_view,
                 conf_view, resource_view, tool_view, manager_log):
        threading.Thread.__init__(self)
        self.setName("EventResponseHandler")
        self._run_view = run_view
        self._conf_view = conf_view
        self._resource_view = resource_view
        self._tool_view = tool_view
        self._event_manager_response = event_manager_response
        self._log = manager_log
        self._prog_dialog = None

    def run(self):
        """ main loop that manages the events responses """
        while True:
            self._event_manager_response.wait(None)
            event_type = self._event_manager_response.get_type()

            self._log.debug("event response: " + event_type)

            if event_type.startswith("resp"):
                gobject.idle_add(self._run_view.stop_spinning,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)

            if event_type == "resp_deploy_platform":
                # opensand platform installation answer
                # enable the buttons back
                gobject.idle_add(self._run_view.disable_start_button, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                gobject.idle_add(self._run_view.disable_deploy_buttons, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)

            elif event_type == "resp_set_scenario" or \
                 event_type == "resp_update_config":
                pass

            elif event_type == "deploy_files":
                gobject.idle_add(self._run_view.spin, "Saving files...",
                                 priority=gobject.PRIORITY_HIGH_IDLE)
                # deploying files, disable buttons
                gobject.idle_add(self._run_view.disable_start_button, True,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                gobject.idle_add(self._run_view.disable_deploy_buttons, True,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)

            elif event_type == "resp_deploy_files":
                # enable the buttons back
                gobject.idle_add(self._run_view.disable_start_button, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                gobject.idle_add(self._run_view.disable_deploy_buttons, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)

            elif event_type == "resp_start_platform":
                # opensand platform start answer:
                # update buttons according to platform status
                if str(self._event_manager_response.get_text()) == "done":
                    # avoid to update buttons while state has not been correctly
                    # updated
                    idx = 0
                    while not self._run_view.is_running() and idx < 10:
                        idx += 1
                        time.sleep(0.5)
                    # update the label of the 'start/stop opensand' button
                    gobject.idle_add(self._run_view.set_start_stop_button,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # starting opensand platform succeeded:
                    # enable back the 'stop opensand' button
                    gobject.idle_add(self._run_view.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # check if everything went fine
                    while self._run_view.is_running() and idx < 10:
                        idx += 1
                        time.sleep(0.5)

                if not self._run_view.is_running():
                    # starting opensand platform failed:
                    # enable back all the buttons
                    gobject.idle_add(self._run_view.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    gobject.idle_add(self._run_view.disable_deploy_buttons, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)

            elif event_type == "resp_stop_platform":
                # opensand platform stop answer
                # update buttons according to platform status
                if str(self._event_manager_response.get_text()) == "done":
                    # avoid to update buttons while state has not been correctly
                    # updated
                    idx = 0
                    while self._run_view.is_running() and idx < 10:
                        idx += 1
                        time.sleep(0.5)
                # update the label of the 'start/stop opensand' button
                gobject.idle_add(self._run_view.set_start_stop_button,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                # enable all the buttons
                gobject.idle_add(self._run_view.disable_start_button, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                gobject.idle_add(self._run_view.disable_deploy_buttons, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
            
            elif event_type == "probe_transfer":
                gobject.idle_add(self.on_probe_transfer_progress,
                                 True)

            elif event_type == "resp_probe_transfer":
                text = self._event_manager_response.get_text()
                gobject.idle_add(self.on_probe_transfer_progress,
                                 False)

            elif event_type == 'quit':
                # quit event
                self.close()
                return
            else:
                self._log.warning("Response Event Handler: unknown event " \
                                  "'%s' received" % event_type)

            self._event_manager_response.clear()

    def close(self):
        """ close the event response handler """
        self._log.debug("Response Event Handler: closed")


    def on_probe_transfer_progress(self, started):
        """ Called when probe transfer from the collector starts or stops """
        if started:
            self._run_view.spin("Save probes...")
            self._prog_dialog = ProgressDialog("Saving probe data, please "
                                               "wait...")
            self._prog_dialog.show()
        else:
            self._prog_dialog.close()
            self._prog_dialog = None



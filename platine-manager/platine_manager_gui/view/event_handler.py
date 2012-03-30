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
event_handler.py - handler events response from controller
"""

import threading
import gobject
import time

class EventReponseHandler(threading.Thread):
    """ Get response events from hosts controllers """
    def __init__(self, view, event_manager_response, manager_log):
        threading.Thread.__init__(self)
        self._view = view
        self._event_manager_response = event_manager_response
        self._log = manager_log

    def run(self):
        """ main loop that managers the events responses """
        while True:
            self._event_manager_response.wait(None)
            event_type = self._event_manager_response.get_type()

            self._log.debug("event response: " + event_type)

            if event_type == "resp_deploy_platform":
                # platine platform installation answer
                # enable the buttons back
                gobject.idle_add(self._view.disable_start_button, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                gobject.idle_add(self._view.disable_deploy_button, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)

            elif event_type == "resp_start_platform":
                # platine platform start answer:
                # update buttons according to platform status
                if str(self._event_manager_response.get_text()) == "done":
                    # avoid to update buttons while state has not been correctly
                    # updated
                    idx = 0
                    while not self._view.is_running() and idx < 5:
                        idx += 1
                        time.sleep(1)
                    # update the label of the 'start/stop platine' button
                    gobject.idle_add(self._view.set_start_stop_button,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # starting platine platform succeeded:
                    # enable back the 'stop platine' button
                    gobject.idle_add(self._view.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                else:
                    # starting platine platform failed:
                    # enable back all the buttons
                    gobject.idle_add(self._view.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    gobject.idle_add(self._view.disable_deploy_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)

            elif event_type == "resp_stop_platform":
                # platine platform stop answer
                # update buttons according to platform status
                if str(self._event_manager_response.get_text()) == "done":
                    # avoid to update buttons while state has not been correctly
                    # updated
                    idx = 0
                    while self._view.is_running() and idx < 5:
                        idx += 1
                        time.sleep(1)
                    # update the label of the 'start/stop platine' button
                    gobject.idle_add(self._view.set_start_stop_button,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # enable back the 'start platine' button
                    gobject.idle_add(self._view.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)

                # enable all the buttons
                gobject.idle_add(self._view.disable_start_button, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)
                gobject.idle_add(self._view.disable_deploy_button, False,
                                 priority=gobject.PRIORITY_HIGH_IDLE+20)

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
        self._log.debug("Response Event Handler: close")
        self._log.debug("Response Event Handler: closed")

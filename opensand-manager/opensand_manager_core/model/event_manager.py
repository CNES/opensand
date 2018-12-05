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

"""
event_manager.py - Event manager for OpenSAND manager
"""

import threading

class EventManager():
    """ Event manager """

    def __init__(self, name):
        self._name = name
        self._evt = threading.Event()
        self._event_type = 'none'
        self._event_text = ''
        self._evtmngr_lock = threading.Lock()

    def set(self, evt_type, texte=''):
        """ set the event """
        if evt_type != "quit" :
            self._evtmngr_lock.acquire()
        else :
            self._evt.clear()
        self._event_type = evt_type
        self._event_text = texte
        return self._evt.set()

    def clear(self):
        """ clear the event """
        ret = None
        if self._event_type != 'none' and self._event_type != 'quit':
            self._event_type = 'none'
            self._event_text = ''
            ret = self._evt.clear()
            self._evtmngr_lock.release()
            return ret

        if self._event_type == 'quit':
            return self._evt.set()

        return self._evt.clear()

    def is_set(self):
        """ check if an event is set """
        return self._evt.is_set()

    def wait(self, timeout=None):
        """ wait for an event """
        return self._evt.wait(timeout)

    def get_type(self):
        """ get the event type """
        return self._event_type

    def get_text(self):
        """ get the event text """
        return self._event_text

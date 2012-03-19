#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
event_manager.py - Event manager for Platine manager
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
        if self._event_type != 'none' and self._event_type != 'quit':
            self._event_type = 'none'
            self._event_text = ''
            self._evtmngr_lock.release()

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




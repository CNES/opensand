#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
window_view.py - the methods that all the views should implement
"""

import gtk.glade

from platine_manager_core.my_exceptions import ViewException

class WindowView:
    """ Platine manager tab view """

    _gladefile = ''

    def __init__(self, parent = None, balise = 'window', gladefile = ''):
        self._initok = False

        if WindowView._gladefile == '' and gladefile == '':
            raise ViewException("no glade file specified")
        elif gladefile != '':
            if WindowView._gladefile != '':
                raise ViewException("two glade files specified")
            else:
                WindowView._gladefile = gladefile
        # GUI initialization (using glade XML file)
        if parent == None :
            self._ui = gtk.glade.XML(WindowView._gladefile, balise)
            self._ui.signal_autoconnect(locals())
        else :
            self._ui = parent

        #mapping events with functions
        sig = {}
        for iterator in dir(self):
            sig[iterator] = getattr(self, iterator)

        self._ui.signal_autoconnect(sig)

        self._initok = True

    def get_current(self):
        """ get the current view """
        return self._ui

    def close(self):
        """ action performed when 'close' signal is received """
        pass

    def activate(self, val):
        """ action performed when 'activate' signal is received """
        pass



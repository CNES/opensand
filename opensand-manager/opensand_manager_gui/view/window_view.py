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
window_view.py - the methods that all the views should implement
"""

import gtk.glade

from opensand_manager_core.my_exceptions import ViewException

class WindowView:
    """ OpenSAND manager tab view """

    _gladefile = ''

    def __init__(self, parent=None, balise='window', gladefile=''):
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

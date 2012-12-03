#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 TAS
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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
progress_dialog.py - A generic progress dialog
"""

import gobject

from opensand_manager_gui.view.window_view import WindowView


# TODO use that for deployment, start, etc.
class ProgressDialog(WindowView):
    """ a window enabling to edit the deploy.ini file """
    def __init__(self, text, model, manager_log):
        WindowView.__init__(self, None, 'progress_dialog')

        self._dlg = self._ui.get_widget('progress_dialog')
        self._log = manager_log
        self._ui.get_widget('progress_label').set_text(text)
        self._dlg.set_transient_for(self._ui.get_widget('window'))
        self._dlg.set_title("Progress - OpenSAND")
        self._running = True

    def ping(self):
        """ indicates progress """
        if self._running:
            self._ui.get_widget('progress_bar').pulse()
            return True
        return False

    def show(self):
        """ show the window """
        self._dlg.show()
        gobject.timeout_add(100, self.ping)

    def close(self):
        """ close the window """
        self._running = False
        self._dlg.destroy()


if __name__ == "__main__":
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model
 
    gobject.threads_init()
    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)
    WindowView(None, 'none', 'opensand.glade')
    DIALOG = ProgressDialog("test progress dialog", MODEL, LOGGER)
    DIALOG.show()
    try:
        gobject.MainLoop().run()
    except KeyboardInterrupt:
        DIALOG.close()

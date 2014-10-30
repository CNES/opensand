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
edit_dialog.py - A window that enable to edit any file
"""

import gtk

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ModelException

# TODO usr gtksourceview that should enable synthax highlighting
class EditDialog(WindowView):
    """ a window enabling to edit any file """
    def __init__(self, filename):
        WindowView.__init__(self, None, 'edit_dialog')

        self._dlg = self._ui.get_widget('edit_dialog')
        self._dlg.set_title("Edit %s - OpenSAND Manager" % filename)
        self._dlg.set_keep_above(True)
        self._buff = gtk.TextBuffer()
        win = self._ui.get_widget('edit_text_win')
        # TODO SourceView here
        self._text = gtk.TextView()
        win.add(self._text)
        self._text.show()
        self._path = filename

    def go(self):
        """ run the window """
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
            self.close()
            return
        self._dlg.set_icon_name('gtk-edit')
        self._dlg.run()

    def close(self):
        """ close the window """
        self._dlg.destroy()

    def load(self):
        """ load the hosts configuration """
        self._text.set_wrap_mode(gtk.WRAP_NONE)
        self._text.set_buffer(self._buff)
        
        try:
            self.copy_file_in_buffer()
        except (OSError, IOError), (_, strerror):
            raise ModelException("cannot load deploy file %s (%s)" %
                                 (self._path, strerror))

    def copy_file_in_buffer(self):
        """ copy the deploy.ini file in the text buffer """
        with open(self._path, 'r') as edit_file:
            self._buff.set_text(edit_file.read())
            
        self._text.set_size_request(700, 600)

    def on_save_edit_clicked(self, source=None, event=None):
        """ 'clicked' event callback on save button """
        start, end = self._buff.get_bounds()
        content = self._buff.get_text(start, end)
        try:
            with open(self._path, 'w') as deploy:
                deploy.write(content)
        except Exception, err:
            error_popup("Error saving file:", str(err))
        self.close()

    def on_cancel_edit_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        self.close()

if __name__ == "__main__":
    import sys
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model

    LOGGER = ManagerLog(7, True, True, True)
    MODEL = Model(LOGGER)
    WindowView(None, 'none', 'opensand.glade')
    DIALOG = EditDialog(sys.argv[1])
    DIALOG.go()
    DIALOG.close()

#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
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
edit_deploy_dialog.py - A window that enable to edit the deploy.ini file
"""

import gtk
import os

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ModelException

class EditDeployDialog(WindowView):
    """ a window enabling to edit the deploy.ini file """
    def __init__(self, model, manager_log):
        WindowView.__init__(self, None, 'edit_deploy_dialog')

        self._dlg = self._ui.get_widget('edit_deploy_dialog')
        self._dlg.set_keep_above(True)
        self._log = manager_log
        self._buff = gtk.TextBuffer()
        self._text = self._ui.get_widget('deploy_text')
        self._path = ''

    def go(self):
        """ run the window """
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
            self.close()
            return
        self._dlg.set_title("Edit ~/.opensand/deploy.ini - OpenSAND")
        self._dlg.set_icon_name('gtk-edit')
        self._dlg.run()

    def close(self):
        """ close the window """
        self._dlg.destroy()

    def load(self):
        """ load the hosts configuration """
        self._text.set_wrap_mode(gtk.WRAP_WORD)
        self._text.set_buffer(self._buff)
        comment = self._buff.create_tag('comment')
        comment.set_property('foreground', 'purple')
        option = self._buff.create_tag('option')
        option.set_property('foreground', 'red')

        try:
            self.copy_file_in_buffer()
        except (OSError, IOError), (errno, strerror):
            raise ModelException("cannot load deploy file %s (%s)" %
                                 (self._path, strerror))

        self._buff.connect('changed', self.interactive_color)


    def copy_file_in_buffer(self):
        """ copy the deploy.ini file in the text buffer """
        if not 'HOME' in os.environ:
            raise ModelException("cannot get HOME environment variable")
        self._path = os.path.join(os.environ['HOME'], ".opensand/deploy.ini")

        with open(self._path, 'r') as deploy:
            line = deploy.readline()
            while line != '':
                if line.startswith('#'):
                    self._buff.insert_with_tags_by_name(
                        self._buff.get_end_iter(), line, 'comment')
                elif line.startswith('['):
                    self._buff.insert_with_tags_by_name(
                        self._buff.get_end_iter(), line, 'option')
                else:
                    self._buff.insert(self._buff.get_end_iter(), line)
                line = deploy.readline()

        self._text.set_size_request(700, 600)

    def on_save_deploy_edit_clicked(self, source=None, event=None):
        """ 'clicked' event callback on save button """
        start, end = self._buff.get_bounds()
        content = self._buff.get_text(start, end)
        with open(self._path, 'w') as deploy:
            deploy.write(content)
        self.close()

    def on_cancel_deploy_edit_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        self.close()

    def interactive_color(self, source=None, event=None):
        cursor = source.get_iter_at_mark(source.get_insert())
        if cursor.get_line_offset() > 1:
            return
        line = cursor.get_line()
        start = source.get_iter_at_line(line)
        end = cursor
        end.forward_line()
        source.remove_all_tags(start, end)
        if start.get_char() == '#':
            source.apply_tag_by_name('comment', start, end)
        if start.get_char() == '[':
            source.apply_tag_by_name('option', start, end)



if __name__ == "__main__":
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model

    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)
    WindowView(None, 'none', 'opensand.glade')
    DIALOG = EditDeployDialog(MODEL, LOGGER)
    DIALOG.go()
    DIALOG.close()

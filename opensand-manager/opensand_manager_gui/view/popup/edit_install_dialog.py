#!/usr/bin/env python 
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 CNES
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
EditInstallDialog.py - Edit the simulation files deployment elements
"""

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ModelException
from opensand_manager_gui.view.utils.config_elements import InstallNotebook

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class EditInstallDialog(WindowView):
    """ the window used to edit the simulation deployment file """
    def __init__(self, model, manager_log):
        WindowView.__init__(self, None, 'edit_install_dialog')

        self._dlg = self._ui.get_widget('edit_install_dialog')
        self._dlg.set_keep_above(True)
        self._model = model
        self._log = manager_log
        self._files = self._model.get_files()
        self._modified = []

    def go(self):
        """ run the window """
        try:
            self.load()
        except ModelException, msg:
            error_popup(str(msg))
            self.close()
            return
        self._dlg.set_title("Edit install dialog - PtManager")
        self._dlg.set_icon_name('gtk-edit')
        self._dlg.run()

    def close(self):
        """ close the window """
        self._dlg.destroy()

    def load(self):
        """ load the simulation deployment file """
        conf_view = self._ui.get_widget('edit_install_config')
        notebook = InstallNotebook(self._files, self.modif_cb)
        conf_view.pack_start(notebook)
        notebook.show_all()

    def modif_cb(self, source, event=None):
        """ callback for source modification in the simlation deployment
            notebook """
        if not source in self._modified:
            self._modified.append(source)

    def on_save_deploy_edit_clicked(self, source=None, event=None):
        """ 'clicked' event callback on save button """
        for elem in self._modified:
            info = elem.get_name().split(':')
            host = info[0]
            path = info[1]
            val = elem.get_text()
            self._model.modify_deploy_simu(host, path, val)

        self._model.save_deploy_simu()
        self.close()

    def on_cancel_install_edit_clicked(self, source=None, event=None):
        """ 'clicked' event callback on undo button """
        self.close()


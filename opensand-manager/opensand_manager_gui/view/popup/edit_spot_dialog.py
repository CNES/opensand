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
edit_spot_dialog.py - OpenSAND manager edit spot id to add or remove
"""

import gtk

from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.window_view import WindowView


class EditSpotDialog(WindowView):
    """ dialog to get the scenario to load """
    def __init__(self, model):
        WindowView.__init__(self, None, 'edit_spot_dialog')

        self._dlg = self._ui.get_widget('edit_spot_dialog')
        self._dlg.set_keep_above(True)
        self.model = model
        self.spot = ""

    def go(self):
        """ run the window """
        self._dlg.set_icon_name(gtk.STOCK_PREFERENCES)
        # run the dialog and store the response
        self._dlg.run()
        return self.spot
        
    def close(self):
        """ close the window """
        self._dlg.destroy()

    def on_edit_spot_id_delete_event(self, source=None, event=None):
        """ delete-event on window """
        self.close()
        
    def on_cancel_spot_id_clicked(self, source=None, event=None):
        """ cancel spot id edition"""
        self.close()
        
    def on_ok_spot_id_clicked(self, source=None, event=None):
        """ save spot id edited"""
        widget = self._ui.get_widget("entry_spot")
        find = False
        if widget.get_text() != None:
            """check spot id exist"""
            config = self.model.get_conf().get_configuration()
            xpath = "//return_up_band"
            elm = config.get(xpath)
            for KEY in config.get_keys(elm):
                if KEY.tag == "spot":
                    content = config.get_element_content(KEY)
                    spot = content["id"]
                    if spot == widget.get_text():
                        find = True;

        if not find:
            error_popup("Wrong spot id.")
        else: 
            self.spot = widget.get_text()
            self.close()
            


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
conf_view.py - the configuration tab view
"""

import os
import gobject

from platine_manager_gui.view.window_view import WindowView
from platine_manager_core.my_exceptions import ConfException

IMG_PATH = "/usr/share/platine/manager/images/"

#TODO find a way to better handle encapsulation schemes and eventually to add one easily
class ConfView(WindowView) :
    """ Element for the configuration tab """

    def __init__(self, parent, model, manager_log):
        WindowView.__init__(self, parent)

        self._log = manager_log

        self._model = model

        self._text_widget = self._ui.get_widget('conf_descr_text')
        self._text_widget.set_alignment(0.0, 0.0)
        self._text_widget.set_padding(20, 20)

        self._img_widget = self._ui.get_widget('conf_descr_image')
        self._img_path = IMG_PATH

        self._description = 'TODO parameters description'
        self._title = 'Protocol stack'

        # parameter to get the correct image on description
        self._payload = ''
        self._up = ''
        self._down = ''

        # check that image path exist
        if not os.path.exists(self._img_path):
            self._log.error("image path '%s' does not exist" % self._img_path)
            raise ConfException("image path '%s' does not exist" %
                                 self._img_path)

        # update view from model
        try:
            gobject.idle_add(self.update_view,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)
        except:
            raise

    def update_view(self):
        """ update the configuration view according to model
            (should be used with gobject.idle_add outside gtk handlers) """
        # main config parameters
        try:
            # emission_std
            widget = self._ui.get_widget(self._model.get_emission_std())
            widget.set_active(True)
            widget.clicked()
            # payload_type
            widget = self._ui.get_widget(self._model.get_payload_type())
            widget.set_active(True)
            widget.clicked()
            # out_encapsulation
            if self._model.get_out_encapsulation() == 'GSE':
                widget = self._ui.get_widget('GSE_OUT')
            elif self._model.get_out_encapsulation() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_OUT')
            else:
                wname = self._model.get_out_encapsulation()
                widget = self._ui.get_widget(wname)
            widget.set_active(True)
            widget.clicked()
            # in_encapsulation
            if self._model.get_in_encapsulation() == 'GSE':
                widget = self._ui.get_widget('GSE_IN')
            elif self._model.get_in_encapsulation() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_IN')
            else:
                widget = self._ui.get_widget(self._model.get_in_encapsulation())
            widget.set_active(True)
            widget.clicked()
            # terminal_type
            widget = self._ui.get_widget(self._model.get_terminal_type())
            widget.set_active(True)
            widget.clicked()
            # frame_duration
            widget = self._ui.get_widget('FrameDuration')
            widget.set_value(self._model.get_frame_duration())

            # disable DVB-S2 emission standard on ST beacuse it is not
            # implemented yet
            widget = self._ui.get_widget('DVB-S2')
            widget.set_sensitive(False)
            # disable advanced options because they are not implemented yet
            widget = self._ui.get_widget('advanced_conf')
            widget.set_sensitive(False)
        except:
            raise

    def is_modified(self):
        """ check if the configuration was modified by user
            (used in callback so no need to use locks) """
        try:
            # emission_std
            widget = self._ui.get_widget(self._model.get_emission_std())
            if not widget.get_active():
                return True
            # payload_type
            widget = self._ui.get_widget(self._model.get_payload_type())
            if not widget.get_active():
                return True
            # out_encapsulation
            if self._model.get_out_encapsulation() == 'GSE':
                widget = self._ui.get_widget('GSE_OUT')
            elif self._model.get_out_encapsulation() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_OUT')
            else:
                wname = self._model.get_out_encapsulation()
                widget = self._ui.get_widget(wname)
            if not widget.get_active():
                return True
            # in_encapsulation
            if self._model.get_in_encapsulation() == 'GSE':
                widget = self._ui.get_widget('GSE_IN')
            elif self._model.get_in_encapsulation() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_IN')
            else:
                widget = self._ui.get_widget(self._model.get_in_encapsulation())
            if not widget.get_active():
                return True
            # terminal_type
            widget = self._ui.get_widget(self._model.get_terminal_type())
            if not widget.get_active():
                return True
            # frame_duration
            widget = self._ui.get_widget('FrameDuration')
            if (widget.get_text() == self._model.get_frame_duration()):
                return True
        except:
            raise

        return False

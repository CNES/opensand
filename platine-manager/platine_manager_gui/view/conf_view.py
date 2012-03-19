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
DESCRIPTION = (
"<span size='x-large'><b>Main configuration for Platine</b></span>\n\n"
"<span size='large'>For more Platine configuration parameters click the "
"advanced button below.\n\n\n\n"
"The following schema describes the protocol stack according to selected "
"configuration:\n\t</span>The payload type is the type of satellite:\n\t\t"
"<i>transparent = starred\n\t\tregenerative = meshed</i>\n\tThe return "
"link/uplink standard is the output DVB standard for terminals, only DVB-RCS "
"is available for the moment\n\tThe DAMA algorithm is the available DAMA "
"implementations\n\tThe return link/uplink and forward link/dowlink "
"are the available encapsulation protocols according to the payload\n\ttype and"
" return link/uplink standard\n\tThe terminal type corresponds to 2 link budget "
"hypothesis\n\tThe frame duration is the duration of a DVB frame")

#TODO find a way to better handle encapsulation schemes and eventually to add one easily
class ConfView(WindowView) :
    """ Element for the configuration tab """

    _dama_rcs = ["Legacy", "UoR", "Stub", "Yes"]
    _dama_s2 = ["MF2-TDMA"]

    def __init__(self, parent, model, manager_log):
        WindowView.__init__(self, parent)

        self._log = manager_log

        self._model = model

        self._text_widget = self._ui.get_widget('conf_descr_text')

        self._img_widget = self._ui.get_widget('conf_descr_image')
        self._img_path = IMG_PATH

        self._description = DESCRIPTION
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
            config = self._model.get_conf()
            # payload_type
            widget = self._ui.get_widget(config.get_payload_type())
            widget.set_active(True)
            widget.clicked()
            # emission_std
            widget = self._ui.get_widget(config.get_emission_std())
            widget.set_active(True)
            widget.clicked()
            # dama
            widget = self._ui.get_widget('dama_box')
            if config.get_emission_std() == "DVB-S2":
                widget.set_active(self._dama_s2.index(config.get_dama()))
            else:
                widget.set_active(self._dama_rcs.index(config.get_dama()))
            # up_return_encap
            if config.get_up_return_encap() == 'GSE':
                widget = self._ui.get_widget('GSE_OUT')
            elif config.get_up_return_encap() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_OUT')
            else:
                wname = config.get_up_return_encap()
                widget = self._ui.get_widget(wname)
            widget.set_active(True)
            widget.clicked()
            # down_forward_encap
            if config.get_down_forward_encap() == 'GSE':
                widget = self._ui.get_widget('GSE_IN')
            elif config.get_down_forward_encap() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_IN')
            else:
                widget = self._ui.get_widget(config.get_down_forward_encap())
            widget.set_active(True)
            widget.clicked()
            # terminal_type
            widget = self._ui.get_widget(config.get_terminal_type())
            widget.set_active(True)
            widget.clicked()
            # frame_duration
            widget = self._ui.get_widget('FrameDuration')
            widget.set_value(int(config.get_frame_duration()))
        except:
            raise

    def is_modified(self):
        """ check if the configuration was modified by user
            (used in callback so no need to use locks) """
        try:
            config = self._model.get_conf()
            # payload_type
            widget = self._ui.get_widget(config.get_payload_type())
            if not widget.get_active():
                return True
            # emission_std
            widget = self._ui.get_widget(config.get_emission_std())
            if not widget.get_active():
                return True
            # dama
            widget = self._ui.get_widget('dama_box')
            model = widget.get_model()
            active = widget.get_active()
            if active < 0:
                return True
            if model[active][0] != config.get_dama():
                return True
            # up_return_encap
            if config.get_up_return_encap() == 'GSE':
                widget = self._ui.get_widget('GSE_OUT')
            elif config.get_up_return_encap() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_OUT')
            else:
                wname = config.get_up_return_encap()
                widget = self._ui.get_widget(wname)
            if not widget.get_active():
                return True
            # down_forward_encap
            if config.get_down_forward_encap() == 'GSE':
                widget = self._ui.get_widget('GSE_IN')
            elif config.get_down_forward_encap() == 'MPEG_ULE':
                widget = self._ui.get_widget('MPEG_IN')
            else:
                widget = self._ui.get_widget(config.get_down_forward_encap())
            if not widget.get_active():
                return True
            # terminal_type
            widget = self._ui.get_widget(config.get_terminal_type())
            if not widget.get_active():
                return True
            # frame_duration
            widget = self._ui.get_widget('FrameDuration')
            if widget.get_text() == int(config.get_frame_duration()):
                return True
        except:
            raise

        return False



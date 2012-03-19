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
conf_event.py - the events on configuration tab
"""

import gtk
import os
import gobject

from platine_manager_gui.view.conf_view import ConfView
from platine_manager_gui.view.popup.infos import error_popup
from platine_manager_core.my_exceptions import ModelException

#TODO find a way to handle properly links protocols
class ConfEvent(ConfView) :
    """ Events on configuration tab """

    def __init__(self, parent, model, manager_log):
        ConfView.__init__(self, parent, model, manager_log)

        self._modif = False
        self._previous_img = ''
        self._descr_refresh = gobject.timeout_add(1000,
                                                  self.refresh_description)

        self._img_widget.clear()
        gobject.idle_add(self.enable_conf_buttons, False)

    def close(self):
        """ close the configuration tab """
        self._log.debug("Conf Event: close")
        if self._descr_refresh is not None:
            gobject.source_remove(self._descr_refresh)
        self._log.debug("Conf Event: description refresh joined")
        self._log.debug("Conf Event: closed")

    def activate(self, val):
        """ 'activate' signal handler """
        if val == False:
            if self._descr_refresh is not None:
                gobject.source_remove(self._descr_refresh)
            self._img_widget.clear()
            self._descr_refresh = None
            self._modif = False
        else:
            self._modif = True
            self._previous_img = ''
            self._descr_refresh = gobject.timeout_add(1000,
                                                      self.refresh_description)

    def refresh_description(self):
        """ refresh the description and image """
        if not self._modif:
            return True

        gobject.idle_add(self.refresh_gui)
        # continue to refresh
        return True

    def refresh_gui(self):
        """ the part of the refreshing that modifies GUI
            (should be used with gobject.idle_add outside gtk ) """
        cursor = gtk.gdk.Cursor(gtk.gdk.WATCH)
        if self._img_widget.window is not None:
            self._img_widget.window.set_cursor(cursor)

        self._text_widget.set_markup(self._description)

        img_name = "%s_%s_%s.png" % (self._payload, self._up, self._down)
        if self._previous_img != img_name and \
           os.path.exists(os.path.join(self._img_path, img_name)):
            self._previous_img = img_name
            self._img_widget.set_from_file(os.path.join(self._img_path,
                                                        img_name))

        if self._img_widget.window is not None:
            self._img_widget.window.set_cursor(None)

    def set_all_downlink_buttons_sensitive(self):
        """ set buttons in Downlink category sensitive """
          #MPEG button
        widget = self._ui.get_widget('MPEG_IN')
        widget.set_sensitive(True)
          #GSE in button
        widget = self._ui.get_widget('GSE_IN')
        widget.set_sensitive(True)
          #MPEG_ATM_AAL5 button
        widget = self._ui.get_widget('MPEG_ATM_AAL5')
        widget.set_sensitive(True)
          #GSE_ATM_AAL5 button
        widget = self._ui.get_widget('GSE_ATM_AAL5')
        widget.set_sensitive(True)
          #GSE_MPEG_ULE button
        widget = self._ui.get_widget('GSE_MPEG_ULE')
        widget.set_sensitive(True)

    def set_all_uplink_buttons_sensitive(self):
        """ set buttons in Uplink category sensitive """
          #MPEG button
        widget = self._ui.get_widget('MPEG_OUT')
        widget.set_sensitive(True)
          #GSE in button
        widget = self._ui.get_widget('GSE_OUT')
        widget.set_sensitive(True)
          #MPEG_ATM_AAL5 button
        widget = self._ui.get_widget('ATM_AAL5')
        widget.set_sensitive(True)

    def is_button_active(self, button):
        """ check if a button is active """
        widget = self._ui.get_widget(button)
        return widget.get_active()

    def disable_button(self, button, replacement_button = 'None'):
        """ make a button not sensitive and select another button if it
            was previously selected """
        widget = self._ui.get_widget(button)
        widget.set_sensitive(False)
        if self.is_button_active(button) and replacement_button is not None:
            widget = self._ui.get_widget(replacement_button)
            widget.set_active(True)
            widget.clicked()

    def enable_conf_buttons(self, enable = True):
        """ make apply and cancel buttons sensitive or not """
        self._modif = True

        # check if Platine is running
        if self._model.is_running():
            enable = False

        self._ui.get_widget('save_conf').set_sensitive(enable)
        self._ui.get_widget('undo_conf').set_sensitive(enable)

    def on_regenerative_button_clicked(self, source=None, event=None):
        """ actions performed when regenerative is selected """
        self._payload = 'regen'
        self.enable_conf_buttons()
        self.set_all_downlink_buttons_sensitive()
        if(self.is_button_active('ATM_AAL5') == True):
            self.disable_button('MPEG_IN', 'MPEG_ATM_AAL5')
            self.disable_button('GSE_IN', 'GSE_ATM_AAL5')
            self.disable_button('GSE_MPEG_ULE', 'GSE_ATM_AAL5')
        if(self.is_button_active('MPEG_OUT') == True):
            self.disable_button('GSE_IN', 'GSE_MPEG_ULE')
            self.disable_button('MPEG_ATM_AAL5', 'MPEG_ULE')
            self.disable_button('GSE_ATM_AAL5', 'GSE_MPEG_ULE')
        if(self.is_button_active('GSE_OUT') == True):
            self.disable_button('MPEG_IN', 'GSE_IN')
            self.disable_button('MPEG_ATM_AAL5', 'GSE_IN')
            self.disable_button('GSE_ATM_AAL5', 'GSE_IN')
            self.disable_button('GSE_MPEG_ULE', 'GSE_IN')
        widget = self._ui.get_widget('label_in_encap')
        widget.set_markup('<b>ST Downlink Encapsulation Scheme</b>')
        widget = self._ui.get_widget('label_out_encap')
        widget.set_markup('<b>ST Uplink Encapsulation Scheme</b>')
        widget = self._ui.get_widget('label_emission_standard')
        widget.set_markup('<b>ST Uplink Standard</b>')

    def on_transparent_button_clicked(self, source=None, event=None):
        """ actions performed when transparent is selected """
        self._payload = 'transp'
        self.enable_conf_buttons()
        self.set_all_downlink_buttons_sensitive()
        self.disable_button('MPEG_ATM_AAL5', 'MPEG_IN')
        self.disable_button('GSE_ATM_AAL5', 'GSE_IN')
        self.disable_button('GSE_MPEG_ULE', 'GSE_IN')
        if(self.is_button_active('GSE_OUT') == True):
            self.disable_button('MPEG_IN', 'GSE_IN')
        widget = self._ui.get_widget('label_in_encap')
        widget.set_markup('<b>Forward Link Encapsulation Scheme</b>')
        widget = self._ui.get_widget('label_out_encap')
        widget.set_markup('<b>Return Link Encapsulation Scheme</b>')
        widget = self._ui.get_widget('label_emission_standard')
        widget.set_markup('<b>ST Return Link Standard</b>')

    def on_dvb_rcs_button_clicked(self, source=None, event=None):
        """ actions performed when DVB-RCS is selected """
        self.enable_conf_buttons()
        self.set_all_uplink_buttons_sensitive()
        self.disable_button('GSE_OUT', 'ATM_AAL5')

    def on_dvb_s2_button_clicked(self, source=None, event=None):
        """ actions performed when DVB-S2 is selected """
        self.enable_conf_buttons()
        self.set_all_uplink_buttons_sensitive()
        self.disable_button('ATM_AAL5', 'GSE_OUT')
        self.disable_button('MPEG_OUT', 'GSE_OUT')

    def on_atm_out_button_clicked(self, source=None, event=None):
        """ actions performed when ATM is selected on uplink """
        self._up = 'atm'
        self.enable_conf_buttons()
        self.set_all_downlink_buttons_sensitive()
        self.disable_button('GSE_MPEG_ULE', 'GSE_IN')
        if(self.is_button_active('regenerative') == True):
            self.disable_button('MPEG_IN', 'MPEG_ATM_AAL5')
            self.disable_button('GSE_IN', 'GSE_ATM_AAL5')
        if(self.is_button_active('transparent') == True):
            self.disable_button('MPEG_ATM_AAL5', 'MPEG_IN')
            self.disable_button('GSE_ATM_AAL5', 'GSE_IN')

    def on_mpeg_out_button_clicked(self, source=None, event=None):
        """ actions performed when MPEG is selected on uplink """
        self._up = 'mpeg'
        self.enable_conf_buttons()
        self.set_all_downlink_buttons_sensitive()
        self.disable_button('MPEG_ATM_AAL5', 'MPEG_IN')
        self.disable_button('GSE_ATM_AAL5', 'GSE_IN')
        if(self.is_button_active('regenerative') == True):
            self.disable_button('GSE_IN', 'GSE_MPEG_ULE')
        if(self.is_button_active('transparent') == True):
            self.disable_button('GSE_MPEG_ULE', 'GSE_IN')

    def on_gse_out_button_clicked(self, source=None, event=None):
        """ actions performed when GSE is selected on uplink """
        self._up = 'gse'
        self.enable_conf_buttons()
        self.set_all_downlink_buttons_sensitive()
        self.disable_button('MPEG_IN', 'GSE_IN')
        self.disable_button('MPEG_ATM_AAL5', 'GSE_IN')
        self.disable_button('GSE_ATM_AAL5', 'GSE_IN')
        self.disable_button('GSE_MPEG_ULE', 'GSE_IN')

    def on_gse_in_button_clicked(self, source=None, event=None):
        """ 'clicked' event on GSE downlink buttons """
        self._down = 'gse'
        self.enable_conf_buttons()

    def on_mpeg_in_button_clicked(self, source=None, event=None):
        """ 'clicked' event on MPEG downlink buttons """
        self._down = 'mpeg'
        self.enable_conf_buttons()

    def on_undo_conf_clicked(self, source=None, event=None):
        """ reload conf from the ini file """
        self.update_view()
        self.enable_conf_buttons(False)

    def on_save_conf_clicked(self, source=None, event=None):
        """ save the new configuration in the ini file """
        # retrieve global parameters
        self.enable_conf_buttons(False)

        # payload type
        if(self.is_button_active('transparent') == True):
            payload_type = 'transparent'
        elif(self.is_button_active('regenerative') == True):
            payload_type = 'regenerative'
        else:
            return
        self._model.set_payload_type(payload_type)

        # emission standard
        if(self.is_button_active('DVB-RCS') == True):
            emission_std = 'DVB-RCS'
        elif(self.is_button_active('DVB-S2') == True):
            emission_std = 'DVB-S2'
        else:
            return
        self._model.set_emission_std(emission_std)

        # output encapsulation scheme
        if(self.is_button_active('ATM_AAL5') == True):
            out_encapsulation = 'ATM_AAL5'
        elif (self.is_button_active('MPEG_OUT') == True):
            out_encapsulation = 'MPEG_ULE'
        elif (self.is_button_active('GSE_OUT') == True):
            out_encapsulation = 'GSE'
        else:
            return
        self._model.set_out_encapsulation(out_encapsulation)

        # input encapsulation scheme
        if (self.is_button_active('MPEG_IN') == True):
            in_encapsulation = 'MPEG_ULE'
        elif (self.is_button_active('GSE_IN') == True):
            in_encapsulation = 'GSE'
        elif (self.is_button_active('MPEG_ATM_AAL5') == True):
            in_encapsulation = 'MPEG_ATM_AAL5'
        elif (self.is_button_active('GSE_ATM_AAL5') == True):
            in_encapsulation = 'GSE_ATM_AAL5'
        elif (self.is_button_active('GSE_MPEG_ULE') == True):
            in_encapsulation = 'GSE_MPEG_ULE'
        else:
            return
        self._model.set_in_encapsulation(in_encapsulation)

        # terminal type
        if (self.is_button_active('collective') == True):
            terminal_type = 'collective'
        else:
            terminal_type = 'individual'
        self._model.set_terminal_type(terminal_type)

        # fame duration
        widget = self._ui.get_widget('FrameDuration')
        frame_duration = widget.get_text()
        self._model.set_frame_duration(frame_duration)

        try:
            self._model.save_configuration()
        except ModelException as error:
            error_popup(error.value)

    def on_frame_duration_value_changed(self, source=None, event=None):
        """ 'change-value' event callback on frame duration burron """
        self.enable_conf_buttons()

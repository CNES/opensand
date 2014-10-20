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
infos.py - the popup used for information
"""

import gtk

# TODO variable to say if there is already a popup, we should not launch many
# of these popups
# TODO we may replace some popups with Info Bar

def error_popup(error, text = ''):
    """ display an error popup without instantiating a new main loop """
    dialog = gtk.MessageDialog(None, gtk.DIALOG_MODAL,
                               gtk.MESSAGE_ERROR,
                               gtk.BUTTONS_CLOSE,
                               error)
    dialog.set_keep_above(True)
    dialog.format_secondary_text(text)
    dialog.set_title("Error - OpenSAND Manager")
    dialog.set_icon_name('gtk-dialog-error')
    if not dialog.modal:
        dialog.set_modal(True)
    dialog.connect('response', dialog_response_cb)
    dialog.show()

def dialog_response_cb(dialog, response_id):
    """ on click close the error popup """
    dialog.destroy()

def yes_no_popup(question, title, icon):
    """ dialog box asking a question """
    dialog = gtk.MessageDialog(None, gtk.DIALOG_MODAL,
                               gtk.MESSAGE_WARNING,
                               gtk.BUTTONS_YES_NO,
                               question)
    dialog.set_keep_above(True)
    dialog.set_title(title)
    dialog.set_icon_name(icon)
    ret = dialog.run()
    dialog.destroy()
    return ret

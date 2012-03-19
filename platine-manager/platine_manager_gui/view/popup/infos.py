#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
infos.py - the popup used for information
"""

import gtk

def error_popup(error, text = ''):
    """ display an error popup without instantiating a new main loop """
    dialog = gtk.MessageDialog(None, gtk.DIALOG_MODAL,
                               gtk.MESSAGE_ERROR,
                               gtk.BUTTONS_CLOSE,
                               error)
    dialog.format_secondary_text(text)
    dialog.set_title("Error - Platine Manager")
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
    dialog.set_title(title)
    dialog.set_icon_name(icon)
    ret = dialog.run()
    dialog.destroy()
    return ret


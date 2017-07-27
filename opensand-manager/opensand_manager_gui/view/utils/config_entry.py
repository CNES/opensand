#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2017 TAS
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
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>

"""
config_entry.py - create an entry for configuration elements
"""

import gtk
import gobject
import os
import pango
from copy import deepcopy

from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.popup.edit_dialog import EditDialog

(TEXT, VISIBLE_CHECK_BOX, CHECK_BOX_SIZE, ACTIVE, \
 ACTIVATABLE, VISIBLE, RESTRICTED) = range(7)
(DISPLAYED, NAME, PROBE_ID, SIZE) = range(4)

class ConfEntry(object):
    """ element for configuration entry """
    def __init__(self, entry_type, value, path, source, host,
                 sig_handlers, file_handler):
        self._type = entry_type
        self._value = value
        self._path = path
        self._source = source
        self._host = host
        self._entry = None
        self._sig_handlers = sig_handlers
        self._file_handler = file_handler

        type_name = ""
        if self._type is None:
            self.load_default()
            type_name = "string"
        else:
            type_name = self._type["type"]
        if type_name == "boolean":
            self.load_bool()
        elif type_name == "enum":
            self.load_enum()
        elif type_name == "numeric":
            self.load_num()
        elif type_name == "file":
            self.load_file()
        else:
            self.load_default()

    def load_default(self):
        """ load a gtk.Entry """
        self._entry = gtk.Entry()
        self._entry.set_text(self._value)
        self._entry.set_width_chars(20)
        self._entry.set_inner_border(gtk.Border(1, 1, 1, 1))
        self._entry.connect('changed', self.global_handler)

    def load_bool(self):
        """ load a gtk.CheckButton """
        self._entry = gtk.CheckButton()
        if self._value == "true":
            self._entry.set_active(1)
        else:
            self._entry.set_active(0)
        self._entry.connect('toggled', self.global_handler)

    def load_enum(self):
        """ load a gtk.ComboBox """
        self._entry = gtk.ComboBox()
        store = gtk.ListStore(gobject.TYPE_STRING)
        for elt in self._type["enum"]:
            store.append([elt])
        self._entry.set_model(store)
        cell = gtk.CellRendererText()
        self._entry.pack_start(cell, True)
        self._entry.add_attribute(cell, 'text', 0)
        if self._value != '':
            try:
                self._entry.set_active(self._type["enum"].index(self._value))
            except ValueError:
                error_popup("Cannot load element %s, value %s is not in the list"
                            % (self._path, self._value))
        self._entry.connect('changed', self.global_handler)
        self._entry.connect('scroll-event', self.do_not_scroll)

    def load_num(self):
        """ load a gtk.SpinButton """
        low = -1000000000
        up = 1000000000
        step = 1
        digits = 0
        if "min" in self._type:
            low = float(self._type["min"])
        if "max" in self._type:
            up = float(self._type["max"])
        if "step" in self._type:
            step = str(self._type["step"])
            digits=len(step[step.find('.'):]) - 1
            step = float(self._type["step"])
        if self._value != '':
            val = float(self._value)
        else:
            val = low
        adj = gtk.Adjustment(value=val, lower=low, upper=up,
                             step_incr=step, page_incr=0, page_size=0)
        self._entry = gtk.SpinButton(adjustment=adj, climb_rate=step,
                                     digits=digits)
        self._entry.connect('value-changed', self.global_handler)
        self._entry.connect('scroll-event', self.do_not_scroll)

    def load_file(self):
        """ load a gtk.FileChooserButton """
        # there is a special case with files, see above
        # In title, set the destination path
        self._entry = gtk.HBox()
        def edit_file(edit_button, event):
            window = EditDialog(self._source)
            ret = window.go()
            if ret == gtk.RESPONSE_APPLY:
                self.global_handler()
                if self._file_handler is not None:
                    self._file_handler(self._source, self._host, self._path)

        edit_button = gtk.Button(stock=gtk.STOCK_EDIT)
        if self._source is None:
            # new line in table, the file does not exist
            edit_button.set_sensitive(False)
        edit_button.show()
        # show edit dialog
        edit_button.connect('button-press-event', edit_file)
        # consider file may be changed
        self._entry.pack_start(edit_button)

        def update_preview_cb(file_chooser, preview):
            filename = file_chooser.get_preview_filename()
            preview.set_editable(False)
            has_preview = False
            try:
                with open(filename, 'r') as content:
                    buf = gtk.TextBuffer()
                    small = buf.create_tag('small')
                    small.set_property('scale', pango.SCALE_SMALL)
                    text = content.read()
                    # if text is not utf-8, will return a UnicodeDecodeError,
                    # this avoid trying to display files with wrong format
                    text.decode('utf-8')
                    buf.insert_with_tags_by_name(buf.get_start_iter(),
                                                 text, 'small')
                    preview.set_buffer(buf)
                has_preview = True
            except Exception:
                has_preview = False
            file_chooser.set_preview_widget_active(has_preview)
            return
        
        def upload_file(button, event):
            if self._source is None:
                error_popup("Missing XSD source content for file element")
                return
            dlg = gtk.FileChooserDialog(self._value + ' - OpenSAND', None,
                                        gtk.FILE_CHOOSER_ACTION_OPEN,
                                        (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                         gtk.STOCK_APPLY, gtk.RESPONSE_APPLY))
            dlg.set_size_request(500, -1)
            preview = gtk.TextView()
            scroll = gtk.ScrolledWindow()
            scroll.add(preview)
            scroll.show_all()
            scroll.set_size_request(200, -1)
            dlg.set_preview_widget(scroll)
            dlg.connect("update-preview", update_preview_cb, preview)
            dlg.set_filename(self._source)
            dlg.set_preview_widget_active(False)
            ret = dlg.run()
            if ret == gtk.RESPONSE_APPLY:
                new_filename = dlg.get_filename()
                self.global_handler()
                if self._file_handler is not None:
                    self._file_handler(new_filename, self._host, self._path)
            dlg.destroy()

        upload_button = gtk.Button(label="Upload")
        img = gtk.Image()
        img.set_from_stock(gtk.STOCK_OPEN, gtk.ICON_SIZE_BUTTON)
        upload_button.set_image(img)
        if self._source is None:
            # new line in table, not enough information
            upload_button.set_sensitive(False)
        upload_button.show()
        # show upload dialog
        upload_button.connect('button-press-event', upload_file)
        # consider file may be changed
        self._entry.pack_start(upload_button)

    def do_not_scroll(self, source=None, event=None):
        """ stop scolling in the element which emits the scroll-event signal """
        source.emit_stop_by_name('scroll-event')
        return False

    def add_completion(self, completion):
        """ add completion to a text entry """
        if self._entry.get_completion() is not None:
            return
        comp = gtk.EntryCompletion()
        liststore = gtk.ListStore(str)
        for data in completion:
            liststore.append([data])
        comp.set_model(liststore)
        self._entry.set_completion(comp)
        comp.set_text_column(0)
#        def match(completion, model, iter):
#        comp.connect('match-selected', match)
#        def activate_entry(entry, liststore):
#            text = entry.get_text()
#            if text:
#                if text not in [row[0] for row in liststore]:
#                    self.liststore.append([text])
#                    entry.set_text('')
#                    return
#        self._entry.connect('activate', activate_entry, liststore)

    def get(self):
        """ get the gtk element """
        return self._entry

    def get_name(self):
        """ get the path for entry """
        return self._path

    def get_value(self):
        """ get the value fot the entry """
        type_name = ""
        if self._type is None:
            type_name = "string"
        else:
            type_name = self._type["type"]

        if type_name == "boolean":
            if self._entry.get_active():
                return "true"
            return "false"
        elif type_name == "enum":
            model = self._entry.get_model()
            active = self._entry.get_active_iter()
            if active is None:
                return ''
            return model.get_value(active, 0)
        elif type_name == "numeric":
            return self._entry.get_text()
        elif type_name == "file":
            # the destination files should not be modified
            # except is source is None (=> new line)
            if self._source is None:
                return self._value
            return None
        else:
            return self._entry.get_text()

    def global_handler(self, source=None, event=None):
        """ handler used to abstract source type """
        for handler in [hdl for hdl in self._sig_handlers
                            if hdl is not None]:
            handler(self, event)


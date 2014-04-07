#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
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

# Author: Vincent Duvert / Viveris Technologies <vduvert@toulouse.viveris.com>


"""
event_tab.py - Used to handles event tabs
"""

from opensand_manager_core.loggers.levels import LOG_LEVELS, LEVEL_EVENT
import gobject
import gtk
import time
import pango

class EventTab(object):
    """
    Handler for a tab used to display events
    """
    def __init__(self, notebook, label, program=None):
        self._notebook = notebook
        self._program = program
        self._icon_level = 100

        # Tab
        self._act_image = gtk.Image()
        self._act_image.hide()

        tab_label = gtk.Label(label)
        tab_label.show()

        tab_hbox = gtk.HBox()
        tab_hbox.pack_start(self._act_image)
        tab_hbox.pack_start(tab_label)
        # set the label for hbox name to retrieve the tab name easily
        tab_hbox.set_name(label)
        tab_hbox.show()

        # Content
        self._buff = gtk.TextBuffer()
        bold = self._buff.create_tag('bold')
        bold.set_property('weight', pango.WEIGHT_BOLD)
        color = self._buff.create_tag('bg_green')
        color.set_property('background', 'green')
        for level in LOG_LEVELS.values():
            try:
                if level.color != '':
                    color = self._buff.create_tag(level.color)
                    color.set_property('foreground', level.color)
                if level.bg != '':
                    color = self._buff.create_tag('bg_' + level.bg)
                    color.set_property('background', level.bg)
            except TypeError:
                # the tag is duplicated
                continue

        self._text = gtk.TextView()
        self._text.set_editable(False)
        self._text.set_wrap_mode(gtk.WRAP_CHAR)
        self._text.set_cursor_visible(False)
        self._text.set_buffer(self._buff)
        self._text.show()

        hadjustement = gtk.Adjustment(value=0, lower=0, upper=100, step_incr=1,
                                      page_incr=10, page_size=10)
        vadjustement = gtk.Adjustment(value=0, lower=0, upper=100, step_incr=1,
                                      page_incr=10, page_size=10)

        scroll = gtk.ScrolledWindow(hadjustment=hadjustement,
                                    vadjustment=vadjustement)
        scroll.set_policy(gtk.POLICY_NEVER, gtk.POLICY_ALWAYS)
        scroll.add(self._text)
        scroll.set_size_request(-1, 150)
        scroll.show()

        self._page_num = notebook.append_page(scroll, tab_hbox)
        notebook.connect("switch-page", self._check_switch_page)

    def message(self, severity, identifier, text, new_line=False):
        """
        Adds a message to the event tab
        """
        now = time.localtime()
        gobject.idle_add(self._message, now, severity, identifier, text,
                         new_line)

    def _message(self, date, severity, identifier, text, new_line):
        """
        Internal handler to add a message to the event tab
        """
        at_end = self._buff.get_end_iter

        color = 'black'
        bg = 'bg_white'
        sev_text = ''
        image = gtk.STOCK_DIALOG_INFO
        if severity is not None and severity in LOG_LEVELS:
            color = LOG_LEVELS[severity].color
            bg = "bg_" + LOG_LEVELS[severity].bg
            sev_text = LOG_LEVELS[severity].msg
            image = LOG_LEVELS[severity].icon
        elif severity == LEVEL_EVENT:
            #   TODO
            sev_text = 'EVENT'
            color = 'white'
            bg = 'bg_green'
            image = gtk.STOCK_DIALOG_INFO

        if new_line:
            self._buff.insert(at_end(), "\n")

        self._buff.insert(at_end(), time.strftime("%H:%M:%S ", date))
        if identifier:
            self._buff.insert_with_tags_by_name(at_end(), "%s: " % identifier,
                                                'bold')
        if severity is not None and sev_text != '':
            self._buff.insert_with_tags_by_name(at_end(), "[%s]: " % sev_text,
                                                color, bg)
        self._buff.insert(at_end(), text.rstrip() + "\n")
        self._buff.place_cursor(at_end())
        self._text.scroll_to_mark(self._buff.get_insert(), 0.0, False, 0, 0)

        if self._notebook.get_current_page() != self._page_num:
            if severity is not None and self._icon_level > severity:
                self._icon_level = severity
                self._act_image.set_from_stock(image, gtk.ICON_SIZE_MENU)

            self._act_image.show()

        return False

    def _check_switch_page(self, _notebook, _page, page_num):
        """ Called when the notebook page is changed """
        if page_num == self._page_num:
            self._act_image.hide()
            self._icon_level = -1
            self._act_image.hide()

    def get_program(self):
        """ get the associated program """
        return self._program

    def update(self, program):
        """ update the event tab """
        self._program = program

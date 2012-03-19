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
import gtk

from platine_manager_gui.view.window_view import WindowView
from platine_manager_gui.view.utils.protocol_stack import ProtocolStack
from platine_manager_gui.view.popup.infos import error_popup
from platine_manager_core.my_exceptions import ConfException

IMG_PATH = "/usr/share/platine/manager/images/"

class ConfView(WindowView):
    """ Element for the configuration tab """

    _dama_rcs = ["Legacy", "UoR", "Stub", "Yes"]
    _dama_s2 = ["MF2-TDMA"]

    def __init__(self, parent, model, manager_log):
        WindowView.__init__(self, parent)

        self._log = manager_log

        self._model = model

        self._title = 'Protocol stack'

        self._out_stack = ProtocolStack(self._ui.get_widget('out_encap_stack'),
                                        self._model.get_modules(),
                                        self.on_stack_modif)

        self._in_stack = ProtocolStack(self._ui.get_widget('in_encap_stack'),
                                       self._model.get_modules(),
                                       self.on_stack_modif)

        self._ip_options = {}

        self._drawing_area = self._ui.get_widget('repr_stack_links')
        pango = self._drawing_area.create_pango_layout("")
        self._drawing_area.connect("expose-event", self.draw_links)
        style = self._drawing_area.get_style()
        self._context_graph = style.fg_gc[gtk.STATE_NORMAL]

        # add ip options
        vbox = self._ui.get_widget('ip_options_vbox')
        modules = self._model.get_modules()
        ip_option = False
        for module in modules:
            if modules[module].get_condition('ip_option'):
                ip_option = True
                button = gtk.CheckButton(label=module.upper())
                button.connect('clicked', self.on_button_clicked)
                button.set_name(module)
                vbox.pack_start(button)
                self._ip_options[module] = button
        vbox.show_all()
        if not ip_option:
            self._ui.get_widget('frame_ip_options').hide()

        # update view from model
        # do not load it in gobject.idle_add because we won't be able to catch
        # the exception
        self.update_view()

    def update_view(self):
        """ update the configuration view according to model
            (should be used with gobject.idle_add outside gtk handlers) """
        # main config parameters
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
        # ip_options
        for button in self._ip_options.values():
            button.set_active(False)
        for active in config.get_ip_options():
            try:
                self._ip_options[active].set_active(True)
            except KeyError, msg:
                self._log.error("cannot fine IP option %s" % active)
        # up_return_encap
        self._out_stack.load(config.get_up_return_encap(),
                             config.get_payload_type())
        # down_forward_encap
        self._in_stack.load(config.get_down_forward_encap(),
                            config.get_payload_type())
        # terminal_type
        widget = self._ui.get_widget(config.get_terminal_type())
        widget.set_active(True)
        widget.clicked()
        # frame_duration
        widget = self._ui.get_widget('FrameDuration')
        widget.set_value(int(config.get_frame_duration()))

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
            active = widget.get_active_iter()
            if model.get_value(active, 0) != config.get_dama():
                return True
            for option in [opt for opt in self._ip_options
                               if self._ip_options[opt].get_active()]:
                if not option in config.get_ip_options():
                    return True
            # up_return_encap
            if self._out_stack.get_stack() != config.get_up_return_encap():
                return True

            # down_forward_encap
            if self._in_stack.get_stack() != config.get_down_forward_encap():
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

    def draw_links(self, drawing_area=None, event=None):
        """ draw the links in the protocol stack representation """
        yorig = 2
        ydst = 17
        self._context_graph.set_line_attributes(line_width=1,
                                                line_style=gtk.gdk.LINE_SOLID,
                                                cap_style=gtk.gdk.CAP_NOT_LAST,
                                                join_style=gtk.gdk.JOIN_MITER)
        # ST -> SAT
        widget = self._ui.get_widget("repr_stack_st")
        xsrc = widget.allocation.x 
        widget = self._ui.get_widget("repr_stack_sat")
        xdst = widget.allocation.x 
        widget = self._ui.get_widget("repr_stack_out_st")
        diff = 25 + widget.allocation.x - xsrc
        self._drawing_area.window.draw_lines(self._context_graph,
                                             [(diff, yorig), (diff, ydst),
                                              (xdst - xsrc + diff - 5, ydst),
                                              (xdst - xsrc + diff - 5, yorig)])
        # draw arrow
        self._drawing_area.window.draw_polygon(self._context_graph, True,
                                               [(xdst - xsrc + diff - 10,
                                                 yorig + 10),
                                                (xdst - xsrc + diff,
                                                 yorig + 10),
                                                (xdst - xsrc + diff - 5,
                                                 yorig - 2)])
        orig = xdst - xsrc + diff + 10
        # GW <-> SAT
        widget = self._ui.get_widget("repr_stack_sat")
        xsrc = widget.allocation.x 
        widget = self._ui.get_widget("repr_stack_gw")
        xdst = widget.allocation.x 
        self._drawing_area.window.draw_lines(self._context_graph,
                                             [(orig, yorig), (orig, ydst),
                                              (xdst - xsrc + orig - 5, ydst),
                                              (xdst - xsrc + orig - 5, yorig)])
        # draw arrow
        if self.is_button_active('transparent'):
            self._drawing_area.window.draw_polygon(self._context_graph, True,
                                                   [(xdst - xsrc + orig - 10,
                                                     yorig + 10),
                                                    (xdst - xsrc + orig,
                                                     yorig + 10),
                                                    (xdst - xsrc + orig - 5,
                                                     yorig - 2)])
        elif self.is_button_active('regenerative'):
            self._drawing_area.window.draw_polygon(self._context_graph, True,
                                                   [(orig - 5, yorig + 10),
                                                    (orig + 5, yorig + 10),
                                                    (orig, yorig - 2)])

        ydst = 34
        self._context_graph.set_line_attributes(line_width=2,
                                                line_style=gtk.gdk.LINE_ON_OFF_DASH,
                                                cap_style=gtk.gdk.CAP_NOT_LAST,
                                                join_style=gtk.gdk.JOIN_MITER)

        # SAT -> ST
        widget = self._ui.get_widget("repr_stack_st")
        xsrc = widget.allocation.x 
        widget = self._ui.get_widget("repr_stack_sat")
        xdst = widget.allocation.x 
        widget = self._ui.get_widget("repr_stack_out_st")
        diff = 85 + widget.allocation.x - xsrc
        self._drawing_area.window.draw_lines(self._context_graph,
                                             [(diff, yorig), (diff, ydst),
                                              (xdst - xsrc + diff - 5, ydst),
                                              (xdst - xsrc + diff - 5, yorig)])
        # draw arrow
        self._drawing_area.window.draw_polygon(self._context_graph, True,
                                               [(diff - 5, yorig + 10),
                                                (diff + 5, yorig + 10),
                                                (diff, yorig - 2)])

        orig = xdst - xsrc + diff + 10
        # GW <-> SAT
        widget = self._ui.get_widget("repr_stack_sat")
        xsrc = widget.allocation.x 
        widget = self._ui.get_widget("repr_stack_gw")
        xdst = widget.allocation.x 
        self._drawing_area.window.draw_lines(self._context_graph,
                                             [(orig, yorig), (orig, ydst),
                                              (xdst - xsrc + orig - 5, ydst),
                                              (xdst - xsrc + orig - 5, yorig)])
        # draw arrow
        if self.is_button_active('transparent'):
            self._drawing_area.window.draw_polygon(self._context_graph, True,
                                                   [(orig - 5, yorig + 10),
                                                    (orig + 5, yorig + 10),
                                                    (orig, yorig - 2)])
        elif self.is_button_active('regenerative'):
            self._drawing_area.window.draw_polygon(self._context_graph, True,
                                                   [(xdst - xsrc + orig - 10,
                                                     yorig + 10),
                                                    (xdst - xsrc + orig,
                                                     yorig + 10),
                                                    (xdst - xsrc + orig - 5,
                                                     yorig - 2)])


    def on_stack_modif(self, source=None, event=None):
        """ 'changed' event on a combobox from the stack """
        self.enable_conf_buttons()

    def enable_conf_buttons(self):
        """ defined in conf_event """
        pass

    def on_button_clicked(self, source, event=None):
        """ defined in conf_event """
        pass

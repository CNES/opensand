#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
# Copyright © 2013 CNES
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
conf_view.py - the configuration tab view
"""

import gtk
import gobject

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.utils.protocol_stack import ProtocolStack

IMG_PATH = "/usr/share/opensand/manager/images/"

class ConfView(WindowView):
    """ Element for the configuration tab """

#    _dama_rcs = ["Legacy"]
#    _dama_s2 = ["MF2-TDMA"]

    def __init__(self, parent, model, manager_log):
        WindowView.__init__(self, parent)

        self._log = manager_log

        self._model = model

        # dictionnary with one protocol stack per terminal
        self._lan_stack_notebook =  self._ui.get_widget('lan_adapt_notebook')
        self._lan_stacks = {}

        self._out_stack = ProtocolStack(self._ui.get_widget('out_encap_stack'),
                                        self._model.get_encap_modules(),
                                        self.on_stack_modif)

        self._in_stack = ProtocolStack(self._ui.get_widget('in_encap_stack'),
                                       self._model.get_encap_modules(),
                                       self.on_stack_modif)

        self._drawing_area = self._ui.get_widget('repr_stack_links')
        self._drawing_area.connect("expose-event", self.draw_links)
        style = self._drawing_area.get_style()
        self._context_graph = style.fg_gc[gtk.STATE_NORMAL]

        # update view from model
        # do not load it in gobject.idle_add because we won't be able to catch
        # the exception
        self.update_view()

        self._timeout_id = gobject.timeout_add(1000, self.update_lan_adaptation)

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
        # dama TODO support up/down DAMA ?
        widget = self._ui.get_widget("frame_dama")
        widget.hide() # TODO remove
#        widget = self._ui.get_widget('dama_box')
#        if config.get_emission_std() == "DVB-S2":
#            widget.set_active(self._dama_s2.index(config.get_dama()))
#        else:
#            widget.set_active(self._dama_rcs.index(config.get_dama()))
#        for host in self._lan_stacks:
#            self._lan_stacks[host].load(host.get_lan_adaptation())
        # up_return_encap
        self._out_stack.load(config.get_up_return_encap(),
                             config.get_payload_type(),
                             config.get_emission_std())
        # down_forward_encap
        self._in_stack.load(config.get_down_forward_encap(),
                            config.get_payload_type(),
                            "DVB-S2")
        # frame_duration
        widget = self._ui.get_widget('frame_duration')
        widget.set_value(int(config.get_frame_duration()))
        # physical layer
        widget = self._ui.get_widget('enable_physical_layer')
        if config.get_enable_physical_layer().lower() == "true":
            widget.set_active(True)
        else:
            widget.set_active(False)

    def update_lan_adaptation(self):
        """ update the lan adaptation notebook """
        # add new hosts
        for host in self._model.get_hosts_list():
            if host in self._lan_stacks:
                continue
            name = host.get_name().lower()
            if name.startswith('st') or name == 'gw':
                vbox = gtk.VBox()
                label = gtk.Label(name.upper())
                self._lan_stack_notebook.append_page(vbox, label)
                header_modif = self._model.get_global_lan_adaptation_modules()
                plugins = dict(host.get_lan_adapt_modules())
                plugins.update(header_modif)
                stack  = ProtocolStack(vbox,
                                       plugins,
                                       self.on_stack_modif,
                                       self._ui.get_widget('header_modif_vbox'),
                                       self._ui.get_widget('frame_header_modif'))
                self._lan_stacks[host] = stack
                stack.load(host.get_lan_adaptation())
        # remove old hosts
        remove = []
        for host in self._lan_stacks:
            if not host in self._model.get_hosts_list():
                page = \
                    self._lan_stack_notebook.page_num(self._lan_stacks[host].get_box())
                self._lan_stack_notebook.remove_page(page)
                remove.append(host)
        # delete here to avoid changing dictionnary size in iteration
        for host in remove:
            del self._lan_stacks[host]
        return True

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
            # dama TODO
#            widget = self._ui.get_widget('dama_box')
#            model = widget.get_model()
#            active = widget.get_active_iter()
#            if model.get_value(active, 0) != config.get_dama():
#                return True

            # lan adaptation
            for host in self._lan_stacks:
                if self._lan_stacks[host].get_stack() != host.get_lan_adaptation():
                    return True

            # up_return_encap
            if self._out_stack.get_stack() != config.get_up_return_encap():
                return True

            # down_forward_encap
            if self._in_stack.get_stack() != config.get_down_forward_encap():
                return True

            # frame_duration
            widget = self._ui.get_widget('frame_duration')
            if widget.get_text() == int(config.get_frame_duration()):
                return True
            # enable physical_layer
            widget = self._ui.get_widget('enable_physical_layer')
            if widget.get_active() and \
               config.get_enable_physical_layer().lower() != "true":
                return True
            if not widget.get_active() and \
               config.get_enable_physical_layer().lower() != "false":
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

    def enable_conf_buttons(self, enable=True):
        """ defined in conf_event """
        pass

    def on_button_clicked(self, source, event=None):
        """ defined in conf_event """
        pass

    def is_button_active(self, button):
        """ check if a button is active """
        widget = self._ui.get_widget(button)
        return widget.get_active()



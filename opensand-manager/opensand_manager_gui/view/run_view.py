#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
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
run_view.py - the run tab view
"""

import gtk
import os
import time

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_core.my_exceptions import RunException

COMPO_X = 70
COMPO_Y = 69
LED_XY = 15
SAT_X = 170
TOP_1 = 50
TOP_2 = 200
TOP_3 = 350
TOOL_X = 20
TOOL_Y = 20
INFO_X = 600
LEGEND_Y = TOP_2

IMG_PATH = "/usr/share/opensand/manager/images/"

class RunView(WindowView):
    """ Element of the run tab """
    def __init__(self, parent, model, manager_log):
        WindowView.__init__(self, parent)

        self._initok = False
        self._model = model
        self._log = manager_log
        # wait before saying you cannot connect to a host
        self._count = 0
        # only log one time connection problem
        self._logged = False

        self._opensand_buff = None
        self._drawing_area = None
        self._stylepango = None
        self._context_graph = None

        self._sat_x = SAT_X
        self._info_x = INFO_X
        self._legend_y = LEGEND_Y

        # set the OpenSAND event and error buffer
        self._opensand_buff = gtk.TextBuffer()
        red = self._opensand_buff.create_tag('red')
        red.set_property('foreground', 'red')
        orange = self._opensand_buff.create_tag('orange')
        orange.set_property('foreground', 'orange')
        self._ui.get_widget('opensand_textview').set_buffer(self._opensand_buff)

        # init drawing area
        self._drawing_area = self._ui.get_widget('main_drawing_area')
        self._stylepango = self._drawing_area.create_pango_layout("")

        self._drawing_area.connect("expose-event", self.expose_handler)
        style = self._drawing_area.get_style()
        self._context_graph = style.fg_gc[gtk.STATE_NORMAL]

        # check that image path exist
        if not os.path.exists(IMG_PATH):
            self._log.error("image path '%s' does not exist" % IMG_PATH)
            raise RunException("image path '%s' does not exist" %
                               IMG_PATH)

    def expose_handler(self, drawing_area, event):
        """ 'expose' event handler on drawing area """
        self._context_graph.set_line_attributes(line_width=1,
                                                line_style=gtk.gdk.LINE_SOLID,
                                                cap_style=gtk.gdk.CAP_NOT_LAST,
                                                join_style=gtk.gdk.JOIN_MITER)

        self._legend_y = TOP_2
        nbr_st = 0
        self._sat_x = 170
        for host in self._model.get_hosts_list():
            if host.get_component() == 'st':
                nbr_st += 1
            if host.get_component() == 'gw':
                nbr_st += 1
                self._sat_x = 30

        if nbr_st > 1:
            self._sat_x = COMPO_X  * nbr_st - 35

        nbr = 0
        for host in self._model.get_hosts_list():
            if host.get_component() == 'gw':
                self.draw_gw(host, 30, TOP_2)
            elif host.get_component() == 'sat':
                self.draw_sat(host, self._sat_x, TOP_1)
            elif host.get_component() == 'st':
                self.draw_st(host, 170 + nbr * 140, TOP_2)
                nbr += 1

        self._info_x = 170 + (nbr + 1) * 140
        self.draw_env_plane_state(self._model.is_collector_known())

        return False

    def draw_env_plane_state(self, collector_known):
        """ draw environment plane """
        image = gtk.Image()
        
        name = "monitoring.png" if collector_known else "monitoring_grey.png"
        png = os.path.join(IMG_PATH, name)
        image.set_from_file(png)

        self.draw_pixbuf(0, 0, self._info_x, TOP_1, COMPO_X, COMPO_Y, image)

        self._stylepango.set_text('Collector')
        self.draw_layout(self._info_x + 5, TOP_1 + COMPO_Y)

    def draw_st(self, host, x, y):
        """ draw satellite terminal """
        image = gtk.Image()
        png = os.path.join(IMG_PATH, 'st.png')
        if host.get_state() is None:
            # TODO we could publish a manager service to check if this is the
            # only manager instance
            self._count += 1
            if not self._logged and self._count > 3:
                self._log.warning("Cannot get %s status, maybe another "
                                  "sand-manager instance is running on the system,"
                                  " else try restarting sand-daemon on "
                                  "distant host" % host.get_name())
                self._logged = True
            png = os.path.join(IMG_PATH, 'st_grey.png')
        image.set_from_file(png)
        self.draw_pixbuf(0, 0, x, y, COMPO_X, COMPO_Y, image)
        self._stylepango.set_text('ST ' + str(host.get_instance()))
        self.draw_layout(x + 10, y + COMPO_Y)
        self.draw_state(host.get_state(), host.get_initialisation_failed(), x, y)
        self.draw_tools(host, x + COMPO_X + 4, y)

        # get satellite
        sat = self._model.get_host('sat')

        if host.get_state():
            if sat is not None and sat.get_state():
                self.draw_line(self._sat_x + int(COMPO_X/2),
                               TOP_1 + COMPO_Y + 20,
                               x + int(COMPO_X/2), y - 10)
            self.draw_line(x + int(COMPO_X/2),
                           y + COMPO_Y + 20,
                           x + int(COMPO_X/2),
                           TOP_3 - 10)
            image = gtk.Image()
            image.set_from_file(os.path.join(IMG_PATH, 'network.png'))
            self.draw_pixbuf(0, 0, x, TOP_3, COMPO_X, COMPO_Y, image)

        self.draw_ws(host, host.get_instance(), x, y)

    def draw_sat(self, host, x, y):
        """ draw satellite """
        image = gtk.Image()
        png = os.path.join(IMG_PATH, 'sat.png')
        if host.get_state() is None:
            self._count += 1
            if not self._logged and self._count > 3:
                self._log.warning("Cannot get %s status, maybe another "
                                  "sand-manager instance is running on the system,"
                                  " else try restarting sand-daemon on "
                                  "distant host" % host.get_name())
                self._logged = True
            png = os.path.join(IMG_PATH, 'sat_grey.png')
        image.set_from_file(png)
        self.draw_pixbuf(0, 0, x, y, COMPO_X, COMPO_Y, image)
        self._stylepango.set_text('Satellite')
        self.draw_layout(x + 10, y + COMPO_Y)
        self.draw_state(host.get_state(), host.get_initialisation_failed(), x, y)
        self.draw_tools(host, x + COMPO_X + 4, y)

    def draw_gw(self, host, x, y):
        """ draw gateway """
        image = gtk.Image()
        png = os.path.join(IMG_PATH, 'gw.png')
        if host.get_state() is None:
            self._count += 1
            if not self._logged and self._count > 3:
                self._log.warning("Cannot get %s status, maybe another "
                                  "sand-manager instance is running on the system,"
                                  " else try restarting sand-daemon on "
                                  "distant host" % host.get_name())
                self._logged = True
            png = os.path.join(IMG_PATH, 'gw_grey.png')
        image.set_from_file(png)
        self.draw_pixbuf(0, 0, x, y, COMPO_X, COMPO_Y, image)
        self._stylepango.set_text('Gateway (GW)')
        self.draw_layout(x + 10, y + COMPO_Y)
        self.draw_state(host.get_state(), host.get_initialisation_failed(), x, y)

        self.draw_tools(host, x + COMPO_X + 4, y)

        # get satellite
        sat = self._model.get_host('sat')

        if host.get_state() and \
           sat is not None and sat.get_state():
            self.draw_line(self._sat_x + int(COMPO_X/2),
                           TOP_1 + COMPO_Y + 20,
                           x + int(COMPO_X/2), y - 10)

        self.draw_ws(host, '2', x, y)

    def draw_ws(self, host, host_inst, x, y):
        """ draw the workstations """
        ws_nbr = 0
        for workstation in self._model.get_workstations_list():
            (inst, ident) = workstation.get_instance().split('_', 1)
            if inst == host_inst:
                ws_nbr += 1
                self._stylepango.set_text("- " + ident)
                self.draw_layout(x + 10, TOP_3 + COMPO_Y + 2 + ws_nbr * 10)
                tool_list = ''
                for tool in [tools for tools in workstation.get_tools()
                                   if tools.get_state()]:
                    tool_list = tool_list + tool.get_name() + ', '
                if tool_list != '':
                    tool_list = '(%s)' % tool_list.rstrip(', ')
                    self._stylepango.set_text(tool_list)
                    self.draw_layout(x + 15 + len(ident) * 7,
                                     TOP_3 + COMPO_Y + 2 + ws_nbr * 10)

        if ws_nbr > 0:
            self._stylepango.set_text(str(ws_nbr) + " workstation(s):")
            self.draw_layout(x, TOP_3 + COMPO_Y)

            if host.get_state() and host_inst == '2':
                self.draw_line(x + int(COMPO_X/2),
                               y + COMPO_Y + 20,
                               x + int(COMPO_X/2),
                               TOP_3 - 10)
                image = gtk.Image()
                image.set_from_file(IMG_PATH + 'network.png')
                self.draw_pixbuf(0, 0, x, TOP_3, COMPO_X, COMPO_Y, image)


    def draw_tools(self, host, x, y):
        """ draw the started tools for the specified host """
        for tool in [tools for tools in host.get_tools()
                           if tools.get_state()]:
            png = "%s/tools/%s.png" % (IMG_PATH, tool.get_name())
            if os.path.exists(png):
                image = gtk.Image()
                image.set_from_file(png)
                self.draw_pixbuf(0, 0, x, y, TOOL_X, TOOL_Y, image)
                self.draw_pixbuf(0, 0, self._info_x, self._legend_y,
                                 TOOL_X, TOOL_Y, image)
                self._stylepango.set_text(tool.get_name().upper())
                self.draw_layout(self._info_x + 25, self._legend_y + 5)
                self._legend_y += 28
            else:
                self._stylepango.set_text(tool.get_name().upper())
                self.draw_layout(x, y)
            y = y + TOOL_Y + 2

    def draw_state(self, state, initialisation_failed, x, y):
        """ draw component state """
        if state is None:
            return

        image = gtk.Image()
        if state == True:
            image.set_from_file(IMG_PATH + 'green.png')
        elif state == False:
            if initialisation_failed == True:
                image.set_from_file(IMG_PATH + 'orange.png')
            elif initialisation_failed == False:
                image.set_from_file(IMG_PATH + 'red.png')

        self.draw_pixbuf(0, 0, x - 9, y + 71, LED_XY, LED_XY, image)
        return

    def draw_line(self, src_x, src_y, dst_x, dst_y):
        """ draw a line on the drawing area """
        self._drawing_area.window.draw_line(self._context_graph,
                                            src_x, src_y, dst_x, dst_y)


    def draw_pixbuf(self, src_x, src_y, dst_x, dst_y, width, heigth, image):
        """ draw an image on the drawing area """
        self._drawing_area.window.draw_pixbuf(self._context_graph,
                                              image.get_pixbuf(),
                                              src_x, src_y, dst_x, dst_y,
                                              width, heigth,
                                              gtk.gdk.RGB_DITHER_NORMAL, 0, 0)


    def draw_layout(self, x, y):
        """ draw an area layout on the drawing area """
        self._drawing_area.window.draw_layout(self._context_graph,
                                              x, y, self._stylepango)

    def show_opensand_event(self, text):
        """ print OpenSAND events in OpenSAND textview
            (should be used with gobject.idle_add outside gtk handlers) """
        if text != "":
            self._log.debug("OpenSAND event: " + text)
            self._opensand_buff.insert(self._opensand_buff.get_end_iter(),
                                     time.strftime("%H:%M:%S ", time.gmtime()))
        self._opensand_buff.insert(self._opensand_buff.get_end_iter(),
                                  text + '\n')
        self._opensand_buff.place_cursor(self._opensand_buff.get_end_iter())
        self._ui.get_widget('opensand_textview').scroll_to_mark(
                self._opensand_buff.get_insert(), 0.0, False, 0, 0)
        
        # show info image if page is not active
        if text != "" and \
           self._ui.get_widget('event_notebook').get_current_page() != 1:
            img = self._ui.get_widget('img_opensand')
            img.show()

    def show_opensand_error(self, text, color = None):
        """ print OpenSAND errors in OpenSAND textview
            (should be used with gobject.idle_add outside gtk handlers) """
        self._log.debug("OpenSAND error: " + text)
        self._opensand_buff.insert(self._opensand_buff.get_end_iter(),
                                  time.strftime("%H:%M:%S ", time.gmtime()))
        if color != None:
            self._opensand_buff.insert_with_tags_by_name(
                    self._opensand_buff.get_end_iter(), '[ERROR] ', color)
        self._opensand_buff.insert(self._opensand_buff.get_end_iter(),
                                  text + '\n')
        self._opensand_buff.place_cursor(self._opensand_buff.get_end_iter())
        self._ui.get_widget('opensand_textview').scroll_to_mark(
        self._opensand_buff.get_insert(), 0.0, False, 0, 0)
        
        # show warning image if page is not active
        if self._ui.get_widget('event_notebook').get_current_page() != 1:
            img = self._ui.get_widget('img_opensand')
            if color is not None:
                img.set_from_stock(gtk.STOCK_DIALOG_WARNING,
                                   gtk.ICON_SIZE_MENU)
            img.show()

    def update_status(self):
        """ update the status of the different component
            (should be used with gobject.idle_add outside gtk handlers) """
        self._drawing_area.queue_draw()

    def set_run_id(self):
        """ configure run ID (for file storage)
            (should be used with gobject.idle_add outside gtk handlers) """
        widget = self._ui.get_widget('run_id_txt')
        run_id = widget.get_text()
        if run_id == '':
            run_id = 'default_0'
        if run_id.startswith("default_"):
            try:
                nbr = int(run_id[(run_id.find('_') + 1):])
                # increment the run_id to avoid overwritting the previous one
                nbr += 1
                run_id = "default_"  + str(nbr)
                widget.set_text(run_id)
            except Exception, msg:
                self._log.warning("Default Run ID will be overwritten "
                                  "due to exception (%s)" % str(msg))

        self._model.set_run(run_id)

    def set_start_stop_button(self):
        """ modify the start button label according to OpenSAND status
            (should be used with gobject.idle_add outside gtk handlers) """
        if self.is_running():
            btn = self._ui.get_widget('start_opensand_button')
            btn.set_label('Stop OpenSAND')
        else:
            btn = self._ui.get_widget('start_opensand_button')
            btn.set_label('Start OpenSAND')

    def disable_start_button(self, status):
        """ set start button sensitive or not
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('start_opensand_button').set_sensitive(not status)

    def disable_deploy_button(self, status):
        """ set deploy button sensitive or not
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('deploy_opensand_button').set_sensitive(not status)

    def hide_deploy_button(self, status = True):
        """ hide or not deploy button
            (should be used with gobject.idle_add outside gtk handlers) """
        widget = self._ui.get_widget('deploy_opensand_button')
        if status:
            widget.hide()
        else:
            widget.show()

    def is_running(self):
        """ check if at least one host or controller is running """
        return self._model.is_running()
    


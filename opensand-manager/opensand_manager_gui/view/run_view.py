#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 TAS
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
run_view.py - the run tab view
"""

import gtk
import gobject
import os

from opensand_manager_core.utils import OPENSAND_PATH, ST, SAT, GW, SPOT
from opensand_manager_core.my_exceptions import RunException
from opensand_manager_core.model.host import InitStatus
from opensand_manager_core.model.environment_plane import Program

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.config_logs_dialog import ConfigLogsDialog
from opensand_manager_gui.view.popup.conf_debug_dialog import ConfigDebugDialog
from opensand_manager_gui.view.event_tab import EventTab

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

IMG_PATH = OPENSAND_PATH + "manager/images/"

class RunView(WindowView):
    """ Element of the run tab """
    def __init__(self, parent, model, manager_log, service_type):
        WindowView.__init__(self, parent)

        self._initok = False
        self._model = model
        self._log = manager_log
        # wait before saying you cannot connect to a host
        self._count = 0
        # only log one time connection problem
        self._logged = False

        self._drawing_area = None
        self._stylepango = None
        self._context_graph = None

        self._sat_x = SAT_X
        self._info_x = INFO_X
        self._legend_y = LEGEND_Y

        # init drawing area
        self._drawing_area = self._ui.get_widget('main_drawing_area')
        self._stylepango = self._drawing_area.create_pango_layout("")

        self._drawing_area.connect("expose-event", self.expose_handler)
        style = self._drawing_area.get_style()
        self._context_graph = style.fg_gc[gtk.STATE_NORMAL]
        self._copy_context = self._drawing_area.window.new_gc(subwindow_mode=
                                              gtk.gdk.INCLUDE_INFERIORS)
        self._color_list = {}
        self._colors = ["#33CC66",
                        "#00CCFF",
                        "#CC3333",
                        "#6600CC",
                        "#FF9933"]
        self._it_color = 0

        self._log_view = LogView(self.get_current(), manager_log)

        # check that image path exist
        if not os.path.exists(IMG_PATH):
            self._log.error("image path '%s' does not exist" % IMG_PATH)
            raise RunException("image path '%s' does not exist" %
                               IMG_PATH)

        # set run_id if a run was already running at launch
        widget = self._ui.get_widget('run_id_txt')
        run_id = self._model.get_run()
        if run_id != 'default':
            widget.set_text(run_id)


    def expose_handler(self, drawing_area, event):
        """ 'expose' event handler on drawing area """
        self._context_graph.set_line_attributes(line_width=1,
                                                line_style=gtk.gdk.LINE_SOLID,
                                                cap_style=gtk.gdk.CAP_NOT_LAST,
                                                join_style=gtk.gdk.JOIN_MITER)

        self._copy_context.set_line_attributes(line_width=2,
                                       line_style=gtk.gdk.LINE_SOLID,
                                       cap_style=gtk.gdk.CAP_NOT_LAST,
                                       join_style=gtk.gdk.JOIN_MITER)
        
        self._legend_y = TOP_2
        list_gw = []
        list_spot = {}
        nbr_st = 0
        self._sat_x = 170
        for host in self._model.get_all_hosts_list():
            if host.get_component() == ST:
                nbr_st += 1
                spot = host.get_spot_id()
                if spot not in list_spot:
                    list_host = [host]
                    list_spot[spot] = list_host 
                else:
                    list_spot[spot].append(host)
                    
            if host.get_component() == GW:
                self._sat_x = 30
                gw = host.get_instance()
                if gw not in list_gw:
                    list_gw.append(gw)

        for instance in list_gw:
            name = GW
            if name+instance not in self._color_list:
                color = self._colors[self._it_color]
                self._color_list[name+instance] = color               
                self._it_color += 1
        
        for instance in list_spot:
            name = SPOT
            if name+instance not in self._color_list:
                color = self._colors[self._it_color]
                self._color_list[name+instance] = color               
                self._it_color += 1


        if nbr_st >= 1 or len(list_gw) >= 1 :
            self._sat_x = COMPO_X  * nbr_st + COMPO_X * len(list_gw) - 35

        nb_gw = 0
        nbr = len(list_gw) - 1
        for host in self._model.get_all_hosts_list():
            if host.get_component() == GW:
                self.draw_gw(host, 30 + nb_gw * 140, TOP_2)
                nb_gw += 1
            elif host.get_component() == SAT:
                self.draw_sat(host, self._sat_x, TOP_1)

        for spot_id in list_spot:
            nb_st = len(list_spot[spot_id])
            self._copy_context.set_rgb_fg_color(gtk.gdk.color_parse(
                self._color_list["spot"+spot_id]))
            self._drawing_area.window.draw_arc(self._copy_context, False, 
                                               140 + nbr * 145 , TOP_2 - 45, 
                                               nb_st * 140, TOP_2 - 15, 
                                               0, 360*64)
            
            self._stylepango.set_text('SPOT'+ str(spot_id) )
            x = 110 +  nbr * 145 + ( nb_st * 140 ) / 2
            self._drawing_area.window.draw_layout(self._copy_context,
                                                  x, TOP_2 - 25 , 
                                                  self._stylepango)

            for st in list_spot[spot_id]:
                self.draw_st(st, 170 + nbr * 145, TOP_2)
                nbr += 1

        self._info_x = 170 + (nbr + 1) * 140
        self.draw_collector_state(self._model.is_collector_known(),
            self._model.is_collector_functional())

        self._log_view.update(self._model.get_all_hosts_list())
        
        return False

    def draw_collector_state(self, collector_known, collector_funct):
        """ draw collector """
        if not collector_known:
            return
        
        image = gtk.Image()
        
        name = "monitoring.png" if collector_funct else "monitoring_grey.png"
        png = os.path.join(IMG_PATH, name)
        image.set_from_file(png)

        self.draw_pixbuf(0, 0, self._info_x, TOP_1, COMPO_X, COMPO_Y, image)

        self._stylepango.set_text('Collector')
        self.draw_layout(self._info_x + 5, TOP_1 + COMPO_Y)

    def draw_st(self, host, x, y):
        """ draw satellite terminal """
        image = gtk.Image()
        png = os.path.join(IMG_PATH, 'st.png')
        if not host.is_complete():
            png = os.path.join(IMG_PATH, 'st_grey.png')
            self._count += 1
            if self._count < 2:
                self._log.warning("Host %s is not complete" % host.get_name())
        elif host.get_state() is None:
            # TODO we could publish a manager service to check if this is the
            # only manager instance
            self._count += 1
            if not self._logged and self._count > 3:
                self._log.warning("Cannot get %s status, maybe another "
                                  "sand-manager instance is running on the "
                                  "system, else try restarting sand-daemon on "
                                  "distant host" % host.get_name())
                self._logged = True
            png = os.path.join(IMG_PATH, 'st_grey.png')
        image.set_from_file(png)
        self.draw_pixbuf(0, 0, x, y, COMPO_X, COMPO_Y, image)
        self._stylepango.set_text("ST" + str(host.get_instance()))
        self.draw_layout(x + 10, y + COMPO_Y)
        self.draw_state(host.get_state(), host.get_init_status(), x, y)
        self.draw_tools(host, x + COMPO_X + 4, y)

        # get satellite
        sat = self._model.get_host(SAT)
        if host.get_state():
            if sat is not None and sat.get_state():
                self.draw_line(self._sat_x + int(COMPO_X/2),
                               TOP_1 + COMPO_Y + 20,
                               x + int(COMPO_X/2), y - 10,
                               self._color_list["gw"+host.get_gw_id()])
                
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
        if not host.is_complete():
            png = os.path.join(IMG_PATH, 'sat_grey.png')
            self._count += 1
            if self._count < 2:
                self._log.warning("Host %s is not complete" % host.get_name())
        elif host.get_state() is None:
            self._count += 1
            if not self._logged and self._count > 3:
                self._log.warning("Cannot get %s status, maybe another "
                                  "sand-manager instance is running on the "
                                  "system,  else try restarting sand-daemon on "
                                  "distant host" % host.get_name())
                self._logged = True
            png = os.path.join(IMG_PATH, 'sat_grey.png')
        image.set_from_file(png)
        self.draw_pixbuf(0, 0, x, y, COMPO_X, COMPO_Y, image)
        self._stylepango.set_text('Satellite')
        self.draw_layout(x + 10, y + COMPO_Y)
        self.draw_state(host.get_state(), host.get_init_status(), x, y)
        self.draw_tools(host, x + COMPO_X + 4, y)

    def draw_gw(self, host, x, y):
        """ draw gateway """
        image = gtk.Image()
        png = os.path.join(IMG_PATH, 'gw.png')
        if not host.is_complete():
            png = os.path.join(IMG_PATH, 'gw_grey.png')
            self._count += 1
            if self._count < 2:
                self._log.warning("Host %s is not complete" % host.get_name())
        elif host.get_state() is None:
            self._count += 1
            if not self._logged and self._count > 3:
                self._log.warning("Cannot get %s status, maybe another "
                                  "sand-manager instance is running on the "
                                  "system,  else try restarting sand-daemon on "
                                  "distant host" % host.get_name())
                self._logged = True
            png = os.path.join(IMG_PATH, 'gw_grey.png')
        image.set_from_file(png)
        self.draw_pixbuf(0, 0, x, y, COMPO_X, COMPO_Y, image)
        self._stylepango.set_text('GW'+ str(host.get_instance()) )
        self.draw_layout(x + 10, y + COMPO_Y)
        self.draw_state(host.get_state(), host.get_init_status(), x, y)

        self.draw_tools(host, x + COMPO_X + 4, y)

        # get satellite
        sat = self._model.get_host(SAT)

        if host.get_state() and \
           sat is not None and sat.get_state():
            self.draw_line(self._sat_x + int(COMPO_X/2),
                           TOP_1 + COMPO_Y + 20,
                           x + int(COMPO_X/2), y - 10,
                           self._color_list["gw"+host.get_instance()])

        self.draw_ws(host, '0', x, y)

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

    def draw_state(self, state, init_status, x, y):
        """ draw component state """
        if state is None:
            return

        image = gtk.Image()
        if state:
            if init_status == InitStatus.PENDING:
                image.set_from_file(IMG_PATH + 'blue.png')
            elif init_status == InitStatus.FAIL:
                image.set_from_file(IMG_PATH + 'orange.png')
            else:
                image.set_from_file(IMG_PATH + 'green.png')
        else:
            if init_status == InitStatus.FAIL:
                image.set_from_file(IMG_PATH + 'orange.png')
            else:
                image.set_from_file(IMG_PATH + 'red.png')

        self.draw_pixbuf(0, 0, x - 9, y + 71, LED_XY, LED_XY, image)
        return

    def draw_line(self, src_x, src_y, dst_x, dst_y, color = None):
        """ draw a line on the drawing area """
        if color is None:
            cg = self._context_graph
        else:
            cg = self._copy_context
            self._copy_context.set_rgb_fg_color(gtk.gdk.color_parse(color))
        self._drawing_area.window.draw_line(cg, src_x, src_y, dst_x, dst_y)


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
        btn = self._ui.get_widget('start_opensand_button')
        menu_btn = self._ui.get_widget('start_stop')
        if self.is_running():
            btn.set_label('Stop OpenSAND')
            menu_btn.set_label('Stop OpenSAND')
        else:
            btn.set_label('Start OpenSAND')
            menu_btn.set_label('Start OpenSAND')

    def disable_start_button(self, status):
        """ set start button sensitive or not
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('start_opensand_button').set_sensitive(not status)
        self._ui.get_widget('start_stop').set_sensitive(not status)

    def disable_deploy_buttons(self, status):
        """ set deploy and intall buttons sensitive or not
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('deploy_opensand_button').set_sensitive(not status)
        self._ui.get_widget('deploy').set_sensitive(not status)
        self._ui.get_widget('edit').set_sensitive(not status)

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


    def simu_program_list_changed(self, programs_dict):
        """ the program list has changed """
        for program in programs_dict.itervalues():
            self._log_view.add_program(program)

    def simu_program_new_log(self, program, name, level, message):
        """ a message sent a new log """
        self._log_view.add_log(program, name, level, message)
    

class LogView(WindowView):
    """ The log notebook handler """

    def __init__(self, parent, manager_log):
        WindowView.__init__(self, parent)

        self._log = manager_log
        self._event_tabs = {} # For the individual program event tabs
        self._run_conf_logs_dlg = ConfigLogsDialog()
        self._conf_debug_dlg = ConfigDebugDialog()
        self._event_notebook = self._ui.get_widget('event_notebook')
        mgr_event_tab = EventTab(self._event_notebook, "Manager events")
        self._log.run(mgr_event_tab)
        widget = self._ui.get_widget('enable_logs')
        self._logs_hdl = widget.connect('toggled', self.on_enable_logs_toggled)
        widget = self._ui.get_widget('enable_syslog')
        self._syslog_hdl = widget.connect('toggled', self.on_enable_syslog_toggled)
        widget = self._ui.get_widget('autoscroll')
        self._autoscroll_hdl = widget.connect('toggled', self.on_autoscroll_toggled)


    def update(self, hosts):
        """ update the host list """
        for tab in self._event_tabs:
            page_num = self._event_tabs[tab].page_num
            child = self._event_notebook.get_nth_page(page_num)
            gobject.idle_add(child.set_sensitive, True)
        for host_model in hosts:
            for _,host in host_model.get_machines().iteritems():
                if host.get_init_status() == InitStatus.FAIL:
                    for tab in self._event_tabs:
                        prog = self._event_tabs[tab].get_program()
                        if host == prog.get_host_model():
                            page_num = self._event_tabs[tab].page_num
                            child = self._event_notebook.get_nth_page(page_num)
                            gobject.idle_add(child.set_sensitive, False)
                    continue
                name = host.get_name().lower()
                if name not in self._event_tabs:
                    program = Program(None, "", name + ".", [], [], host)
                    self.add_program(program)


    def add_program(self, program):
        """ there is a new program to handle """
        if program.name not in self._event_tabs:
            self._event_tabs[program.name] = \
                EventTab(self._event_notebook, program.name, program)
        else:
            self._event_tabs[program.name].update(program)

    def add_log(self, program, name, level, message):
        """ called when an environment plane log is received """
        self._event_tabs[program.name].message(level, name, message)

    def global_event(self, message):
        """ put an event in all event tabs """
        for tab in self._event_tabs.values():
            tab.message(None, None, message, True)
    
    def on_event_notebook_switch_page(self, notebook, page, page_num):
        """ page switched on event notebook """
        program = self.get_program_for_active_tab(page_num)
        if program == None:
            self._ui.get_widget("logging_toolbar").hide()
            gobject.idle_add(self._run_conf_logs_dlg.hide)
            gobject.idle_add(self._conf_debug_dlg.hide)
            return
        widget = self._ui.get_widget("logging_toolbar")
        gobject.idle_add(widget.show)
        gobject.idle_add(self._run_conf_logs_dlg.update_list, program)
        gobject.idle_add(self._conf_debug_dlg.update, program)
        logs_enabled = program.logs_enabled()
        syslog_enabled = program.syslog_enabled()
        # if program has a debug configuration, enable log configuration
        widget = self._ui.get_widget('configure_logging')
        gobject.idle_add(widget.set_sensitive, True)
        if not program.is_running() and \
           not self._conf_debug_dlg.has_debug(program):
            gobject.idle_add(widget.set_sensitive, False)
        widget = self._ui.get_widget('enable_logs')
        # block toggle signal when modifying state from here
        widget.handler_block(self._logs_hdl)
        gobject.idle_add(widget.set_active, logs_enabled)
        widget.handler_unblock(self._logs_hdl)
        widget = self._ui.get_widget('enable_syslog')
        widget.handler_block(self._syslog_hdl)
        gobject.idle_add(widget.set_active, syslog_enabled)
        widget.handler_unblock(self._syslog_hdl)
        # set autoscroll state
        tab = self.get_active_tab(page_num)
        widget = self._ui.get_widget('autoscroll')
        val = tab.autoscroll
        widget.handler_block(self._autoscroll_hdl)
        gobject.idle_add(widget.set_active, val)
        widget.handler_unblock(self._autoscroll_hdl)

    def on_configure_logging_clicked(self, source=None, event=None):
        """ event handler for logging configuration """ 
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if program == None:
            gobject.idle_add(self._ui.get_widget("logging_toolbar").hide)
            gobject.idle_add(self._run_conf_logs_dlg.hide)
            return
        if program.is_running():
            gobject.idle_add(self._run_conf_logs_dlg.update_list, program)
            self._run_conf_logs_dlg.show()
        else:
            gobject.idle_add(self._conf_debug_dlg.update, program)
            self._conf_debug_dlg.show()

    def on_enable_syslog_toggled(self, source=None, event=None):
        """ event handler for syslog activation on remote program """ 
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if program is not None:
            wlog = self._ui.get_widget('enable_logs')
            wsyslog = self._ui.get_widget('enable_syslog')
            active = wlog.get_active() or wsyslog.get_active()
            program.enable_syslog(wsyslog.get_active())
            widget = self._ui.get_widget('configure_logging')
            gobject.idle_add(widget.set_sensitive, active)

    def on_enable_logs_toggled(self, source=None, event=None):
        """ event handler for logging  activation """ 
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if program is not None:
            wlog = self._ui.get_widget('enable_logs')
            wsyslog = self._ui.get_widget('enable_syslog')
            active = wlog.get_active() or wsyslog.get_active()
            program.enable_logs(wlog.get_active())
            widget = self._ui.get_widget('configure_logging')
            gobject.idle_add(widget.set_sensitive, active)

    def on_erase_logs_clicked(self, source=None, event=None):
        """ event handler for erase logs clicked """
        page = self._event_notebook.get_current_page()
        tab = self.get_active_tab(page)
        if tab is None:
            return
        tab.empty()

    def on_autoscroll_toggled(self, source=None, event=None):
        """ event handler for autoscroll toggled """
        page = self._event_notebook.get_current_page()
        tab = self.get_active_tab(page)
        if tab is None:
            return
        widget = self._ui.get_widget('autoscroll')
        val = widget.get_active()
        tab.autoscroll = val

    def get_active_tab(self, page_num):
        """ get the active event notebook page """
        child = self._event_notebook.get_nth_page(page_num)
        progname = self._event_notebook.get_tab_label(child).get_name()
        if not progname in self._event_tabs:
            return None
        return self._event_tabs[progname]

    def get_program_for_active_tab(self, page_num):
        """ get the program associated with the active event notebook page """
        tab = self.get_active_tab(page_num)
        if tab is None:
            return None
        program = tab.get_program()
        return program

    def on_start(self, run):
        """ the start button has been pressed
            (should be used with gobject.idle_add outside gtk handlers) """
        self.global_event("***** New run: %s *****" % run)
        widget = self._ui.get_widget('enable_logs')
        gobject.idle_add(widget.set_sensitive, True)
        widget = self._ui.get_widget('enable_syslog')
        gobject.idle_add(widget.set_sensitive, True)
        widget = self._ui.get_widget('configure_logging')
        gobject.idle_add(widget.set_sensitive, True)
        widget = self._ui.get_widget('erase_logs')
        gobject.idle_add(widget.set_sensitive, False)
        page = self._event_notebook.get_current_page()
        gobject.idle_add(self._run_conf_logs_dlg.hide)
        gobject.idle_add(self._conf_debug_dlg.hide)
        program = self.get_program_for_active_tab(page)
        if program == None:
            self._ui.get_widget("logging_toolbar").hide()
            return
        gobject.idle_add(self._run_conf_logs_dlg.update_list, program)

    def on_stop(self):
        """ the stop button has been pressed """
        widget = self._ui.get_widget('enable_logs')
        gobject.idle_add(widget.set_sensitive, False)
        widget = self._ui.get_widget('enable_syslog')
        gobject.idle_add(widget.set_sensitive, False)
        widget = self._ui.get_widget('configure_logging')
        gobject.idle_add(widget.set_sensitive, False)
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if self._conf_debug_dlg.has_debug(program):
            gobject.idle_add(widget.set_sensitive, True)
        widget = self._ui.get_widget('erase_logs')
        gobject.idle_add(widget.set_sensitive, True)
        gobject.idle_add(self._run_conf_logs_dlg.hide)
        gobject.idle_add(self._conf_debug_dlg.hide)
        for page_num in range(self._event_notebook.get_n_pages()):
            program = self.get_program_for_active_tab(page_num)
            if program is None:
                continue
            # set default values for next run
            # TODO maybe output should send syslog and logs state
            #      instead of using default values...
            program.enable_logs(True)
            program.enable_syslog(True)




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
probe_view.py - the probe tab view
"""

import gtk
import gobject
import threading
import os

from opensand_manager_gui.view.probe_display import ProbeDisplay
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ProbeException
from opensand_manager_gui.view.utils.config_elements import ProbeSelectionController

(TEXT, VISIBLE, ACTIVE) = range(3)

class ProbeView(WindowView):
    """ Elements of the probe tab """
    def __init__(self, parent, model, manager_log):
        """constructor, initialization """
        WindowView.__init__(self, parent)

        self._model = model
        self._log = manager_log

        # create the probes and the probe controller
        self._probe_lock = threading.Lock()

        # init the treeview used for the probe selection
        self._iter_id = {} # host name : iter string
        # Probe display
        self._vbox = self._ui.get_widget("probe_vbox")
        
        self._status_label = self._ui.get_widget("label_displayed")
        self._probe_button = self._ui.get_widget("probe_button")

        # {index_list:[graphic, graphic_type, subplot]}
        self._display = {}

        self._simu_running = False
        self._update_graph_tag = None
        self._scenario = None
        self._run = None

        self._probe_sel_controller = ProbeSelectionController(self,
            self._ui.get_widget("probe_sel_progs"),
            self._ui.get_widget("probe_sel_probes"))
        self._probe_display = ProbeDisplay(self._ui.get_widget("probe_vbox"))

    def toggled_cb(self, cell, path):
        """ this function is defined in probe_event """
        pass

    def plot_imported_graph(self, i, nbr, title, plot_type, x_list, y_list):
        """ plot a graph from data in file  """
        # Construct the current subplot
        self._fig.subplots_adjust(hspace = 0.8)

        sub = pylab.subplot(nbr, 1, i + 1)

        self._log.debug('plot file length: %s %s' %
                        (len(x_list) , len(y_list)))

        colors = self.random_color()
        if plot_type == 'dot':
            pylab.plot(x_list, y_list, 'o', 1, color=colors)
        elif plot_type == 'dotline':
            # dotline is replaced by line when importing graph because there is
            # too much values
            pylab.plot(x_list, y_list, '-', 1, color=colors)
        elif plot_type == 'step':
            pylab.plot(x_list, y_list, ls='steps', lw=1, color=colors)
        elif plot_type == 'stem':
            pylab.plot(x_list, y_list, 'o', 1, color=colors)
            pylab.vlines(x_list, 0, y_list, linestyle='solid',
                         color=colors, linewidth=1)
        elif plot_type == 'fill':
            pylab.fill(x_list, y_list, colors, 0.5)
        else: #line by default
            pylab.plot(x_list, y_list, '-', 1, color=colors)

        pylab.title(title)

        ymin = min(y_list)
        ymax = max(y_list)
        rymin = ymin - ((ymax - ymin) * 0.1)
        rymax = ymax + ((ymax - ymin) * 0.1)
        pylab.ylim(ymin=rymin, ymax=rymax)

        formatter = FormatStrFormatter('%2.8g')
        sub.yaxis.set_major_formatter(formatter)
        sub.xaxis.set_major_formatter(formatter)

        self._canvas.draw_idle()

        gobject.idle_add(self.enable_savefig_button,
                         priority=gobject.PRIORITY_HIGH_IDLE+20)

    def simu_program_list_changed(self, programs):
        """ the program list changed during simulation """
        self._probe_sel_controller.update_data(programs)
    
    def new_probe_value(self, probe, time, value):
        """ a new probe value was received """
        self._probe_display.add_probe_value(probe, time, value)
    
    def simu_state_changed(self):
        """ the simulation was (possibly) started or stopped """
        new_state = self._model.is_running()
    
        if self._simu_running == new_state:
            return
    
        self._simu_running = new_state
    
        if new_state:
            self._set_state_simulating()
            self._start_graph_update()
        else:
            self._stop_graph_update()
            self._set_state_idle()
            

    def displayed_probes_changed(self, displayed_probes):
        """ a probe was selected/unselected for display """
        
        self._probe_display.update(displayed_probes)

    def _set_state_idle(self):
        self._status_label.set_markup("<b>Displayed:</b> -")
        self._probe_button.set_label("Load…")
        self._probe_button.set_sensitive(False)
    
    def _set_state_simulating(self):
        self._status_label.set_markup("<b>Displayed:</b> Current simulation")
        self._probe_button.set_label("Configure collection…")
        self._probe_button.set_sensitive(True)
    
    def _set_state_run_loaded(self):
        self._status_label.set_markup("<b>Displayed:</b> Run %s" % self._run)
        self._probe_button.set_label("Load…")
        self._probe_button.set_sensitive(True)

    def _start_graph_update(self):
        """ enables the timer to refresh the graphs periodically """
        
        self._update_graph_tag = gobject.timeout_add(500,
            self._probe_display.graph_update,
            priority=gobject.PRIORITY_HIGH_IDLE)
    
    def _stop_graph_update(self):
        gobject.source_remove(self._update_graph_tag)
        self._update_graph_tag = None

    def save_figure(self, filename):
        """ save the displayed figure """
        # an error popup is raised on error by the function
        try:
            pylab.savefig(filename)
        except ValueError, error:
            error = str(error).partition('\n')
            error_popup(error[0], error[2])

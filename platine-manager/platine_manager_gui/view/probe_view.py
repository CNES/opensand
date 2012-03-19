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
probe_view.py - the probe tab view
"""

import gtk
import gtk.glade
import gobject
import threading
import os

from random import randrange
from matplotlib.backends.backend_gtkagg import FigureCanvasGTK
import pylab
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter

from platine_manager_gui.view.window_view import WindowView
from platine_manager_gui.view.probes.probe_model import ProbeModel
from platine_manager_gui.view.probes.probe_controller import ProbeController
from platine_manager_gui.view.popup.infos import error_popup
from platine_manager_core.my_exceptions import ProbeException
from platine_manager_gui.view.utils.config_elements import ConfigurationTree

pylab.hold(False)

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
        self._probes = ProbeModel(self._model, self._probe_lock, self._log)
        self._probe_controller = ProbeController(self._probes, manager_log)

        self._host_names_detected = []

        # init the treeview used for the probe selection
        self._iter_id = {} # host name : iter string
        # Probe display
        self._vbox = self._ui.get_widget("probe_vbox")

        # {index_list:[graphic, graphic_type, subplot]}
        self._display = {}

        self._files_path = ''

        with gtk.gdk.lock:
            treeview = self._ui.get_widget('probe_treeview')
            self._tree = ConfigurationTree(treeview, 'Statistics', 'Selected',
                                           None, self.toggled_cb)
            
        pylab.clf()
        self._fig = plt.figure()
        self._canvas = None

        gobject.idle_add(self.disable_savefig_button,
                         priority=gobject.PRIORITY_HIGH_IDLE+20)

        # refresh the probe tree immediatly then create an object
        # which refresh it
        self.refresh()
        self._refresh_probe_tree = gobject.timeout_add(1000, self.refresh)

    def toggled_cb(self, cell, path):
        """ this function is defined in probe_event """
        pass

    def init_canvas(self, nbr):
        """ initialize the graphic canvas """
        pylab.clf()
        formatter = FormatStrFormatter('%2.8g')

        if nbr != 0:
            for i in range(nbr):
                self._fig.add_subplot(nbr, 1, i + 1)
        else:
            self._fig.clear()
            return

        if self._canvas is not None:
            self._vbox.remove(self._canvas)
        self._canvas = FigureCanvasGTK(self._fig)
        self._vbox.pack_start(self._canvas, True, True)
        self._canvas.show_all()
        self._canvas.draw_idle()

    def init_graph(self, display_index, x, y, title = '',
                   xlabel = '', ylabel = '', idx = 0, tot = 0,
                   graph_type = 'line'):
        """ initialize a graphic """
        # Construct the current subplot
        self._fig.subplots_adjust(hspace = 0.8)

        sub = pylab.subplot(tot, 1, idx + 1)

        colors = self.random_color()
        pl = None
        vl = []
        poly = None
        if graph_type == 'dot':
            pl = pylab.plot(x, y, 'o', 2, colors)
        elif graph_type == 'dotline':
            pl = pylab.plot(x, y, 'o-', 2, colors)
        elif graph_type == 'step':
            pl = pylab.plot(x, y, ls='steps', lw=2, c=colors)
        elif graph_type == 'stem':
            vl.append(pylab.plot(x, y, 'o', 2, colors))
            vl.append(pylab.vlines(x, 0, y,  linestyle='solid', color='b'))
        elif graph_type == 'fill':
            poly = pylab.fill(x, y, 0, None, linewidth=2)
        else: #line by default
            pl = pylab.plot(x, y, '-', 2, colors)

        pylab.title(title, size="small")
        pylab.xlabel(xlabel, x=0.9, y=0.9, size="x-small")
        pylab.ylabel(ylabel, x=0.9, y=0.9, size="x-small")
        pylab.xticks(size="x-small")
        pylab.yticks(size="x-small")

        if pl is not None:
            self._display[display_index][0] = pl
            self._display[display_index][1] = 'plot'
            self._display[display_index][2] = sub
        elif len(vl) != 0:
            self._display[display_index][0] = vl
            self._display[display_index][1] = 'vline'
            self._display[display_index][2] = sub
        elif poly is not None:
            self._display[display_index][0] = poly
            self._display[display_index][1] = 'poly'
            self._display[display_index][2] = sub

    def update_plots(self, x, y, xmin, ymin, xmax, ymax, graph, sub):
        """ add new values into a plot graph """
        graph.set_data(x, y)

        rymin = ymin - ((ymax - ymin) * 0.1)
        rymax = ymax + ((ymax - ymin) * 0.1)

        sub.axis([xmin, xmax, rymin, rymax])

        formatter = FormatStrFormatter('%2.8g')
        sub.yaxis.set_major_formatter(formatter)
        sub.xaxis.set_major_formatter(formatter)

# TODO find an equivalent for set_data for vlines and fill to avoid calling the
#      function on each update
    def update_vlines(self, x, y, xmin, ymin, xmax, ymax, graph, sub):
        """ add new values into a stem graph """
        graph[0][0].set_data(x, y)
        sub.vlines(x, 0, y,  linestyle='solid', color='b')

        rymin = ymin - ((ymax - ymin) * 0.1)
        rymax = ymax + ((ymax - ymin) * 0.1)

        sub.axis([xmin, xmax, rymin, rymax])

        formatter = FormatStrFormatter('%2.8g')
        sub.yaxis.set_major_formatter(formatter)
        sub.xaxis.set_major_formatter(formatter)

    def update_polys(self, x, y, xmin, ymin, xmax, ymax, graph, sub):
        """ add new values into a polygons graph """
        sub.fill_between(x, y, 0, None, linewidth=2)

        rymin = ymin - ((ymax - ymin) * 0.1)
        rymax = ymax + ((ymax - ymin) * 0.1)

        sub.axis([xmin, xmax, rymin, rymax])

        formatter = FormatStrFormatter('%2.8g')
        sub.yaxis.set_major_formatter(formatter)
        sub.xaxis.set_major_formatter(formatter)

    def random_color(self):
        """ select a random color """
        list_colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']

        pos = randrange(len(list_colors))
        elem = list_colors[pos]

        return elem

    def update_all_graphs(self):
        """ update all graphs """
        if self._canvas is not None:
            self.draw_graph()

    def draw_graph(self):
        """ apply change """
        self._canvas.draw_idle()

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

        # TODO y_list -> y_list
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

    def update_probe_tree(self, new, old):
        """ update the probe tree view """
        # get the list of ST to append only relevant substatistics
        st_names = []
        for host in self._model.get_hosts_list():
            if host.get_name().startswith('st'):
                st_names.append(host.get_name())

        for probe in [elt for elt in self._probes.get_list()
                          if elt.get_name() in new]:
            stats = {}
            for stat in probe.get_stat_list():
                # substatistic
                if len(stat.get_index_list()) > 0 and \
                   stat.get_index_list()[0].get_name() != '':
                    stats[stat.get_name()] = []
                    # add only the relevant ST in the list
                    for index in stat.get_index_list():
                        name = index.get_name()
                        if name != '' and \
                           (name[:2].lower() != 'st' or 
                            name.lower() in st_names):
                            stats[stat.get_name()].append(name)
                # no substatistic
                else:
                    stats[stat.get_name()] = True
            gobject.idle_add(self._tree.add_host,
                             probe, stats)

        for probe in [elt for elt in self._probes.get_list()
                          if elt.get_name() in old]:
            gobject.idle_add(self._tree.del_host, probe.get_name())
            self._probes.remove_probe(probe)

    def refresh(self):
        """ refresh the probe list """
        if self._model.is_running():
            self._files_path = ''
            gobject.idle_add(self.enable_plot_button,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)
        # a scenario has been loaded
        elif self._files_path != '':
            gobject.idle_add(self.enable_plot_button,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)
        # load the last run
        elif self._model.get_run() != 'default':
            scenario = self._model.get_scenario()
            run = self._model.get_run()
            self._files_path = os.path.join(scenario, run)
            gobject.idle_add(self.enable_plot_button,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)
        # no previous run, nothing loaded
        else:
            self._files_path = ''
            gobject.idle_add(self.disable_plot_button,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)
        self._probe_lock.acquire()
        real_names = []
        for host in self._model.get_hosts_list():
            real_names.append(host.get_name())

        # add the new hosts
        new_host_names = set(real_names) - set(self._host_names_detected)
        if len(new_host_names) > 0:
            self._host_names_detected.extend(new_host_names)

            try:
                self._probes.create(new_host_names)
            except ProbeException as error:
                self._log.warning("failed to update probes for %s: %s!" %
                                  (new_host_names, error.value))
                self._probe_lock.release()
                return True
            else:
                self._log.debug("new probe successfully append")

        # remove old hosts
        old_host_names = set(self._host_names_detected) - set(real_names)
        for host in old_host_names:
            self._host_names_detected.remove(host)

        gobject.idle_add(self.update_probe_tree,
                         new_host_names, old_host_names)
        self._probe_lock.release()
        return True

    def disable_plot_button(self):
        """ disable plot probe button
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('plot').set_sensitive(False)

    def enable_plot_button(self):
        """ enable plot probe button
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('plot').set_sensitive(True)
        label = ''
        if self._model.is_running():
            label = self._model.get_run()
        elif self._files_path != '':
            label = self._files_path
        self._ui.get_widget('run_label').set_text(label)

    def disable_savefig_button(self):
        """ disable save probe button
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('savefig').set_sensitive(False)

    def enable_savefig_button(self):
        """ enable save probe button
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('savefig').set_sensitive(True)

    def save_figure(self, filename):
        """ save the displayed figure """
        # an error popup is raised on error by the function
        try:
            pylab.savefig(filename)
        except ValueError, error:
            error = str(error).partition('\n')
            error_popup(error[0], error[2])

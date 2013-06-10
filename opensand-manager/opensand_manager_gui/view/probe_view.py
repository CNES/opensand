#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 TAS
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

import gobject
import threading

from opensand_manager_gui.view.probe_display import ProbeDisplay
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_gui.view.popup.config_collection_dialog import ConfigCollectionDialog
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
        self._load_run_button = self._ui.get_widget("load_run_button")
        self._clean_sel_button = self._ui.get_widget("clear_probes")
        self._conf_coll_button = self._ui.get_widget("conf_collection_button")
        self._save_fig_button = self._ui.get_widget("save_figure_button")

        self._simu_running = False
        self._saved_data = None
        self._update_graph_tag = None

        self._probe_sel_controller = ProbeSelectionController(self,
            self._ui.get_widget("probe_sel_progs"),
            self._ui.get_widget("probe_sel_probes"))
        self._conf_coll_dialog = ConfigCollectionDialog(model, manager_log,
                                                        self._probe_sel_controller)
        self._probe_display = ProbeDisplay(self._ui.get_widget("probe_vbox"))

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
        self._saved_data = None
        self._probe_display.set_probe_data(None)

        if new_state:
            self._set_state_simulating()
            self._start_graph_update()
        else:
            if self._update_graph_tag is not None:
                self._stop_graph_update()

            self._set_state_idle()

    def simu_data_available(self):
        """ run when simulation data is available """
        self._saved_data = self._model.get_saved_probes()
        if self._saved_data:
            self._probe_display.set_probe_data(self._saved_data.get_data())
            self._set_state_run_loaded()
        else:
            self._set_state_idle(enable_loading=True)

    def displayed_probes_changed(self, displayed_probes):
        """ a probe was selected/unselected for display """
        self._probe_display.update(displayed_probes)
        if len(displayed_probes) > 0:
            self._clean_sel_button.set_sensitive(True)
            self._save_fig_button.set_sensitive(True)
        else:
            self._clean_sel_button.set_sensitive(False)
            self._save_fig_button.set_sensitive(False)

    def scenario_changed(self):
        """ the scenario was changed """
        if self._simu_running:
            return

        self.run_changed()

    def run_changed(self):
        """ the run was changed """
        if self._simu_running:
            return

        if self._model.get_run():
            self.simu_data_available()
        else:
            self._set_state_idle()

    def _set_state_idle(self, enable_loading=False):
        """ update status when idle """
        self._probe_sel_controller.update_data({})
        self._conf_coll_dialog.hide()
        self._status_label.set_markup("<b>Displayed:</b> -")
        self._load_run_button.set_sensitive(enable_loading)
        self._load_run_button.show()
        self._conf_coll_button.hide()
        self._clean_sel_button.hide()
        self._clean_sel_button.set_sensitive(False)
        self._save_fig_button.set_sensitive(False)

    def _set_state_simulating(self):
        """ update status when a simulation is running """
        self._status_label.set_markup("<b>Displayed:</b> Current simulation")
        self._load_run_button.hide()
        self._conf_coll_button.show()
        self._clean_sel_button.show()
        self._clean_sel_button.set_sensitive(False)
        self._save_fig_button.set_sensitive(False)

    def _set_state_run_loaded(self, run=None):
        """ update status when a scenario is loaded """
        self._conf_coll_dialog.hide()
        self._status_label.set_markup("<b>Displayed:</b> Run %s" %
                                      (run or self._model.get_run()))
        self._load_run_button.set_sensitive(True)
        self._load_run_button.show()
        self._conf_coll_button.hide()
        self._clean_sel_button.show()
        self._clean_sel_button.set_sensitive(False)
        self._save_fig_button.set_sensitive(True)

        self._probe_sel_controller.update_data(self._saved_data.get_programs())

    def _start_graph_update(self):
        """ enables the timer to refresh the graphs periodically """
        self._update_graph_tag = gobject.timeout_add(500,
            self._probe_display.graph_update,
            priority=gobject.PRIORITY_HIGH_IDLE)

    def _stop_graph_update(self):
        """ stop updating graph """
        gobject.source_remove(self._update_graph_tag)
        self._update_graph_tag = None

    def save_figure(self, filename):
        """ save the displayed figure """
        # an error popup is raised on error by the function
        try:
            self._probe_display.save_figure(filename)
        except ValueError, error:
            error = str(error).partition('\n')
            error_popup(error[0], error[2])

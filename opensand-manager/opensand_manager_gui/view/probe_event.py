#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
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
probe_event.py - the events on probe tab
"""

import gtk
import os

from opensand_manager_gui.view.probe_view import ProbeView
from opensand_manager_gui.view.popup.run_dialog import RunDialog

class ProbeEvent(ProbeView):
    """ Events for the probe tab """
    def __init__(self, parent, model, manager_log):
        ProbeView.__init__(self, parent, model, manager_log)

        self._updating = False

    def close(self):
        """ close probe tab """
        self._log.debug("Probe Event: close")

        if self._simu_running and self._update_graph_tag is not None:
            self._stop_graph_update()

        self._log.debug("Probe Event: closed")

    def activate(self, val):
        """ 'activate' signal handler """
        if not self._simu_running:
            return

        if val and self._update_graph_tag is None:
            self._start_graph_update(False)

        elif not val and self._update_graph_tag is not None:
            self._stop_graph_update()

    def on_load_run_button_clicked(self, _):
        """ The load run button was clicked """
        dlg = RunDialog(self._model.get_scenario(), self._model.get_run())
        run = dlg.go()

        self._saved_data = self._model.get_saved_probes(run)
        if not self._saved_data:
            return

        self._probe_display.set_probe_data(self._saved_data.get_data())
        self._set_state_run_loaded(run)

    def on_conf_collection_button_clicked(self, _):
        """ The Configure Collection button was clicked """
        self._conf_coll_dialog.show()

    def on_save_figure_button_clicked(self, _):
        """ The Save Figure button was clicked """
        dlg = gtk.FileChooserDialog("Save Figure", None,
                                    gtk.FILE_CHOOSER_ACTION_SAVE,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                    gtk.STOCK_APPLY, gtk.RESPONSE_APPLY))
        dlg.set_current_name("probe.svg")
        dlg.set_current_folder(os.path.join(self._model.get_scenario(),
                                            self._model.get_run()))
        dlg.set_do_overwrite_confirmation(True)
        imgfilter = gtk.FileFilter()
        imgfilter.add_pixbuf_formats()
        imgfilter.set_name('All images')
        dlg.add_filter(imgfilter)
        allfilter = gtk.FileFilter()
        allfilter.add_pattern("*")
        allfilter.set_name('All files')
        dlg.add_filter(allfilter)
        ret = dlg.run()
        filename = dlg.get_filename()
        if ret == gtk.RESPONSE_APPLY and filename is not None:
            (_, ext) = os.path.splitext(filename)
            if ext == '':
                # set default filetype to svg, jpg gives bad results
                filename = filename + '.svg'
            self._log.debug("save graphic to " + filename)
            self.save_figure(filename)

        dlg.destroy()

    def on_clear_probes_clicked(self, _):
        """ The clear probe selection button was clicked """
        self._probe_sel_controller.clear_selection()

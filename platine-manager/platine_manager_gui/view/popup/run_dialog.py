#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
run_dialog.py - Platine manager scenario dialog
"""

import os
import gtk
import gobject

from platine_manager_gui.view.window_view import WindowView

MAX_ST = 5

class RunDialog(WindowView):
    """ dialog to get the scenario to load """
    def __init__(self, scenario):
        WindowView.__init__(self, None, 'run_dialog')

        self._dlg = self._ui.get_widget('run_dialog')
        # add available runs into combo box
        self.populate(scenario)

    def go(self):
        """ run the window """
        # run the dialog and store the response
        result = self._dlg.run()
        if result == 0:
            return True
        else:
            return False

    def get_run(self):
        """ get the run value """
        widget = self._ui.get_widget("run_box")
        model = widget.get_model()
        active = widget.get_active()
        if active < 0:
            return None
        return model[active][0]

    def populate(self, scenario):
        """ add run elements into the combo box """
        # the list of directories to ignore
        ignore = ['sat', 'gw', 'tools']
        for i in range(MAX_ST + 1):
            ignore.append("st" + str(i))

        store = gtk.ListStore(gobject.TYPE_STRING)
        content = os.listdir(scenario)
        for path in content:
            if os.path.isdir(os.path.join(scenario, path)) and \
               path not in ignore:
                store.append([path])

        widget = self._ui.get_widget("run_box")
        widget.set_model(store)
        cell = gtk.CellRendererText()
        widget.pack_start(cell, True)
        widget.add_attribute(cell, 'text', 0)
        widget.set_active(0)

    def close(self):
        """ close the window """
        self._dlg.destroy()


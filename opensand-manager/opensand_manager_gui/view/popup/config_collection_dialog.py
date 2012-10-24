# -*- coding: utf-8 -*-

"""
config_collection_dialog.py - Dialog allowing to enable and disable probes
"""

import gobject
import gtk
import threading

from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_gui.view.utils.config_elements import ConfigurationTree, \
                                                           ConfigurationNotebook

(TEXT, VISIBLE, ACTIVE, ACTIVATABLE) = range(4)

class ConfigCollectionDialog(WindowView):
    """ an advanced configuration window """
    def __init__(self, model, manager_log, sel_controller):
        WindowView.__init__(self, None, 'config_collection_window')

        self._dlg = self._ui.get_widget('config_collection_window')
        self._listview = self._ui.get_widget('config_collection_view')
        self._sel_controller = sel_controller
        self._model = model
        self._log = manager_log
        self._shown = False
        self._programs = {}
        
        self._probe_store = gtk.ListStore(bool, str, int, int)
        self._listview.set_model(self._probe_store)
        self._listview.get_selection().set_mode(gtk.SELECTION_NONE)
        
        column_text = gtk.TreeViewColumn("Probe")
        self._listview.append_column(column_text)
        
        cellrenderer_toggle = gtk.CellRendererToggle()
        column_text.pack_start(cellrenderer_toggle, False)
        column_text.add_attribute(cellrenderer_toggle, "active", 0)
        cellrenderer_toggle.connect("toggled", self._cell_toggled)
        
        cellrenderer_text = gtk.CellRendererText()
        column_text.pack_start(cellrenderer_text, True)
        column_text.add_attribute(cellrenderer_text, "text", 1)
        
        self._dlg.connect('delete-event', self._delete)

    def show(self):
        """ show the window """
        
        if self._shown:
            self._dlg.present()
            return
        
        self._shown = True
        
        self._sel_controller.register_collection_dialog(self)
        
        self._dlg.set_icon_name('gtk-properties')
        self._dlg.show()
    
    def hide(self):
        """ hide the window """
        if not self._shown:
            return
        
        self._sel_controller.register_collection_dialog(None)
        self._shown = False
        self._dlg.hide()

    def update_list(self, program_dict):
        """ update the probe list shown by the window """
    
        self._programs = program_dict
        
        self._probe_store.clear()
        
        for program in self._programs.itervalues():
            for probe in program.get_probes():
                self._probe_store.append([probe.enabled, probe.full_name,
                    probe.program.ident, probe.ident])
    
    def _cell_toggled(self, _, path):
        """ a window cell has been toggled """
    
        it = self._probe_store.get_iter(path)
        new_value = not self._probe_store.get_value(it, 0)
        prog_id = self._probe_store.get_value(it, 2)
        probe_id = self._probe_store.get_value(it, 3)
        
        probe = self._programs[prog_id].get_probe(probe_id)
        was_hidden = False
        if probe.displayed:
            was_hidden = True
            probe.displayed = False
        
        probe.enabled = new_value
        self._probe_store.set(it, 0, new_value)
        
        self._sel_controller.probe_enabled_changed(probe, was_hidden)

    def _delete(self, _widget, _event):
        """ the window close button was clicked """
        
        self.hide()
        return True        

# -*- coding: utf-8 -*-

"""
progress_dialog.py - A generic progress dialog
"""

from opensand_manager_gui.view.window_view import WindowView

class ProgressDialog(WindowView):
    """ a window enabling to edit the deploy.ini file """
    def __init__(self, text, model, manager_log):
        WindowView.__init__(self, None, 'progress_dialog')

        self._dlg = self._ui.get_widget('progress_dialog')
        self._log = manager_log
        self._ui.get_widget('progress_label').set_text(text)
        self._dlg.set_transient_for(self._ui.get_widget('window'))

    def ping(self):
        """ indicates progress """
        self._ui.get_widget('progress_bar').pulse()

    def show(self):
        """ show the window """

        self._dlg.show()

    def close(self):
        """ close the window """
        self._dlg.destroy()
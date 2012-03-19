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
platine_view.py - Platine manager view
"""

import gtk
import gtk.glade
import time
import gobject
import os

from platine_manager_gui.view.window_view import WindowView
from platine_manager_gui.view.conf_event import ConfEvent
from platine_manager_gui.view.run_event import RunEvent
from platine_manager_gui.view.probe_event import ProbeEvent
from platine_manager_gui.view.tool_event import ToolEvent
from platine_manager_gui.view.popup.infos import error_popup, yes_no_popup
from platine_manager_core.my_exceptions import RunException, ProbeException, \
                                               ViewException, ModelException
from platine_manager_core.utils import copytree

GLADE_PATH = '/usr/share/platine/manager/platine.glade'


class View(WindowView):
    """ Platine manager view """
    def __init__(self, model, manager_log, glade='',
                 scenario='', dev_mode=False):
        if glade == '':
            glade = GLADE_PATH
        if not os.path.exists(glade):
            self._log.error("cannot find glade file at %s" % glade)
            raise ViewException

        WindowView.__init__(self, gladefile=glade)

        self._model = model
        self._log = manager_log
        self._log.run(self._ui, 'manager_textview')

        self._log.info("Welcome to Platine Manager !")
        self._log.info("Initializing platform, please wait...")

        # initialize each tab
        try:
            self._eventconf = ConfEvent(self.get_current(),
                                        self._model, self._log)
            self._eventrun = RunEvent(self.get_current(), self._model,
                                      dev_mode, self._log)
            self._eventtool = ToolEvent(self.get_current(),
                                        self._model, self._log)
            self._eventprobe = ProbeEvent(self.get_current(),
                                          self._model, self._log)
        except RunException:
            raise
        except ProbeException:
            self._log.warning("Probe tab was disabled")
            self._ui.get_widget("probe_tab").set_sensitive(False)
        except ViewException:
            raise

        self._current_page = self._ui.get_widget('notebook').get_current_page()
        self._pages = \
        {
            'run'   : 0,
            'conf'  : 1,
            'tools' : 2,
            'probes': 3,
        }

        # update the window title
        if scenario == "":
            gobject.idle_add(self.set_title, "Platine Manager - [new]")
        else:
            gobject.idle_add(self.set_title,
                             "Platine Manager - [%s]" % scenario)

        gobject.idle_add(self.set_recents)

    def exit(self):
        """ quit main window and application """
        self.exit_kb()
        gtk.main_quit()

    def exit_kb(self):
        """ quit main window and application when keyboard interrupted """
        if self._model.is_default_modif():
            text = "Default path will be overwritten next time\n\n" \
                   "Do you want to save your scenario ?"
            ret = yes_no_popup(text, "Save scenario ?",
                               "gtk-dialog-warning")
            if ret == gtk.RESPONSE_YES:
                try:
                    self.save_as()
                except Exception, err:
                    error_popup("Errors saving scenario:", str(err))

        if self._model.is_running():
            text = "The platform is still running, " \
                   "other users won't be able to use it\n\n" \
                   "Stop it before exiting ?"
            ret = yes_no_popup(text, "Stop ?", "gtk-dialog-warning")
            if ret == gtk.RESPONSE_YES:
                self._eventrun.on_start_platine_button_clicked()
                iter = 0
                while self._model.is_running() and iter < 20:
                    self._log.info("Waiting for platform to stop...")
                    iter += 1
                    time.sleep(1)

        self._log.debug("View: close")
        self._log.debug("View: close model")
        self._model.close()
        self._log.debug("View: close configuration view")
        self._eventconf.close()
        self._log.debug("View: close run view")
        self._eventrun.close()
        self._log.debug("View: close tool view")
        self._eventtool.close()
        self._log.debug("View: close probe view")
        self._eventprobe.close()
        self._log.debug("View: closed")

    def on_quit(self, source=None, event=None):
        """ event handler for close button """
        self._log.info("Close application")
        self._log.info("Please wait...")
        self.exit()

    def on_about_activate(self, source=None, event=None):
        """ event handler for about button """
        self.show_about()

    def on_notebook_switch_page(self, notebook, page, page_num):
        """ notebook page changed """
        # on configuration tab keep page if parameters have not been saved
        if self._current_page == self._pages['conf'] and \
           page_num != self._pages['conf'] and \
           not self._model.is_running():
            try:
                if self._eventconf.is_modified():
                    text =  "The configuration was not saved\n\n" \
                            "Do you want to save it ?"
                    ret = yes_no_popup(text,
                                       "Save Configuration - Platine Manager",
                                       "gtk-dialog-info")
                    if ret == gtk.RESPONSE_YES:
                        self._eventconf.on_save_conf_clicked()
                    else:
                        self._eventconf.update_view()
            except Exception, msg:
                self._log.warning("error when trying to check if configuration "
                                  "was modified: " + str(msg))

        # on tools tab keep page if some tools were modified
        if self._current_page == self._pages['tools'] and \
           page_num != self._pages['tools'] and \
           not self._model.is_running():
            # if the save button is sensitive,
            # the configuration may have changed
            if self._ui.get_widget('save_tool_conf').is_sensitive():
                text =  "The tools configuration was not saved\n\n" \
                        "Do you want to save it ?"
                ret = yes_no_popup(text, "Save Tools - Platine Manager",
                                   "gtk-dialog-info")
                if ret == gtk.RESPONSE_YES:
                    self._eventtool.on_save_tool_conf_clicked()
                else:
                    self._eventtool.on_undo_tool_conf_clicked()

        self._current_page = page_num

        if page_num == self._pages['run']:
            if self._eventrun != None:
                self._eventrun.activate(True)
            if self._eventconf != None:
                self._eventconf.activate(False)
            if self._eventtool != None:
                self._eventtool.activate(False)
            if self._eventprobe != None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['conf']:
            if self._eventrun != None:
                self._eventrun.activate(False)
            if self._eventconf != None:
                self._eventconf.activate(True)
            if self._eventtool != None:
                self._eventtool.activate(False)
            if self._eventprobe != None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['tools']:
            if self._eventconf != None:
                self._eventconf.activate(False)
            if self._eventrun != None:
                self._eventrun.activate(False)
            if self._eventtool != None:
                self._eventtool.activate(True)
            if self._eventprobe != None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['probes']:
            if self._eventconf != None:
                self._eventconf.activate(False)
            if self._eventrun != None:
                self._eventrun.activate(False)
            if self._eventtool != None:
                self._eventtool.activate(False)
            if self._eventprobe != None:
                self._eventprobe.activate(True)

    def switchpage(self, pagenum):
        """ modify the current page """
        self._ui.get_widget('notebook').set_current_page(pagenum)

    def show_about(self):
        """ show about dialog """
        xml = gtk.glade.XML(GLADE_PATH, 'about_dialog')
        about = xml.get_widget('about_dialog')
        result = about.run()
        if result == gtk.RESPONSE_CANCEL:
            about.hide()

    def run(self):
        """ start gobject loops """
        gtk.gdk.threads_enter()
        # start all gobject loops: GUI and Service
        # Thus PlatineServiceListener MUST be initialized before that call
        gtk.main()
        gtk.gdk.threads_leave()

    def on_new_activate(self, source=None, event=None):
        """ event handler for new button """
        folder = self.create_new_scenario()
        if folder is None:
            self._log.info("Load default scenario")
            folder = ""
        try:
            self._model.set_scenario(folder)
            if folder != "":
                self._log.info("New scenario created in %s" % folder)
        except ModelException as msg:
            error_popup(msg)
        # update the window title
        if folder is None or folder == "":
            self.set_title("Platine Manager - [new]")
        else:
            self.set_title("Platine Manager - [%s]" % folder)
        # reload the configuration
        self._eventconf.update_view()
        self.set_recents()

    def on_open_activate(self, source=None, folder=None):
        """ event handler for open button """
        if folder is None:
            dlg = gtk.FileChooserDialog("Open a scenario folder", None,
                                        gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                        (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                        gtk.STOCK_APPLY, gtk.RESPONSE_APPLY))
            dlg.set_current_folder(os.path.dirname(self._model.get_scenario()))
            ret = dlg.run()
            folder = dlg.get_filename()
            dlg.destroy()
        else:
            ret = gtk.RESPONSE_APPLY

        if ret == gtk.RESPONSE_APPLY and folder is not None:
            try:
                if not 'manager.ini' in os.listdir(folder):
                    error_popup("The directory does not contain a scenario")
                    return
            except OSError, msg:
                error_popup(str(msg))
                return

            self._log.info("Open scenario from %s" % folder)
            try:
                self._model.set_scenario(folder)
            except ModelException, msg:
                error_popup(str(msg))
        else:
            folder = None

        # update the window title
        if folder is None or folder == "":
            self.set_title("Platine Manager - [new]")
        else:
            self.set_title("Platine Manager - [%s]" % folder)
        # reload the configuration
        self._eventconf.update_view()
        self.set_recents()

    def on_save_as_activate(self, source=None, event=None):
        """ event handler for save_as button """
        try:
            folder = self.save_as()
        except Exception, err:
            error_popup("Errors saving scenario:", str(err))
            return

        if folder is not None:
            self._model.set_scenario(folder)

        # update the window title
        if folder is None or folder == "":
            self.set_title("Platine Manager - [new]")
        else:
            self.set_title("Platine Manager - [%s]" % folder)
        # reload the configuration
        self._eventconf.update_view()
        self.set_recents()

    def save_as(self):
        """ save the current scenario in a new folder """
        folder = self.create_new_scenario()
        if folder is not None:
            self._log.info("Save scenario in %s" % folder)
            try:
                copytree(self._model.get_scenario(), folder)
            except Exception:
                raise
        return folder

    def on_close_activate(self, source=None, event=None):
        """ event handler for close button """
        # update the window title
        self.set_title("Platine Manager - [new]")
        try:
            self._model.set_scenario("")
        except ModelException as msg:
            error_popup(msg)
        # reload the configuration
        self._eventconf.update_view()

    def create_new_scenario(self):
        """ create a new Platine scenario """
        # create a folder for the new scenario
        #FIXME when an empty path is given it freezes...
        dlg = gtk.FileChooserDialog("Create the new scenario folder", None,
                                    gtk.FILE_CHOOSER_ACTION_CREATE_FOLDER,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                    gtk.STOCK_APPLY, gtk.RESPONSE_APPLY))
        dlg.set_current_folder(os.path.dirname(self._model.get_scenario()))
        ret = dlg.run()
        folder = dlg.get_filename()
        if folder is not None and os.path.basename(folder).isspace():
            folder = None
        if ret == gtk.RESPONSE_APPLY and folder is not None:
            if os.path.exists(os.path.join(folder, "manager.ini")):
                over = yes_no_popup("Scenario '%s' already exists\n"
                                    "Do you want to overwrite it ?" %
                                    folder, "Overwrite scenario",
                                    "gtk-dialog-warning")
                if over != gtk.RESPONSE_YES:
                    folder = None
        else:
            folder = None
        dlg.destroy()

        return folder

    def set_title(self, title):
        """ set the main window title
            (should be used with gobject.idle_add outside gtk handlers) """
        self._ui.get_widget('window').set_title(title)

    def set_recents(self):
        """ set the list of recent files
            (should be used with gobject.idle_add outside gtk handlers) """
        if not 'HOME' in os.environ:
            return

        filename = os.path.join(os.environ['HOME'], ".platine/recent")
        if  not os.path.exists(filename):
            return

        with open(filename, 'r') as recent_file:
            recents = recent_file.readlines()

        recent_widget = self._ui.get_widget('open_recent')
        recent_widget.remove_submenu()
        submenu = gtk.Menu()
        for recent in recents:
            recent = recent.strip()
            menu_item = gtk.MenuItem(os.path.basename(recent))
            menu_item.set_tooltip_text(recent)
            menu_item.set_use_underline(False)
            menu_item.connect('activate', self.on_open_activate, recent)
            menu_item.set_visible(True)
            submenu.append(menu_item)

        recent_widget.set_submenu(submenu)

##### TEST #####
if __name__ == "__main__":
    from platine_manager_core.loggers.manager_log import ManagerLog
    from platine_manager_core.platine_model import Model

    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)

    VIEW = View(MODEL, LOGGER)
    VIEW.run()

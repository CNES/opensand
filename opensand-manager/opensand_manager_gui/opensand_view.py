#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
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
opensand_view.py - OpenSAND manager view
"""

import gtk
import gobject
import os
import shutil

from opensand_manager_gui.view.event_tab import EventTab
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.conf_event import ConfEvent
from opensand_manager_gui.view.run_event import RunEvent
from opensand_manager_gui.view.probe_event import ProbeEvent
from opensand_manager_gui.view.tool_event import ToolEvent
from opensand_manager_gui.view.popup.infos import error_popup, yes_no_popup
from opensand_manager_gui.view.utils.mines import SizeDialog, MineWindow
from opensand_manager_gui.view.popup.progress_dialog import ProgressDialog
from opensand_manager_gui.view.popup.config_logs_dialog import ConfigLogsDialog
from opensand_manager_core.my_exceptions import ConfException, ProbeException, \
                                               ViewException, ModelException
from opensand_manager_core.utils import copytree

GLADE_PATH = '/usr/share/opensand/manager/opensand.glade'
KONAMI = ['Up', 'Up', 'Down', 'Down',
          'Left', 'Right', 'Left', 'Right',
          'b', 'a']

class View(WindowView):
    """ OpenSAND manager view """
    def __init__(self, model, manager_log, glade='',
                 scenario='', dev_mode=False, service_type=''):
        self._log = manager_log
        if glade == '':
            glade = GLADE_PATH
        if not os.path.exists(glade):
            self._log.error("cannot find glade file at %s" % glade)
            raise ViewException("cannot find glade file at %s" % glade)

        WindowView.__init__(self, gladefile=glade)

        self._model = model
        
        self._event_tabs = {} # For the individudual program event tabs
        self._conf_logs_dialog = ConfigLogsDialog(self._model, self._log)
        self._event_notebook = self._ui.get_widget('event_notebook')
        mgr_event_tab = EventTab(self._event_notebook, "Manager events")
        self._log.run(mgr_event_tab)

        self._info_label = self._ui.get_widget('info_label')
        self._counter = 0
        infos = ['Service type: ' + service_type]
        gobject.timeout_add(5000, self.update_label, infos)
        
        self._prog_dialog = None

        self._log.info("Welcome to OpenSAND Manager !")
        self._log.info("Initializing platform, please wait...")

        widget = self._ui.get_widget('enable_logs')
        self._logs_hdl = widget.connect('toggled', self.on_enable_logs_toggled)
        widget = self._ui.get_widget('enable_syslog')
        self._syslog_hdl = widget.connect('toggled', self.on_enable_syslog_toggled)


        # initialize each tab
        try:
            self._eventconf = ConfEvent(self.get_current(),
                                        self._model, self._log)
            self._eventrun = RunEvent(self.get_current(), self._model,
                                      self, dev_mode, self._log)
            self._eventtool = ToolEvent(self.get_current(),
                                        self._model, self._log)
            self._eventprobe = ProbeEvent(self.get_current(),
                                          self._model, self._log)

            self._eventconf.activate(False)
            self._eventtool.activate(False)
            self._eventprobe.activate(False)
        except ProbeException:
            self._log.warning("Probe tab was disabled")
            self._ui.get_widget("probe_tab").set_sensitive(False)

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
            gobject.idle_add(self.set_title, "OpenSAND Manager - [new]")
        else:
            gobject.idle_add(self.set_title,
                             "OpenSAND Manager - [%s]" % scenario)

        self._keylist = []

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

        # TODO do we keep this part
        #if self._model.is_running():
        #    text = "The platform is still running, " \
        #           "other users won't be able to use it\n\n" \
        #           "Stop it before exiting ?"
        #    ret = yes_no_popup(text, "Stop ?", "gtk-dialog-warning")
        #    if ret == gtk.RESPONSE_YES:
        #        self._eventrun.on_start_opensand_button_clicked()
        #        iter = 0
        #        while self._model.is_running() and iter < 20:
        #            self._log.info("Waiting for platform to stop...")
        #            iter += 1
        #            time.sleep(1)

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
                                       "Save Configuration - OpenSAND Manager",
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
                ret = yes_no_popup(text, "Save Tools - OpenSAND Manager",
                                   "gtk-dialog-info")
                if ret == gtk.RESPONSE_YES:
                    self._eventtool.on_save_tool_conf_clicked()
                else:
                    self._eventtool.on_undo_tool_conf_clicked()

        self._current_page = page_num

        if page_num == self._pages['run']:
            if self._eventrun is not None:
                self._eventrun.activate(True)
            if self._eventconf is not None:
                self._eventconf.activate(False)
            if self._eventtool is not None:
                self._eventtool.activate(False)
            if self._eventprobe is not None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['conf']:
            if self._eventrun is not None:
                self._eventrun.activate(False)
            if self._eventconf is not None:
                self._eventconf.activate(True)
            if self._eventtool is not None:
                self._eventtool.activate(False)
            if self._eventprobe is not None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['tools']:
            if self._eventconf is not None:
                self._eventconf.activate(False)
            if self._eventrun is not None:
                self._eventrun.activate(False)
            if self._eventtool is not None:
                self._eventtool.activate(True)
            if self._eventprobe is not None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['probes']:
            if self._eventconf is not None:
                self._eventconf.activate(False)
            if self._eventrun is not None:
                self._eventrun.activate(False)
            if self._eventtool is not None:
                self._eventtool.activate(False)
            if self._eventprobe is not None:
                self._eventprobe.activate(True)

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
        # Thus OpenSandServiceListener MUST be initialized before that call
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
            self.set_default_run()
        except ModelException as msg:
            error_popup(msg)
        # update the window title
        if folder is None or folder == "":
            self.set_title("OpenSAND Manager - [new]")
        else:
            self.set_title("OpenSAND Manager - [%s]" % folder)
        # reload the configuration
        try:
            self._eventconf.update_view()
        except ConfException as msg:
            error_popup(str(msg))
        finally:
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
                if not 'core_global.conf' in os.listdir(folder):
                    error_popup("The directory does not contain a scenario")
                    return
            except OSError, msg:
                error_popup(str(msg))
                return

            self._log.info("Open scenario from %s" % folder)
            try:
                self._model.set_scenario(folder)
                self.set_default_run()
            except ModelException, msg:
                error_popup(str(msg))
        else:
            folder = None

        # update the window title
        if folder is None or folder == "":
            self.set_title("OpenSAND Manager - [new]")
        else:
            self.set_title("OpenSAND Manager - [%s]" % folder)
        # reload the configuration
        self._eventprobe.scenario_changed()
        try:
            self._eventconf.update_view()
        except ConfException as msg:
            error_popup(str(msg))
        finally:
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
            self.set_title("OpenSAND Manager - [new]")
        else:
            self.set_title("OpenSAND Manager - [%s]" % folder)
        # reload the configuration
        try:
            self._eventconf.update_view()
        except ConfException as msg:
            error_popup(str(msg))
        finally:
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
        """ event handler for close scenario button """
        # update the window title
        self.set_title("OpenSAND Manager - [new]")
        try:
            self._model.set_scenario("")
            self.set_default_run()
        except ModelException as msg:
            error_popup(msg)
        # reload the configuration
        try:
            self._eventconf.update_view()
        except ConfException as msg:
            error_popup(str(msg))

    def create_new_scenario(self):
        """ create a new OpenSAND scenario """
        # create a folder for the new scenario
        # do not use gtk.FILE_CHOOSER_ACTION_CREATE_FOLDER as it freeze when an
        # empty path is given
        dlg = gtk.FileChooserDialog("Create the new scenario folder", None,
                                    gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                    gtk.STOCK_APPLY, gtk.RESPONSE_APPLY))
        dlg.set_current_folder(os.path.dirname(self._model.get_scenario()))
        ret = dlg.run()
        folder = dlg.get_filename()
        if folder is not None and os.path.basename(folder).isspace():
            folder = None
        if ret == gtk.RESPONSE_APPLY and folder is not None:
            if os.path.exists(os.path.join(folder, "core_global.conf")):
                over = yes_no_popup("Scenario '%s' already exists\n"
                                    "Do you want to overwrite it ?" %
                                    folder, "Overwrite scenario",
                                    "gtk-dialog-warning")
                if over != gtk.RESPONSE_YES:
                    folder = None
                else:
                    try:
                        shutil.rmtree(folder)
                        os.makedirs(folder)
                    except (OSError, IOError), (_, strerror):
                        self._log.error("Failed to overwrite '%s': %s" %
                                        (folder, strerror))
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

        filename = os.path.join(os.environ['HOME'], ".opensand/recent")
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

    def set_default_run(self):
        """ reset the run value """
        self._model.set_run("")
        self._eventprobe.run_changed()

    def on_window_key_press_event(self, source=None, event=None):
        """ callback called on keyboard press """
        current = gtk.gdk.keyval_name(event.keyval)
        self._keylist.append(current)
        if len(self._keylist) > len(KONAMI):
            self._keylist.pop(0)
        if self._keylist == KONAMI:
            dlg = SizeDialog()
            dlg.run()
            h, v, n = dlg.size_options
            dlg.destroy()
            MineWindow(h, v, n)
        return False
    
    def on_program_list_changed(self, programs_dict):
        """ called when the environment plane program list changes """
        for program in programs_dict.itervalues():
            if program.name not in self._event_tabs:
                self._event_tabs[program.name] = \
                    EventTab(self._event_notebook, program.name, program)
            else:
                self._event_tabs[program.name].update(program)
        
        self._eventprobe.simu_program_list_changed(programs_dict)
    
    def on_new_program_log(self, program, name, level, message):
        """ called when an environment plane log is received """
        self._event_tabs[program.name].message(level, name, message)

    def global_event(self, message):
        """ put an event in all event tabs """
        for tab in self._event_tabs.values():
            tab.message(None, None, message, True)
    
    def on_new_probe_value(self, probe, timestamp, value):
        """ called when a new probe value is received """
        self._eventprobe.new_probe_value(probe, timestamp, value)
    
    def on_simu_state_changed(self):
        """ Called when the simulation state changes """
        self._eventprobe.simu_state_changed()
    
    def on_probe_transfer_progress(self, started):
        """ Called when probe transfer from the collector starts or stops """
        if started:
            self._prog_dialog = ProgressDialog("Saving probe data, please "
                                               "wait…", self._model, self._log)
            self._prog_dialog.show()
        else:
            self._prog_dialog.close()
            self._prog_dialog = None
            self._eventprobe.simu_data_available()

    def update_label(self, infos=[]):
        """ Update the message displayed on Manager """
        self._info_label.set_label(infos[self._counter])

        msg = 'Developer mode enabled'
        if self._model.get_dev_mode() and not msg in infos:
            infos.append(msg)
        elif msg in infos:
            infos.remove(msg)
        if len(infos) == 0:
            return True
        self._counter = (self._counter + 1) % len(infos)
        return True

    def on_event_notebook_switch_page(self, notebook, page, page_num):
        """ page switched on event notebook """
        program = self.get_program_for_active_tab(page_num)
        if program == None:
            self._ui.get_widget("logging_toolbar").hide()
            gobject.idle_add(self._conf_logs_dialog.hide)
            return
        self._ui.get_widget("logging_toolbar").show()
        gobject.idle_add(self._conf_logs_dialog.update_list, program)
        logs_enabled = program.logs_enabled()
        syslog_enabled = program.syslog_enabled()
        widget = self._ui.get_widget('enable_logs')
        # block toggle signal when modifying state from here
        widget.handler_block(self._logs_hdl)
        widget.set_active(logs_enabled)
        widget.handler_unblock(self._logs_hdl)
        widget = self._ui.get_widget('enable_syslog')
        widget.handler_block(self._syslog_hdl)
        widget.set_active(syslog_enabled)
        widget.handler_unblock(self._syslog_hdl)
        pass

    def on_configure_logging_clicked(self, source=None, event=None):
        """ event handler for logging configuration """ 
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if program == None:
            self._ui.get_widget("logging_toolbar").hide()
            gobject.idle_add(self._conf_logs_dialog.hide)
            return
        gobject.idle_add(self._conf_logs_dialog.update_list, program)
        self._conf_logs_dialog.show()

    def on_enable_syslog_toggled(self, source=None, event=None):
        """ event handler for syslog activation on remote program """ 
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if program is not None:
            widget = self._ui.get_widget('enable_syslog')
            program.enable_syslog(widget.get_active())

    def on_enable_logs_toggled(self, source=None, event=None):
        """ event handler for logging  activation """ 
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if program is not None:
            widget = self._ui.get_widget('enable_logs')
            active = widget.get_active()
            program.enable_logs(active)
            self._ui.get_widget('configure_logging').set_sensitive(active)

    def get_program_for_active_tab(self, page_num):
        """ get the label of the active event notebook page """
        child = self._event_notebook.get_nth_page(page_num)
        progname = self._event_notebook.get_tab_label(child).get_name()
        if not progname in self._event_tabs:
            return None
        program = self._event_tabs[progname].get_program()
        return program

    def on_start(self, run):
        """ the start button has been pressed
            (should be used with gobject.idle_add outside gtk handlers) """
        self.global_event("***** New run: %s *****" % run)
        widget = self._ui.get_widget('enable_logs')
        widget.set_sensitive(True)
        widget = self._ui.get_widget('enable_syslog')
        widget.set_sensitive(True)
        widget = self._ui.get_widget('configure_logging')
        widget.set_sensitive(True)
        page = self._event_notebook.get_current_page()
        program = self.get_program_for_active_tab(page)
        if program == None:
            self._ui.get_widget("logging_toolbar").hide()
            gobject.idle_add(self._conf_logs_dialog.hide)
            return
        gobject.idle_add(self._conf_logs_dialog.update_list, program)

    def on_stop(self):
        """ the stop button has been pressed """
        widget = self._ui.get_widget('enable_logs')
        widget.set_sensitive(False)
        widget = self._ui.get_widget('enable_syslog')
        widget.set_sensitive(False)
        widget = self._ui.get_widget('configure_logging')
        widget.set_sensitive(False)
        gobject.idle_add(self._conf_logs_dialog.hide)
        for page_num in range(self._event_notebook.get_n_pages()):
            program = self.get_program_for_active_tab(page_num)
            if program is None:
                continue
            # set default values for next run
            # TODO maybe output should send syslog and logs state
            #      instead of using default values...
            program.enable_logs(True)
            program.enable_syslog(True)



##### TEST #####
if __name__ == "__main__":
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model

    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)

    VIEW = View(MODEL, LOGGER)
    VIEW.run()

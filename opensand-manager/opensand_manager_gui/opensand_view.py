#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 TAS
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

# Author: Bénédicte Motto / <bmotto@toulouse.viveris.com>

"""
opensand_view.py - OpenSAND manager view
"""

import gtk
import gobject
import os
import shutil

from opensand_manager_core.controller.tcp_server import CommandServer
from opensand_manager_core.my_exceptions import ConfException, ProbeException, \
                                                ViewException, ModelException

from opensand_manager_core.utils import OPENSAND_PATH, copytree
from opensand_manager_gui.view.window_view import WindowView
from opensand_manager_gui.view.conf_event import ConfEvent
from opensand_manager_gui.view.resource_event import ResourceEvent
from opensand_manager_gui.view.run_event import RunEvent
from opensand_manager_gui.view.probe_event import ProbeEvent
from opensand_manager_gui.view.tool_event import ToolEvent
from opensand_manager_gui.view.event_handler import EventResponseHandler
from opensand_manager_gui.view.popup.infos import error_popup, \
                                                  info_popup, \
                                                  yes_no_popup
from opensand_manager_gui.view.popup.about_dialog import AboutDialog
from opensand_manager_gui.view.utils.mines import SizeDialog, MineWindow

GLADE_PATH = OPENSAND_PATH + "manager/opensand.glade"
KONAMI = ['Up', 'Up', 'Down', 'Down',
          'Left', 'Right', 'Left', 'Right',
          'b', 'a']
INIT_ITER = 6

class View(WindowView):
    """
    OpenSAND manager view:
        - handle menu actions
        - handle window title
        - handle notebook containing tab views
        - handle some events and dispatch them
    """
    def __init__(self, model, manager_log, glade='',
                 dev_mode=False, adv_mode=False,
                 service_type=''):
        self._log = manager_log
        if glade == '':
            glade = GLADE_PATH
        if not os.path.exists(glade):
            self._log.error("cannot find glade file at %s" % glade)
            raise ViewException("cannot find glade file at %s" % glade)

        WindowView.__init__(self, gladefile=glade)

        self._model = model
        
        # initialize each tab
        try:
            # run first because its starts the logging notebook
            self._eventrun = RunEvent(self.get_current(), self._model,
                                      dev_mode, adv_mode, self._log,
                                      service_type)
            self._eventconf = ConfEvent(self.get_current(),
                                        self._model, self._log)
            self._eventresource = ResourceEvent(self.get_current(),
                                                self._model, self._log)
            self._eventtool = ToolEvent(self.get_current(),
                                        self._model, self._log)
            self._eventprobe = ProbeEvent(self.get_current(),
                                          self._model, self._log)

            self._eventconf.activate(False)
            self._eventresource.activate(False)
            self._eventtool.activate(False)
            self._eventprobe.activate(False)
        except ProbeException:
            self._log.warning("Probe tab was disabled")
            self._ui.get_widget("probe_tab").set_sensitive(False)

        status_box = self._ui.get_widget('status_box')
#        status_box.modify_bg(gtk.STATE_NORMAL, gtk.gdk.Color(0xffff, 0xffff,
#                                                             0xffff))
        self._info_label = self._ui.get_widget('info_label')
        self._counter = 0
        infos = ['Service type: ' + service_type]
        gobject.timeout_add(5000, self.update_label, infos)


        self._current_page = self._ui.get_widget('notebook').get_current_page()
        self._pages = \
        {
            'run'   : 0,
            'conf'  : 1,
            'resources'  : 2,
            'tools' : 3,
            'probes': 4,
        }

        # update the window title
        if self._model._is_default:
            gobject.idle_add(self.set_title, "OpenSAND Manager - [new]")
        else:
            gobject.idle_add(self.set_title,
                             "OpenSAND Manager - [%s]" %
                             self._model.get_scenario())

        self._keylist = []

        gobject.idle_add(self.set_recents)

# TODO for new scenario, for start, etc
#        self._event_manager = self._model.get_event_manager()
        self._event_response_handler = EventResponseHandler(
                            self._model.get_event_manager_response(),
                            self._eventrun,
                            self._eventconf,
                            self._eventresource,
                            self._eventtool,
                            self._eventprobe,
                            self._log)

        # start event response handler
        self._event_response_handler.start()

        self._first_refresh = True
        self._refresh_iter = 0
        # at beginning check platform state immediatly, then every 2 seconds
        self.on_timer_status()
        self._timeout_id = gobject.timeout_add(1000, self.on_timer_status)

        self._log.info("Welcome to OpenSAND Manager !")
        self._log.info("Initializing platform, please wait...")
        CommandServer._shutdown = self.on_quit
        
        
    def exit(self):
        """ quit main window and application """
        self.exit_kb()
        gtk.main_quit()

    def exit_kb(self):
        """ quit main window and application when keyboard interrupted """
        if self._model.is_default_modif() and not self._model.is_running():
            text = "Default path will be overwritten next time\n\n" \
                   "Do you want to save your scenario ?"
            ret = yes_no_popup(text, "Create scenario ?",
                               gtk.STOCK_DIALOG_WARNING)
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
        #    ret = yes_no_popup(text, "Stop ?", gtk.STOCK_DIALOG_WARNING)
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
        self._log.debug("View: close resources configuration view")
        self._eventresource.close()
        self._log.debug("View: close run view")
        self._eventrun.close()
        self._log.debug("View: close tool view")
        self._eventtool.close()
        self._log.debug("View: close probe view")
        self._eventprobe.close()
        self._log.debug("View: closed")
        if self._event_response_handler.is_alive():
            self._log.debug("Run Event: join response event handler")
            self._event_response_handler.join()
            self._log.debug("Run Event: response event handler joined")


    def on_quit(self, source=None, event=None):
        """ event handler for close button """
        self._log.info("Close application")
        self._log.info("Please wait...")
        self.exit()

            
    def update_label(self, infos=[]):
        """ Update the message displayed on Manager """
        self._info_label.set_markup(infos[self._counter])

        msg = 'Developer mode enabled'
        if self._model.get_dev_mode():
            if not msg in infos:
                infos.append(msg)
        elif msg in infos:
            infos.remove(msg)
        msg = 'Advanced  mode enabled'
        if self._model.get_adv_mode():
            if not msg in infos:
                infos.append(msg)
        elif msg in infos:
            infos.remove(msg)
        if len(infos) == 0:
            return True
        self._counter = (self._counter + 1) % len(infos)
        return True


    def on_timer_status(self):
        """ handler to get OpenSAND status from model periodicaly """
        # determine the current status of the platform at first refresh
        # (ie. just after opensand manager startup)
        if self._first_refresh:
            # check if we have main components but
            # do not stay refreshed after INIT_ITER iterations
            if self._refresh_iter <= INIT_ITER and \
               (not self._model.main_hosts_found() or
                not self._model.is_collector_functional()):
                self._log.debug("platform status is not fully known, " \
                                "wait a little bit more...")
                self._refresh_iter = self._refresh_iter + 1
            else:
                if self._refresh_iter > INIT_ITER:
                    self._log.warning("the mandatory components were not "
                                      "found on the system, the platform "
                                      "won't be able to start")
                    self._log.info("you will need at least a satellite, "
                                   "a gateway and a ST: "
                                   "please deploy the missing component(s), "
                                   "they will be automatically detected")
                
                if not self._model.is_collector_functional():
                    self._log.warning("The OpenSAND collector is not known. "
                                      "The probes will not be available.")

                # check if some components are running
                state = self._model.is_running()
                # if at least one application is started at manager startup,
                # let do as if opensand was started
                if state:
                    gobject.idle_add(self._eventrun.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # disable the 'deploy opensand' and 'save config' buttons
                    gobject.idle_add(self._eventrun.disable_deploy_buttons, True,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    if self._refresh_iter <= INIT_ITER:
                        self._log.info("Platform is currently started")
                else:
                    gobject.idle_add(self._eventrun.disable_start_button, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    # enable the 'deploy opensand' and 'save config' buttons
                    gobject.idle_add(self._eventrun.disable_deploy_buttons, False,
                                     priority=gobject.PRIORITY_HIGH_IDLE+20)
                    if self._refresh_iter <= INIT_ITER:
                        self._log.info("Platform is currently stopped")
                # opensand manager is now ready to be used
                self._first_refresh = False
                if self._refresh_iter <= INIT_ITER:
                    self._log.info("OpenSAND Manager is now ready, have fun !")

        # update event GUI
        # calling refresh on run view remove tooltips (due to queue draw ?)
        #  => do not refresh that when we are not on run view
        if self._current_page == self._pages['run']:
            gobject.idle_add(self._eventrun.refresh,
                             priority=gobject.PRIORITY_HIGH_IDLE+20)
        
        # Update simulation state for the probe view
        gobject.idle_add(self._eventprobe.simu_state_changed,
                         priority=gobject.PRIORITY_HIGH_IDLE+20)

        
        # restart timer
        return True


    def on_about_activate(self, source=None, event=None):
        """ event handler for about button """
        about = AboutDialog()
        about.run()
        about.close()

    def on_info_activate(self, source=None, event=None):
        """ event handler for info button """
        text = "The default configuration files are stored in the <i>" + \
               OPENSAND_PATH + "</i> folder and its subfolders.\n\nTBC"
        info_popup(text, "OpenSAND Manager - Information")


    def on_notebook_switch_page(self, notebook, page, page_num):
        """ notebook page changed """
        # on configuration tab keep page if parameters have not been saved
        if self._current_page == self._pages['conf'] and \
           page_num != self._pages['conf']:
           #and not self._model.is_running():
            try:
                if self._eventconf.is_modified():
                    text =  "The configuration was not saved\n\n" \
                            "Do you want to save it ?"
                    ret = yes_no_popup(text,
                                       "Save Configuration - OpenSAND Manager",
                                       gtk.STOCK_DIALOG_INFO)
                    if ret == gtk.RESPONSE_YES:
                        self._eventconf.on_save_conf_clicked()
                    else:
                        self._eventconf.update_view()
                        self._eventresource.update_view()
            except Exception, msg:
                self._log.warning("error when trying to check if configuration "
                                  "was modified: " + str(msg))

        # on tools tab keep page if some tools were modified
        if self._current_page == self._pages['tools'] and \
           page_num != self._pages['tools']:
           #and not self._model.is_running():
            # if the save button is sensitive,
            # the configuration may have changed
            if self._ui.get_widget('save_tool_conf').is_sensitive():
                text =  "The tools configuration was not saved\n\n" \
                        "Do you want to save it ?"
                ret = yes_no_popup(text, "Save Tools - OpenSAND Manager",
                                   gtk.STOCK_DIALOG_INFO)
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
            if self._eventresource is not None:
                self._eventresource.activate(False)
            if self._eventtool is not None:
                self._eventtool.activate(False)
            if self._eventprobe is not None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['conf']:
            if self._eventrun is not None:
                self._eventrun.activate(False)
            if self._eventconf is not None:
                self._eventconf.activate(True)
            if self._eventresource is not None:
                self._eventresource.activate(False)
            if self._eventtool is not None:
                self._eventtool.activate(False)
            if self._eventprobe is not None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['resources']:
            if self._eventrun is not None:
                self._eventrun.activate(False)
            if self._eventconf is not None:
                self._eventconf.activate(False)
            if self._eventresource is not None:
                self._eventresource.activate(True)
                self._eventresource.update_view()
            if self._eventtool is not None:
                self._eventtool.activate(False)
            if self._eventprobe is not None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['tools']:
            if self._eventconf is not None:
                self._eventconf.activate(False)
            if self._eventresource is not None:
                self._eventresource.activate(False)
            if self._eventrun is not None:
                self._eventrun.activate(False)
            if self._eventtool is not None:
                self._eventtool.activate(True)
            if self._eventprobe is not None:
                self._eventprobe.activate(False)
        elif page_num == self._pages['probes']:
            if self._eventconf is not None:
                self._eventconf.activate(False)
            if self._eventresource is not None:
                self._eventresource.activate(False)
            if self._eventrun is not None:
                self._eventrun.activate(False)
            if self._eventtool is not None:
                self._eventtool.activate(False)
            if self._eventprobe is not None:
                self._eventprobe.activate(True)

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
            self._eventconf.read_conf_free_spot()
            self._eventconf.enable_conf_buttons(False)
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
            self._eventconf.read_conf_free_spot()
            self._eventconf.enable_conf_buttons(False)
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
                                    gtk.STOCK_DIALOG_WARNING)
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
        gobject.idle_add(self._eventrun.simu_program_list_changed,
                         programs_dict)
        gobject.idle_add(self._eventprobe.simu_program_list_changed,
                         programs_dict)
    
    def on_new_program_log(self, program, name, level, message):
        """ called when an environment plane log is received """
        gobject.idle_add(self._eventrun.simu_program_new_log,
                         program, name, level, message)

    def on_new_probe_value(self, probe, timestamp, value):
        """ called when a new probe value is received """
        gobject.idle_add(self._eventprobe.new_probe_value,
                         probe, timestamp, value)
    

##### TEST #####
if __name__ == "__main__":
    from opensand_manager_core.loggers.manager_log import ManagerLog
    from opensand_manager_core.opensand_model import Model

    LOGGER = ManagerLog('debug', True, True, True)
    MODEL = Model(LOGGER)

    VIEW = View(MODEL, LOGGER)
    VIEW.run()

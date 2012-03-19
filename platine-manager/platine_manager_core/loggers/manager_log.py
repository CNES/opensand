#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
manager_log.py - logger for Platine Manager
"""

try:
    import gtk
except ImportError:
    GUI = False
else:
    GUI = True
import gobject
import logging
import syslog
import sys
import time

from platine_manager_core.loggers.syslog_handler import SysLogHandler

class ManagerLog():
    """ logger for Platine manager that print messages in GUI
        and with a syslog logger """
    def __init__(self, level='debug',
                 enable_std=True, enable_gui=False,
                 enable_syslog=False,
                 logger_name='PtManager'):
        """ constructor, initialization """
        levels = {
                    'debug'  : 0,
                    'info'   : 1,
                    'warning': 2,
                    'error'  : 3,
                  }

        self._gui = enable_gui
        if self._gui and not GUI:
            print "Warning: cannot enable GUI because gtk is not installed"
            self._gui = False
        self._log = enable_std or enable_syslog
        self._level = levels[level]

        self._buff = None
        self._text_widget = None
        self._ui = None

        self._logger = None

        if self._log:
            self._logger = logging.getLogger(logger_name)
            self._logger.setLevel(logging.DEBUG)

        # GUI logger
        if self._gui:
            self._buff = gtk.TextBuffer()
            red = self._buff.create_tag('red')
            red.set_property('foreground', 'red')
            orange = self._buff.create_tag('orange')
            orange.set_property('foreground', 'orange')
            green = self._buff.create_tag('green')
            green.set_property('foreground', 'green')

        if enable_syslog:
            # syslog logger
            log = SysLogHandler('PtManager', syslog.LOG_PID,
                                syslog.LOG_DAEMON)
            self._logger.addHandler(log)

        if enable_std:
            # print logs in terminal
            log = logging.StreamHandler(sys.stdout)
            formatter = logging.Formatter("%(asctime)s - %(name)s - "
                                          "%(levelname)s - %(message)s")
            log.setFormatter(formatter)
            self._logger.addHandler(log)


    def _print_(self, tag, color, text):
        """ print message in a text view """
        if self._text_widget != None:
            self._buff.insert(self._buff.get_end_iter(), \
                              time.strftime("%H:%M:%S ", time.gmtime()))
            self._buff.insert_with_tags_by_name(self._buff.get_end_iter(), \
                                                tag, color)
            self._buff.insert(self._buff.get_end_iter(), text + '\n')
            self._buff.place_cursor(self._buff.get_end_iter())
            widget = self._ui.get_widget(self._text_widget)
            widget.scroll_to_mark(self._buff.get_insert(), 0.0, False, 0, 0)
        return False

    def run(self, gui, text_widget = ''):
        """ attach the logger to the text widget if GUI is activated """
        if self._gui:
            self._ui = gui
            self._ui.get_widget(text_widget).set_buffer(self._buff)
            self._text_widget = text_widget

    def error(self, text):
        """ print an error in text view and/or in logger """
        if self._log:
            self._logger.error(text)
        if self._gui:
            gobject.idle_add(self._print_, '[ERROR]: ', 'red', text)

    def info(self, text):
        """ print an information in text view and/or in logger """
        if self._level > 1:
            return
        if self._log:
            self._logger.info(text)
        if self._gui:
            gobject.idle_add(self._print_, '[INFO]: ', 'green', text)

    def warning(self, text):
        """ print a warning  in text view and/or in logger """
        if self._level > 2:
            return
        if self._log:
            self._logger.warning(text)
        if self._gui:
            gobject.idle_add(self._print_, '[WARNING]: ', 'orange', text)

    def debug(self, text):
        """ print a debug message in logger """
        if self._level > 0:
            return
        if self._log:
            self._logger.debug(text)


#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2016 TAS
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
manager_log.py - logger for OpenSAND Manager
"""

try:
    import gtk
except ImportError:
    GUI = False
else:
    GUI = True
import logging
import syslog
import sys

from opensand_manager_core.loggers.syslog_handler import SysLogHandler
from opensand_manager_core.loggers.levels import LOG_LEVELS, MGR_ERROR, \
                                                 MGR_WARNING, MGR_NOTICE, \
                                                 MGR_INFO, MGR_DEBUG

class ManagerLog():
    """ logger for OpenSAND manager that print messages in GUI
        and with a syslog logger """
    def __init__(self, level=0,
                 enable_std=True, enable_gui=False,
                 enable_syslog=False,
                 logger_name='sand-manager'):
        """ constructor, initialization """
        self._gui = enable_gui
        if self._gui and not GUI:
            print "Warning: cannot enable GUI because gtk is not installed"
            self._gui = False
        self._log = enable_std or enable_syslog
        self._level = LOG_LEVELS[level].level

        self._buff = None
        self._event_tab = None

        self._logger = None

        if self._log:
            self._logger = logging.getLogger(logger_name)
            self._logger.setLevel(logging.DEBUG)

        if enable_syslog:
            # syslog logger
            log = SysLogHandler('sand-manager', syslog.LOG_PID,
                                syslog.LOG_DAEMON)
            self._logger.addHandler(log)

        if enable_std:
            # print logs in terminal
            log = logging.StreamHandler(sys.stdout)
            formatter = logging.Formatter("%(asctime)s - %(name)s - "
                                          "%(levelname)s - %(message)s")
            log.setFormatter(formatter)
            self._logger.addHandler(log)


    def run(self, event_tab=None):
        """ attach the logger to the event tab if GUI is activated """
        if self._gui:
            self._event_tab = event_tab

    def error(self, text):
        """ print an error in text view and/or in logger """
        if self._log:
            self._logger.error(text)

        if self._event_tab:
            self._event_tab.message(MGR_ERROR, None, text)

    def info(self, text, mandatory=False, new_line=False):
        """ print an information in text view and/or in logger """
        if self._level < MGR_NOTICE and not mandatory:
            return

        if self._log:
            self._logger.info(text)

        if self._event_tab:
            self._event_tab.message(MGR_INFO, None, text, new_line)

    def warning(self, text):
        """ print a warning  in text view and/or in logger """
        if self._level < MGR_WARNING:
            return

        if self._log:
            self._logger.warning(text)

        if self._event_tab:
            self._event_tab.message(MGR_WARNING, None, text)

    def debug(self, text):
        """ print a debug message in logger """
        if self._level < MGR_DEBUG:
            return

        if self._log:
            self._logger.debug(text)

#        if self._event_tab:
#            self._event_tab.message(MGR_DEBUG, None, text)

#!/usr/bin/env python 
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
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
levels.py - The events level
"""

from opensand_manager_core.utils import red, blue, green, yellow

try:
    import gtk
except ImportError:
    GUI=False
else:
    GUI = True

# TODO merge with syslog ?!
(MGR_EMERGENCY, MGR_ALERT, MGR_CRITICAL,
 MGR_ERROR, MGR_WARNING, MGR_NOTICE,
 MGR_INFO, MGR_DEBUG) = range(8)

LEVEL_EVENT=10

class LogLevel(object):
    """ The logging levels """
    def __init__(self):
        self._color = 'black'
        self._bg_color = 'white'
        self._msg = ""
        self._name = ""
        self._level = 0
        self._critical = False
        self._shell_col_cb = lambda x: x
        if GUI:
            self._icon = gtk.STOCK_DIALOG_INFO

    @property
    def color(self):
        """ Get the log color """
        return self._color
    
    @property
    def bg(self):
        """ Get the log background color """
        return self._bg_color

    @property
    def msg(self):
        """ Get the log level message """
        return self._msg

    @property
    def name(self):
        """ Get the log level name """
        return self._name

    @property
    def level(self):
        """ Get the log level """
        return self._level
    
    @property
    def icon(self):
        """ Get an icon associated with log severity """
        return self._icon

    @property
    def critical(self):
        """ Check if event is critical """
        return self._critical

    def shell_color(self, message):
        """ colorise data for shell """
        return self._shell_col_cb(message)

class LogCritical(LogLevel):
    """ The critical log level """
    def __init__(self):
        LogLevel.__init__(self)
        self._color = 'white'
        self._bg_color = 'red'
        self._msg = "CRITICAL"
        self._name = "Critical"
        self._level = MGR_CRITICAL
        self._critical = True
        self._shell_col_cb = red
        if GUI:
            self._icon = gtk.STOCK_DIALOG_ERROR

class LogError(LogLevel):
    """ The error log level """
    def __init__(self):
        LogLevel.__init__(self)
        self._color = 'white'
        self._bg_color = 'red'
        self._msg = "ERROR"
        self._name = "Error"
        self._level = MGR_ERROR
        self._shell_col_cb = red
        if GUI:
            self._icon = gtk.STOCK_DIALOG_ERROR

class LogWarning(LogLevel):
    """ The warning log level """
    def __init__(self):
        LogLevel.__init__(self)
        self._color = 'orange'
        self._msg = "WARNING"
        self._name = "Warning"
        self._level = MGR_WARNING
        self._shell_col_cb = yellow
        if GUI:
            self._icon = gtk.STOCK_DIALOG_WARNING

class LogNotice(LogLevel):
    """ The notice log level """
    def __init__(self):
        LogLevel.__init__(self)
        self._color = 'blue'
        self._msg = "NOTICE"
        self._name = "Notice"
        self._shell_col_cb = blue
        self._level = MGR_NOTICE

class LogInfo(LogLevel):
    """ The info log level """
    def __init__(self):
        LogLevel.__init__(self)
        self._color = 'green'
        self._msg = "INFO"
        self._name = "Info"
        self._shell_col_cb = green
        self._level = MGR_INFO

class LogDebug(LogLevel):
    """ The debug log level """
    def __init__(self):
        LogLevel.__init__(self)
        self._level = MGR_DEBUG
        self._name = "Debug"

LOG_LEVELS = {
    MGR_DEBUG: LogDebug(),
    MGR_INFO: LogInfo(),
    MGR_NOTICE: LogNotice(),
    MGR_WARNING: LogWarning(),
    MGR_ERROR: LogError(),
    MGR_CRITICAL: LogCritical(),
#    MGR_ALERT: LogCritical(),
#    MGR_EMERGENCY: LogCritical(),
}


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

# Author: Audric Schiltknecht / Viveris Technologies
# $Id: misc.py 9613 2010-09-03 12:23:04Z aschiltkne $
''' syslog.py - syslog logger for 'logging module'
'''

import logging
import syslog

#===============================================================================
# Define a Logger for Syslog
#===============================================================================
class SysLogHandler(logging.Handler):
    ''' A syslog log handler.

    Overwrite the logging.SysLogHandler, since it is based
    on TCP/Unix socket.

    This one make use of syslog module
    '''

    priority_map = {
                logging.DEBUG: syslog.LOG_DEBUG,
                logging.INFO: syslog.LOG_INFO,
                logging.WARNING: syslog.LOG_WARNING,
                logging.ERROR: syslog.LOG_ERR,
                logging.CRITICAL: syslog.LOG_CRIT,
                logging.FATAL: syslog.LOG_CRIT,
            }

    def __init__(self, ident='', logopt=0, facility=syslog.LOG_USER):
        """
        Initialize a handler.
        """
        logging.Handler.__init__(self)
        self.facility = facility

        syslog.openlog(ident, logopt, facility)

    def close(self):
        """
        Closes syslog.
        """
        syslog.closelog()
        logging.Handler.close(self)

    def emit(self, record):
        """
        Emit a record.

        The record is formatted, and then sent to the syslog server. If
        exception information is present, it is NOT sent to the server.
        """
        msg = self.format(record)
        try:
            priority = self.priority_map[record.levelno]
            syslog.syslog(priority, msg)
        except Exception:
            self.handleError(record)

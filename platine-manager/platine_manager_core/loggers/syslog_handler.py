# -*- coding: utf-8 -*-
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

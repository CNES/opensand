#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
my_exceptions.py - exceptions for Platine Daemon
"""

class Timeout(Exception):
    """ timeout exception """
    pass

class InstructionError(Exception):
    """ wrong instruction received """
    def __init__(self, value):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class XmlError(Exception):
    """ error when parsing an XML file """
    pass

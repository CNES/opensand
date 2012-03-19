#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
my_exceptions.py - exceptions for Platine Manager
"""

class InstructionError(Exception):
    """ wrong instruction received """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class ModelException(Exception):
    """ error with model """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class CommandException(Exception):
    """ error with host command client """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class RunException(Exception):
    """ error with run view """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class ConfException(Exception):
    """ error with conf view """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class ProbeException(Exception):
    """ error with probe view """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class ToolException(Exception):
    """ error with tool view """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)

class ViewException(Exception):
    """ error with view """
    def __init__(self, value=''):
        Exception.__init__(self)
        self.value = value
    def __str__(self):
        return repr(self.value)


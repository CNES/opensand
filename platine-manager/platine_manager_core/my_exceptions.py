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

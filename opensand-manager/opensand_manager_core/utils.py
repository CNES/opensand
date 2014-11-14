#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
# Copyright © 2014 CNES
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
opensand_manager_core/utils.py - Utilities for OpenSAND Manager
"""

import shutil
import os

OPENSAND_PATH = "/usr/share/opensand/"
COL_RED="\033[31m"
COL_GREEN="\033[32m"
COL_BLUE="\033[34m"
COL_BOLD="\033[1m"
COL_END="\033[0m"


def _bold(msg):
    """ return the message with bold characters """
    return COL_BOLD + msg + COL_END

def _color(msg, color, bold):
    """ return the message colored """
    msg = color + msg + COL_END
    if bold:
        return bold(msg)
    return msg

def red(msg, bold=False):
    """ return the message colored in red """
    return _color(msg, COL_RED, bold)

def green(msg, bold=False):
    """ return the message colored in green """
    return _color(msg, COL_GREEN, bold)

def blue(msg, bold=False):
    """ return the message colored in blud """
    return _color(msg, COL_BLUE, bold)

def copytree(src, dst):
    """ Recursively copy a directory tree using copy2()
        Adapted from shutil.copytree """
    names = os.listdir(src)

    if not os.path.exists(dst):
        os.makedirs(dst)
    for name in names:
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if os.path.isdir(srcname):
                copytree(srcname, dstname)
            else:
                shutil.copy2(srcname, dstname)
        except (IOError, os.error):
            raise
    try:
        shutil.copystat(src, dst)
    except OSError:
        raise



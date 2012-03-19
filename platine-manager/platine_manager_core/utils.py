#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
platine_manager_core/utils.py - Utilities for Platine Manager
"""

import shutil
import os

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



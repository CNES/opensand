#!/usr/bin/env python 
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 CNES
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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
gse.py - The GSE encapsulation module
"""

from platine_manager_core.encap_module import EncapModule

class GseModule(EncapModule):
    
    _name = 'GSE'
    
    def __init__(self):
        EncapModule.__init__(self)
        self._upper['transparent'] = ("IP",
                                      "ROHC",)
        self._upper['regenerative'] = ("ATM/AAL5",
                                       "MPEG2-TS",)
        self._xml = 'gse.conf'
        self._xsd  = 'gse.xsd'
        start = "<span size='x-large' foreground='#1088EB'><b>"
        end = "</b></span>"
        self._description = "%sGSE encapsulation plugin for Platine%s" % \
                            (start, end)
        self._condition['dvb-rcs'] = False
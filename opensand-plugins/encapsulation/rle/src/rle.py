#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 CNES
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

# Author: Aurélien DELRIEU / <adelrieu@toulouse.viveris.com>

"""
rle.py - The RLE encapsulation module
"""

from opensand_manager_core.module import EncapModule

class RleModule(EncapModule):
    
    _name = 'RLE'
    
    def __init__(self):
        super(RleModule, self).__init__()
        self._xml = 'rle.conf'
        self._xsd  = 'rle.xsd'
        start = "<span size='x-large' foreground='#1088EB'><b>"
        end = "</b></span>"
        self._description = "%sRLE encapsulation plugin for OpenSAND%s" % \
                            (start, end)
        
        self._add_config(self.TRANSPARENT, self.DVB_RCS2, self.RETURN_LINK,
                         False, True)
        self._add_config(self.REGENERATIVE, self.DVB_RCS2, self.FORWARD_LINK,
                         False, True)
        self._add_config(self.REGENERATIVE, self.DVB_RCS2, self.RETURN_LINK,
                         False, True)

#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to stresent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2017 CNES
#
#
# This file is part of the OpenSAND testbed.
#
#
# OpenSAND is free software : you can redistri will be useful, but WITHOUT
# ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see http://www.gnu.org/licenses/.
#
#

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

import os

for lib in os.listdir(os.path.dirname(__file__)):
    if not lib.endswith(".py"):
        continue
    if os.path.basename(lib) == os.path.basename(__file__):
        continue
    __import__(os.path.basename(lib).rsplit('.', 1)[0],
               globals(), locals(), [''], -1)

#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 TAS
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
opensand_band.py - The OpenSAND bandwidth representation
"""

import os
from optparse import OptionParser

from opensand_manager_core.carriers_band import CarriersBand
from opensand_manager_core.utils import OPENSAND_PATH, ID, GW, SPOT, \
        RETURN_UP, FORWARD_DOWN, DVB_RCS, DVB_RCS2
from opensand_manager_core.opensand_xml_parser import XmlParser

XSD = OPENSAND_PATH + "core_global.xsd"


class OpenSandBand():
    """ The OpenSAND Bandwidth representation """

    def __init__(self):
        self._carriers_band = CarriersBand()

        default = os.path.join(os.environ['HOME'], ".opensand/default")

        opt_parser = OptionParser()
        opt_parser.add_option("-s", "--scenario", dest="scenario",
                              default=default, metavar="SCENARIO_PATH",
                              help="the OpenSAND scenario")
        opt_parser.add_option("-r", "--return", action="store_true",
                              dest="ret", default=False,
                              help="compute band information for return link")
        opt_parser.add_option("-f", "--forward", action="store_true",
                              dest="forward", default=False,
                              help="compute band information for forward link")
        
        (options, args) = opt_parser.parse_args()
        
        # if no link option choose both
        if not options.ret and not options.forward:
            options.ret = True
            options.forward = True

        config = XmlParser(os.path.join(options.scenario, "core_global.conf"), XSD)
        elem = config.get("//common/return_link_standard")
        return_link_std = config.get_value(elem)
        
        if options.ret:
            print \
"**************************************************************************\n" \
"****************************** RETURN ************************************\n" \
"**************************************************************************\n"
            link = RETURN_UP
            section_path = "%s_band" % link
            for KEY in config.get_keys(config.get(section_path)):
                if KEY.tag == SPOT:
                    content = config.get_element_content(KEY)
                    print "spot %s gw %s" % (content[ID], content[GW])
                    self._carriers_band.parse(link, config, KEY)
                    self._carriers_band.modcod_def(options.scenario, config,
                                                   return_link_std)
                    print self._carriers_band.str()
                    print
        if options.forward:
            print \
"**************************************************************************\n" \
"****************************** FORWARD ***********************************\n" \
"**************************************************************************\n"
            link = FORWARD_DOWN
            section_path = "%s_band" % link
            for KEY in config.get_keys(config.get(section_path)):
                if KEY.tag == SPOT:
                    content = config.get_element_content(KEY)
                    print "spot %s gw %s" % (content[ID], content[GW])
                    self._carriers_band.parse(link, config, KEY)
                    self._carriers_band.modcod_def(options.scenario, config,
                                                   return_link_std)
                    print self._carriers_band.str()
                    print


if __name__ == "__main__":
    BAND = OpenSandBand()

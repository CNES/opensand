#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
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
from math import floor
from fractions import Fraction
from optparse import OptionParser
from opensand_manager_core.opensand_xml_parser import XmlParser
from opensand_manager_core.utils import GreedyConfigParser

XSD="/usr/share/opensand/core_global.xsd"


class OpenSandBand():
    """ The OpenSAND Bandwidth representation """

    def __init__(self):
        self._bandwidth = 0.0
        self._roll_off = 0.0
        self._categories = {}
        self._carriers_groups = {}
        self._fmt_group = {}
        self._fmt = {}

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

        if options.ret:
            print \
"**************************************************************************\n" \
"****************************** RETURN ************************************\n" \
"**************************************************************************\n"
            self._parse(options.scenario, "up_return")
            print str(self)
            print
        if options.forward:
            print \
"**************************************************************************\n" \
"****************************** FORWARD ***********************************\n" \
"**************************************************************************\n"
            self._parse(options.scenario, "down_forward")
            print str(self)


    def _reset(self):
        """ reset all data """
        self._bandwidth = 0.0
        self._roll_off = 0.0
        self._categories = {}
        self._carriers_groups = {}
        self._fmt_group = {}
        self._fmt = {}

    def _parse(self, scenario, link):
        """ parse configuration and get results """
        self._reset()
        config = XmlParser(os.path.join(scenario, "core_global.conf"), XSD)

        # bandwidth
        xpath = "//%s_band/bandwidth" % link
        self._bandwidth = float(config.get_value(config.get(xpath)))
        # roll-off
        xpath = "//%s_band/roll_off" % link
        self._roll_off = float(config.get_value(config.get(xpath)))
        # carriers
        xpath = "//%s_band/carriers_distribution" % link
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            ratios = content["ratio"].replace(',', ';')
            ratios = ratios.replace('-', ';')
            ratios = map(lambda x: float(x), ratios.split(';'))
            fmt_groups =  content["fmt_group"].replace(',', ';')
            fmt_groups = fmt_groups.replace('-', ';')
            fmt_groups = fmt_groups.split(';')
            self._add_carrier(content["category"],
                              ratios,
                              float(content["symbol_rate"]),
                              fmt_groups)

        # fmt groups
        xpath = "//%s_band/fmt_groups" % link
        for group in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(group)
            self._add_fmt_group(content["id"],
                                content["fmt_id"])


        simu = GreedyConfigParser()
        simu.read(os.path.join(scenario, "simulation_files.ini"))
        file = simu.get("global", "/configuration/global/%s_modcod_def/text()" % link)
        # fmt ids
        self._load_fmt(file)

        self._compute()

    def _load_fmt(self, path):
        """ load the FMT definitions """
        with  open(path, 'r') as modcod_def:
            for line in modcod_def:
                if (line.startswith("/*") or 
                    line.isspace() or
                    line.startswith('nb_fmt')):
                    continue
                elts = line.split()
                if len(elts) != 5:
                    continue
                if not elts[0].isdigit:
                    continue
                # id, modulation, coding_rate, spectral_efficiency, required Es/N0
                # self._fmt[7] = _Fmt("QPSK", "6/7", 1.714, 9.34)
                self._fmt[int(elts[0])] = _Fmt(elts[1], elts[2],
                                               float(elts[3]), float(elts[4]))

    def _add_carrier(self, name, ratios, symbol_rate_baud, fmt_groups):
        """ add a new category """
        if not name in self._categories:
            self._categories[name] = []

        carriers = _CarriersGroup(ratios, symbol_rate_baud, fmt_groups)
        self._categories[name].append(carriers)

    def _add_fmt_group(self, group_id, fmt_ids):
        """ add a FMT group """
        ids = fmt_ids.split(';')
        id_list = []
        for fmt_id in ids:
            if '-' in fmt_id:
                (mini, maxi) = fmt_id.split('-')
                id_list.extend(range(int(mini), int(maxi) + 1))
            else:
                id_list.append(int(fmt_id))

        self._fmt_group[group_id] = id_list

    def _check(self):
        """ check that everything is ok """
        for name in self._categories:
            for carriers in self._categories[name]:
                if len(carriers.fmt_groups) != len(carriers.ratios):
                    raise Exception("not the same numbers of ratios and fmt "
                                    "groups")
                for group in carriers.fmt_groups:
                    if not group in self._fmt_group:
                        raise KeyError(group)

        for gid in self._fmt_group:
            for fmt_id in self._fmt_group[gid]:
                if not fmt_id in self._fmt:
                    raise KeyError(fmt_id)

    def _compute(self):
        """ the configuration was initialized """
        self._check()
        weighted_sum = 0.0
        for name in self._categories:
            for carriers in self._categories[name]:
                ws = sum(carriers.ratios) * carriers.symbol_rate / 1E6
                weighted_sum += ws

        # TODO check that this is not 0

        for name in self._categories:
            for carriers in self._categories[name]:
                nbr = floor((sum(carriers.ratios) / weighted_sum) *
                            (self._bandwidth / (1 + self._roll_off)))
                carriers.number = nbr
                
    def _get_carrier_bitrates(self, carriers):
        """ get the maximum bitrate per carriers group """
        br = []
        i = 0
        for ratio in carriers.ratios:
            rs = carriers.symbol_rate * ratio / sum(carriers.ratios)
            max_fmt = max(self._fmt_group[carriers.fmt_groups[i]])
            min_fmt = min(self._fmt_group[carriers.fmt_groups[i]])
            fmt = self._fmt[max_fmt]
            max_br = rs * fmt.modulation * fmt.coding_rate
            fmt = self._fmt[min_fmt]
            min_br = rs * fmt.modulation * fmt.coding_rate
            br.append((min_br, max_br))
            i += 1
        return br

    def _get_max_bitrate(self, name):
        """ get the maximum bitrate for a given category """
        bitrate = 0
        for carriers in self._categories[name]:
            i = 0
            for ratio in carriers.ratios:
                rs = carriers.symbol_rate * ratio / sum(carriers.ratios)
                max_fmt = max(self._fmt_group[carriers.fmt_groups[i]])
                fmt = self._fmt[max_fmt]
                br = rs * fmt.modulation * fmt.coding_rate
                bitrate += br
                i += 1
        return bitrate
    
    def _get_min_bitrate(self, name):
        """ get the minimum bitrate for a given category """
        bitrate = 0
        for carriers in self._categories[name]:
            i = 0
            for ratio in carriers.ratios:
                rs = carriers.symbol_rate * ratio / sum(carriers.ratios)
                min_fmt = min(self._fmt_group[carriers.fmt_groups[i]])
                fmt = self._fmt[min_fmt]
                br = rs * fmt.modulation * fmt.coding_rate
                bitrate += br * carriers.number
                i += 1
        return bitrate

    def _get_carriers_number(self, name):
        """ get the carriers number for a given category """
        nbr = 0
        for carriers in self._categories[name]:
            nbr += carriers.number
        return nbr

    def __str__(self):
        """ print band representation """
        output = "BAND: %sMhz roll-off=%s" % (self._bandwidth, self._roll_off)
        for name in self._categories:
            output += "\n\nCATEGORY %s" % (name)
            i = 0
            for carriers in self._categories[name]:
                rates = ""
                for (min_rate, max_rate) in self._get_carrier_bitrates(carriers):
                    rates += "[%d, %d] kb/s " % (min_rate / 1000,
                                                 max_rate / 1000)
                rates = rates.rstrip()
                i += 1
                output += "\nGroup %d: %s (%s)" % \
                          (i, carriers, rates)
            output += "\n    %d carrier(s)" % (self._get_carriers_number(name))
            output += "\n    Bitrate [%d, %d] kb/s" % (
                      (self._get_min_bitrate(name) / 1000),
                      (self._get_max_bitrate(name) / 1000))
        return output

class _CarriersGroup():
    """ The terminal categories """

    def __init__(self, ratios, symbol_rate_baud, fmt_groups):
        self._ratios = ratios
        self._symbol_rate = symbol_rate_baud
        self._fmt_groups = fmt_groups
        self._carriers_number = 0

    def __str__(self):
        return "ratio=%s Rs=%g => %d carriers" % (sum(self.ratios),
                                                  self.symbol_rate,
                                                  self.number)

    @property
    def ratios(self):
        """ get the category name """
        return self._ratios

    @property
    def symbol_rate(self):
        """ get the category name """
        return self._symbol_rate

    @property
    def fmt_groups(self):
        """ get the category name """
        return self._fmt_groups

    @property
    def number(self):
        """ get the number of carriers """
        return self._carriers_number

    @number.setter
    def number(self, carriers_number):
        """ set the number of carriers """
        self._carriers_number = carriers_number

class _Fmt():
    """ A FMT definition """

    def __init__(self, modulation, coding_rate, spectral_efficiency,
                 required_es_n0):
        self._modulation = modulation
        self._coding_rate = Fraction(coding_rate)
        self._spectral_efficiency = spectral_efficiency

    @property
    def modulation(self):
        """ get the modulation factor to apply acocrding to modulation type """
        if self._modulation == "BPSK":
            return 1
        elif self._modulation == "QPSK":
            return 2
        elif self._modulation == "8PSK":
            return 3
        elif self._modulation == "16APSK":
            return 4
        elif self._modulation == "32APSK":
            return 5

    @property
    def coding_rate(self):
        """ get the coding rate """
        return float(self._coding_rate)


if __name__ == "__main__":
    BAND = OpenSandBand()

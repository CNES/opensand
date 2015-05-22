#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 TAS
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

# Author: Bénédicte Motto / <bmotto@toulouse.viveris.com>

"""
carriers_band.py - The OpenSAND bandwidth representation
"""

import os
from fractions import Fraction

from opensand_manager_core.utils import get_conf_xpath, ROLL_OFF, \
        OPENSAND_PATH, ID, FMT_ID, FMT_GROUP, \
        RATIO, ACCESS_TYPE, SYMBOL_RATE, CATEGORY
CATEGORY
from opensand_manager_core.carrier import Carrier

XSD = OPENSAND_PATH + "core_global.xsd"


class CarriersBand():
    """ The OpenSAND Bandwidth representation """

    def __init__(self):
        self._access_type = ""
        self._bandwidth = 0.0
        self._roll_off = 0.0
        self._categories = {}
        self._carriers_groups = {}
        self._fmt_group = {}
        self._fmt = {}

        """config = self._model.get_conf().get_configuration()
        link = RETURN_UP
        section_path = "%s_band" % link
        for KEY in config.get_keys(config.get(section_path)):
            if KEY.tag == SPOT:
                content = config.get_element_content(KEY)
                self._parse(link, config, KEY)
                self._modcod_def(link, config)
        
        link = FORWARD_DOWN
        section_path = "%s_band" % link
        for KEY in config.get_keys(config.get(section_path)):
            if KEY.tag == SPOT:
                content = config.get_element_content(KEY)
                self._parse(link, config, KEY)
                self._modcod_def(link, config)"""

    def reset(self):
        """ reset all data """
        self._bandwidth = 0.0
        self._roll_off = 0.0
        self._categories = {}
        self._carriers_groups = {}
        self._fmt_group = {}
        self._fmt = {}

    def parse(self, link, config, KEY):
        """ parse configuration and get results """
        self.reset()
        # bandwidth
        xpath = "//%s/bandwidth" % config.get_path(KEY)
        self._bandwidth = float(config.get_value(config.get(xpath)))
        # roll-off
        xpath = get_conf_xpath(ROLL_OFF, link)
        self._roll_off = float(config.get_value(config.get(xpath)))
        # carriers
        xpath = "//%s/carriers_distribution" % config.get_path(KEY)
        for carrier in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(carrier)
            fmt_groups =  content[FMT_GROUP].replace(',', ';')
            fmt_groups = fmt_groups.replace('-', ';')
            fmt_groups = fmt_groups.split(';')
            self.add_carrier(content[CATEGORY],
                              content[ACCESS_TYPE],
                              content[RATIO],
                              float(content[SYMBOL_RATE]),
                              fmt_groups)

        # fmt groups
        xpath = "//%s/fmt_groups" % config.get_path(KEY)
        for group in config.get_table_elements(config.get(xpath)):
            content = config.get_element_content(group)
            self.add_fmt_group(int(content[ID]),
                                content[FMT_ID])


    def modcod_def(self, scenario, link, config):
        # ACM
        # TODO fix this
        for std in ["rcs", "s2"]:
            xpath = "//%s_modcod_def_%s" % (link, std)
            elem = config.get(xpath)
            if elem is not None:
                break
        name = config.get_name(elem)
        path = os.path.join(scenario, config.get_file_source(name))
        self.load_fmt(path)

        self.compute()

    def load_fmt(self, path):
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

    def add_carrier(self, name, access_type, ratios, symbol_rate_baud, fmt_groups):
        """ add a new category """
        if not name in self._categories:
            self._categories[name] = []

        carrier = Carrier(symbol_rate=symbol_rate_baud,
                          category=name,
                          access_type=access_type, 
                          modcod=fmt_groups,
                          ratio=ratios)
        self._categories[name].append(carrier)

    def add_fmt_group(self, group_id, fmt_ids):
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

    def check(self):
        """ check that everything is ok """
        for name in self._categories:
            for carrier in self._categories[name]:
                if len(carrier.getFmtGroups()) != len(carrier.getRatio()):
                    raise Exception("not the same numbers of ratios and fmt "
                                    "groups")
                for group in carrier.getFmtGroups():
                    if not group in self._fmt_group:
                        raise KeyError(group)

        for gid in self._fmt_group:
            for fmt_id in self._fmt_group[gid]:
                if not fmt_id in self._fmt:
                    raise KeyError(fmt_id)

    def compute(self):
        """ the configuration was initialized """
        self.check()
        weighted_sum = 0.0
        for name in self._categories:
            for carrier in self._categories[name]:
                ws = sum(carrier.getRatio()) * carrier.getSymbolRate() / 1E6
                weighted_sum += ws

        total_ratio = 0
        for name in self._categories:
            for carrier in self._categories[name]:
                total_ratio += sum(carrier.getRatio())
        
        # replace floor by round because bandwidth is calculed exactly
        # according to the number of wanted carrier 
        number = round((total_ratio / weighted_sum) *
                                (self._bandwidth / (1 + self._roll_off)))
        if number == 0:
            number = 1
                
        for name in self._categories:
            for carrier in self._categories[name]:
                nbr = int(round(number * sum(carrier.getRatio()) /
                                total_ratio))
                if nbr == 0:
                    nbr = 1
                carrier.setNbCarrier(nbr)
                
    def get_carrier_bitrates(self, carrier):
        """ get the maximum bitrate per carrier group """
        br = []
        i = 0
        for ratio in carrier.getRatio():
            rs = carrier.getSymbolRate() * ratio / sum(carrier.getRatio())
            max_fmt = max(self._fmt_group[carrier.getFmtGroups()[i]])
            min_fmt = min(self._fmt_group[carrier.getFmtGroups()[i]])
            fmt = self._fmt[max_fmt]
            max_br = rs * fmt.modulation * fmt.coding_rate
            fmt = self._fmt[min_fmt]
            min_br = rs * fmt.modulation * fmt.coding_rate
            br.append((min_br, max_br))
            i += 1
        return br

    def get_max_bitrate(self, name, access_type):
        """ get the maximum bitrate for a given category """
        bitrate = 0
        for carrier in self._categories[name]:
            if carrier.getAccessType() != access_type:
                continue
            i = 0
            for ratio in carrier.getRatio():
                rs = carrier.getSymbolRate() * ratio / sum(carrier.getRatio())
                max_fmt = max(self._fmt_group[carrier.getFmtGroups()[i]])
                fmt = self._fmt[max_fmt]
                br = rs * fmt.modulation * fmt.coding_rate
                bitrate += br * carrier.getNbCarrier()
                i += 1
        return bitrate
    
    def get_min_bitrate(self, name, access_type):
        """ get the maximum bitrate for a given category """
        bitrate = 0
        for carrier in self._categories[name]:
            if carrier.getAccessType() != access_type:
                continue
            i = 0
            for ratio in carrier.getRatio():
                rs = carrier.getSymbolRate() * ratio / sum(carrier.getRatio())
                min_fmt = min(self._fmt_group[carrier.getFmtGroups()[i]])
                fmt = self._fmt[min_fmt]
                br = rs * fmt.modulation * fmt.coding_rate
                bitrate += br * carrier.getNbCarrier()
                i += 1
        return bitrate

    def get_carriers_number(self, name, access):
        """ get the carriers number for a given category """
        nbr = 0
        for carrier in self._categories[name]:
            if carrier.getAccessType() != access:
                continue
            nbr += carrier.getNbCarrier()
        return nbr
    
    def get_access_type(self, name):
        """ get the access types in a category """
        access_types = []
        for carrier in self._categories[name]:
            access_types.append(carrier.getAccessType())
        return set(access_types)

    def str(self):
        """ print band representation """
        output = "  BAND: %sMhz roll-off=%s" % (self._bandwidth, self._roll_off)
        for name in self._categories:
            output += "\n\n  CATEGORY %s" % (name)
            for access in self.get_access_type(name):
                output += "\n    * Access type: %s" % access
                i = 0
                for carrier in self._categories[name]:
                    if carrier.getAccessType() != access:
                        continue
                    rates = ""
                    for (min_rate, max_rate) in self.get_carrier_bitrates(carrier):
                        rates += "[%d, %d] kb/s " % (min_rate / 1000,
                                                     max_rate / 1000)
                    rates += "per carrier"
                    i += 1
                    output += "\n  Group %d: %s (%s)" % \
                              (i, carrier, rates)
                output += "\n      %d carrier(s)" % (self.get_carriers_number(name,
                                                                              access))
                output += "\n      Total bitrate [%d, %d] kb/s" % (
                    (self.get_min_bitrate(name, access) / 1000),
                    (self.get_max_bitrate(name, access) / 1000))
        return output

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


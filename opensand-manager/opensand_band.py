#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
opensand_band.py - The OpenSAND bandwidth representation
"""

import os
import sys
from math import ceil
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
        if options.ret and options.forward:
            print "Please choose between return and forward option"
            opt_parser.print_help()
            sys.exit(1)
        if not options.ret and not options.forward:
            print "Please choose an option between return and forward"
            opt_parser.print_help()
            sys.exit(1)
        link = ""
        if options.ret:
            link = "up_return"
        if options.forward:
            link = "down_forward"

        self._parse(options.scenario, link)

    def _parse(self, scenario, link):
        """ parse configuration and get results """
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
            self._add_carrier(content["category"],
                              float(content["ratio"]),
                              float(content["symbol_rate"]),
                              content["fmt_group"])

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

    def _add_carrier(self, name, ratio, symbol_rate_baud, fmt_group):
        """ add a new category """
        if not name in self._categories:
            self._categories[name] = []

        carriers = _CarriersGroup(ratio, symbol_rate_baud, fmt_group)
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
                if not carriers.fmt_group in self._fmt_group:
                    raise KeyError(carriers.fmt_group)

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
                ws = carriers.ratio * carriers.symbol_rate / 1E6
                weighted_sum += ws

        # TODO check that this is not 0

        for name in self._categories:
            for carriers in self._categories[name]:
                nbr = ceil((carriers.ratio / weighted_sum) *
                           (self._bandwidth / (1 + self._roll_off)))
                carriers.number = nbr
                
    def _get_carrier_bitrate(self, carriers):
        """ get the maximum bitrate per carriers group """
        rs = carriers.symbol_rate * carriers.number
        max_fmt = max(self._fmt_group[carriers.fmt_group])
        fmt = self._fmt[max_fmt]
        br = rs * fmt.modulation * fmt.coding_rate
        return br

    def _get_max_bitrate(self, name):
        """ get the maximum bitrate for a given category """
        bitrate = 0
        for carriers in self._categories[name]:
            rs = carriers.symbol_rate * carriers.number
            max_fmt = max(self._fmt_group[carriers.fmt_group])
            fmt = self._fmt[max_fmt]
            br = rs * fmt.modulation * fmt.coding_rate
            bitrate += br
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
                i += 1
                output += "\nGroup %d: %s (%d kb/s)" % \
                          (i, carriers,
                           self._get_carrier_bitrate(carriers) / 1000)
            output += "\n    %d carrier(s)" % (self._get_carriers_number(name))
            output += "\n    Bitrate %d kb/s" % \
                      (self._get_max_bitrate(name) / 1000)
        return output

class _CarriersGroup():
    """ The terminal categories """

    def __init__(self, ratio, symbol_rate_baud, fmt_group):
        self._ratio = ratio
        self._symbol_rate = symbol_rate_baud
        self._fmt_group = fmt_group
        self._carriers_number = 0

    def __str__(self):
        return "ratio=%s Rs=%g => %d carriers" % (self.ratio,
                                                  self.symbol_rate,
                                                  self.number)

    @property
    def ratio(self):
        """ get the category name """
        return self._ratio

    @property
    def symbol_rate(self):
        """ get the category name """
        return self._symbol_rate

    @property
    def fmt_group(self):
        """ get the category name """
        return self._fmt_group

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
    print str(BAND)

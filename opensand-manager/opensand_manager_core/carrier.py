#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
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

# Author : Maxime POMPA


"""
carrier.py - Satellite carrier handling
"""

import numpy as np
import re

from opensand_manager_core.utils import DAMA, SCPC, ALOHA, VCM, CCM, ACM, \
                                        S2, RCS, RCS2, DVB_RCS, DVB_RCS2

class Carrier :
    """
    Create a carrier
        Arg :   _symbol_rate    : float
                _nb_carrier     : integer (default = 1)
                _category       : integer (default = 1)
                _access_type    : string (default = 'CCM')
                _fmt_groups     : list of string (defautl empty)
                _list_modcod    : list of string (default empty)
                _ratio          : dictionnary with ['modcod':ratio] only use for VCM
    """

    def __init__(self, symbol_rate = 0, nb_carrier = 1, category = 0,
                 access_type = 'CCM', fmt_groups = '1', modcod = '1',
                 ratio = '50') :
        self._symbol_rate = symbol_rate
        # initialize in set_access_type
        self._access_type = None
        self._str_fmt_grp = fmt_groups
        self._str_modcod = modcod
        self._str_ratio = ratio
        self._list_modcod = self.parser(modcod)
        self._fmt_groups = self.parser(fmt_groups)
        self._ratio = self.parser(ratio)
        self._nb_carrier = nb_carrier

        self._rates = []

        self.set_access_type(access_type)

        self._category = int(category)

        self._X = []                #Position in X to trace the graphic
        self._Y = []                #Position in Y to trace the graphic

    ##################################################

    def parser(self, list_str):
        ids = []
        if type(list_str) is not list:
            ids = re.split(r"[;, ]+", list_str)
        else:
            for elm in list_str:
                for elm_id in re.split(r"[;, ]+", elm):
                    ids.append(elm_id)

        id_list = []
        for elm_id in ids:
            if '-' in elm_id:
                (mini, maxi) = elm_id.split('-')
                id_list.extend(range(int(mini), int(maxi) + 1))
            else:
                id_list.append(int(elm_id))

        return id_list


    ##################################################

    def calculate_xy(self, roll_off = 0, offset = 0):
        """
        Calculate all the X and Y position of the carrier to trace the graphic
        """
        half_rolloff = float(roll_off) / 2

        self._X = []
        self._Y = []

        symbol_rate = float(self._symbol_rate) / 1E6

        """
        We do not use directly self._X to stock the value of linspace
        because it will create a numpy.ndarray.
        """
        for value in np.linspace(0,1,100):
            self._X.append(value)
            self._Y.append(np.sin(np.pi*value))
        for i, value in enumerate(self._X):
            self._X[i] = (float(self._X[i]) * symbol_rate + symbol_rate * half_rolloff)

        self._X.insert(0, 0)
        self._Y.insert(0, 0)
        self._X.append(symbol_rate * (1 + roll_off))
        self._Y.append(0)

        for i, value in enumerate(self._X):
            self._X[i] = float(self._X[i])+offset


    ##################################################

    def set_symbol_rate(self, symbol_rate):
        self._symbol_rate = symbol_rate

    def set_fmt_groups(self, fmt_groups):
        self._str_fmt_grp = fmt_groups
        self._fmt_groups = self.parser(fmt_groups)

    def set_category(self, category):
        self._category = int(category)

    def set_access_type(self, access_type):
        self._access_type = access_type
       
    def set_modcod(self, modcod):
        self._str_modcod = modcod
        self._list_modcod = self.parser(modcod)

    def set_ratio(self, ratio):
        self._str_ratio = ratio
        self._ratio = self.parser(ratio)

    def set_nb_carriers(self, nb_carrier):
       self._nb_carrier = nb_carrier

    ##################################################

    def get_symbol_rate(self):
        return self._symbol_rate

    def get_str_fmt_grp(self):
        return self._str_fmt_grp

    def get_str_modcod(self):
        return self._str_modcod

    def get_modcod(self):
        return self._list_modcod

    def get_category(self):
        return int(self._category)

    def get_old_category(self, config):
        return get_category_name(config, self._category)

    def get_access_type(self):
        return self._access_type

    def get_old_access_type(self):
        if self._access_type == CCM:
            return ACM
        else:
            return self._access_type

    def get_str_ratio(self):
        return self._str_ratio

    def get_ratio(self):
        return self._ratio

    def get_fmt_groups(self):
        return self._fmt_groups

    def get_std(self, return_link_standard):
        std = None
        if self._access_type in [SCPC, CCM, ACM, VCM]:
            std = S2
        elif self._access_type in [DAMA, ALOHA]:
            if return_link_standard == DVB_RCS2:
                std = RCS2
            else:
                std = RCS
        else:
            raise Exception("Unknown access type %s" % self._access_type)
        return std

    def get_x(self):
        return self._X

    def get_y(self):
        return self._Y

    def get_rates(self):
        return self._rates

    def set_rates(self, rates):
        self._rates = rates

    def get_bandwidth(self, roll_off):
        """
        Return the total bandwith of th carrier
        to get only the symbol rate use get_symbol_rate()
        """
        return float(self._symbol_rate) * (roll_off + 1) * self._nb_carrier

    def get_nb_carriers(self):
        return self._nb_carrier

    ##################################################

    def __str__(self):
        return "ratio=%s Rs=%g => %d carriers" % (sum(self._ratio),
                                                  self._symbol_rate,
                                                  self._nb_carrier)

##################################################
#This function finds category index from name
def get_category_name(config, value):
    #Get list of categories
    category_type = config.get_simple_type("Category")
    if category_type is None \
       or category_type["enum"] is None \
       or len(category_type["enum"]) <= 0:
        return ""

    #Check value validity
    if len(category_type["enum"]) <= value:
        value = 0

    return category_type["enum"][value]

##################################################
#This function finds category index from name
def find_category(config, name):
    #Get list of categories
    category_type = config.get_simple_type("Category")
    if category_type is None or category_type["enum"] is None:
        return 0 
    
    #Search name
    i = 0
    names = category_type["enum"]
    while i < len(names) and not names[i].startswith(name):
        i = i + 1

    #Check value validity
    if len(names) <= i:
        return 0

    return i

##################################################
if __name__ == '__main__':

    CARRIER = Carrier(12, 1, 1, 'VCM', "1;2", "20;18", "4;6")
    print CARRIER
    print(CARRIER.get_modcod())
    print(CARRIER.get_ratio())

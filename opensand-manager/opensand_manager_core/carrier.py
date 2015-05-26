#!/usr/bin/env python2
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

# Author : Maxime POMPA 


"""
carrier.py - Satellite carrier handling
"""

import numpy as np

class Carrier :
    """
    Create a carrier 
        Arg :   _symbol_rate    : float
                nb_carrier     : integer (default = 1)
                category       : integer (default = 1)
                access_type    : string (default = 'CCM')
                fmt_groups     : list of string (defautl empty)
                list_modcod    : list of string (default empty)
                ratio          : dictionnary with ['modcod':ratio] only use for VCM
    """
    
    def __init__(self, symbol_rate = 0, nb_carrier = 1, category = 1, 
                 access_type = 'CCM', fmt_groups = '1', modcod = '1', 
                 ratio = '50') :
        self._symbol_rate = symbol_rate
        self._access_type = access_type
        self._str_fmt_grp = fmt_groups
        self._str_modcod = modcod
        self._str_ratio = ratio
        self._list_modcod = self.parser(modcod)
        self._fmt_groups = self.parser(fmt_groups)
        self._ratio = self.parser(ratio)
        self._nb_carrier = nb_carrier
        
        if category == "Standard":
            category = 1
        elif category == "Premium":
            category = 2
        elif category == "Pro":
            category = 3
        self._category = category
        
        self._X = []                #Position in X to trace the graphic
        self._Y = []                #Position in Y to trace the graphic
        
    ##################################################
    
    def parser(self, list_str):
        ids = []
        if type(list_str) is not list:
            ids = list_str.split(';')    
        else:
            for elm in list_str:
                for elm_id in elm.split(';'):
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
        
    def calculateXY(self, roll_off = 0, offset = 0):
        """
        Calculate all the X and Y position of the carrier to trace the graphic
        """
        half_rolloff = float(roll_off)/2
        
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
        self._X.append(symbol_rate * (1+roll_off))    
        self._Y.append(0)
        
        for i, value in enumerate(self._X):
            self._X[i] = float(self._X[i])+offset
        
    
    """MUTATEUR"""
    ##################################################

    def setSymbolRate(self, symbol_rate):
        self._symbol_rate = symbol_rate
    
    def setFmtGroups(self, fmt_groups):
        self._str_fmt_grp = fmt_groups
        self._fmt_groups = self.parser(fmt_groups)
    
    def setCategory(self, category):
        self._category = category
    
    def setAccessType(self, access_type):
        self._access_type = access_type
        
    def setModcod(self, modcod):
        self._str_modcod = modcod
        self._list_modcod = self.parser(modcod)
        
    def setRatio(self, ratio):
        self._str_ratio = ratio
        self._ratio = self.parser(ratio)
       
    def setNbCarrier(self, nb_carrier):
       self._nb_carrier = nb_carrier 
    
    """ACCESSEUR"""
    ##################################################

    def getSymbolRate(self):
        return self._symbol_rate
   
    def get_str_fmt_grp(self):
        return self._str_fmt_grp

    def get_str_modcod(self):
        return self._str_modcod

    def getModCod(self):
        return self._list_modcod

    def getCategory(self):
        return int(self._category)
        
    def get_old_category(self):    
        ret=""
        if self._category == 1:
            ret="Standard"
        elif self._category == 2:
            ret="Premium"
        elif self._category == 3:
            ret="Pro"    
        return ret

    def getAccessType(self):
        return self._access_type

    def get_old_access_type(self):
        if self._access_type == "CCM":
            return "ACM"
        else:
            return self._access_type

    def getStrRatio(self):
        return self._str_ratio

    def getRatio(self):
        return self._ratio

    def getFmtGroups(self):
        return self._fmt_groups

    def getX(self):
        return self._X
        
    def getY(self):
        return self._Y

    def getBandwidth(self, roll_off):
        """
        Return the total bandwith of th carrier
        to get only the symbol rate use getSymbolRate()
        """
        return float(self._symbol_rate) * (roll_off + 1) * self._nb_carrier
   
    def getNbCarrier(self):
        return self._nb_carrier

    ##################################################
    
    """def __str__(self):
        return "Symbol Rate : " + str(self._symbol_rate)+"\n"\
        "category : " + str(self._category)+"\n"\
        "Access Type : " + str(self._access_type)+"\n"\
        "MODCOD : " + str(self._list_modcod)+"\n"\
        "Ratio : " + str(self._ratio)+"\n" """
    
    def __str__(self):
        return "ratio=%s Rs=%g => %d carriers" % (sum(self._ratio),
                                                  self._symbol_rate,
                                                  self._nb_carrier)

##################################################
if __name__ == '__main__':

    p1 = Carrier(12, 1, 1, 'VCM',"2;5-8" , "4;6")

    print(p1.getModeCode())
    print(p1.getRatio())
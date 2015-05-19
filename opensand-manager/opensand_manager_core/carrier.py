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
        Arg :   symbol_rate    : float
                group          : integer (default = 1)
                access_type    : string (default = 'CCM')
                list_modcod    : list of string (default empty)
                ratio          : dictionnary with ['modcod':ratio] only use for VCM
    """
    
    def __init__(self, symbol_rate = 0, group = 1, 
                 access_type = 'CCM', modcod = '1', 
                 ratio = '10') :
        self._symbolRate = symbol_rate
        self._access_type= access_type
        self._str_modcod = []
        for fmt_grp in modcod:
            self._str_modcod.append(fmt_grp)
        self._list_modcod = self.parse_modcod(self._str_modcod)
        self._str_ratio = ratio
        self._ratio = self.parse_ratio(ratio)
        
        if group == "Standard":
            group = 1
        elif group == "Premium":
            group = 2
        elif group == "Pro":
            group = 3
        self._group = group
        
        self._X = []                #Position in X to trace the graphic
        self._Y = []                #Position in Y to trace the graphic
        
    ##################################################
    
    def parse_modcod(self, modcod):
        ids = []
        for mc in modcod:
            for mc_id in mc.split(';'):
                ids.append(mc_id)
        id_list = []
        for fmt_id in ids:
            if '-' in fmt_id:
                (mini, maxi) = fmt_id.split('-')
                id_list.extend(range(int(mini), int(maxi) + 1))
            else:
                id_list.append(int(fmt_id))

        return id_list
            
            
    def parse_ratio(self, ratio):
        ids = ratio.split(';')
        id_list = []
        for rat_id in ids:
            id_list.append(float(rat_id))

        return id_list
    ##################################################
        
    def calculateXY(self, roll_off = 0, offset = 0):
        """
        Calculate all the X and Y position of the carrier to trace the graphic
        """
        half_rolloff = float(roll_off)/2
        
        self._X = []
        self._Y = []
        
        """
        We do not use directly self._X to stock the value of linspace 
        because it will create a numpy.ndarray.
        """
        for value in np.linspace(0,1,100):
            self._X.append(value)
            self._Y.append(np.sin(np.pi*value))
        for i, value in enumerate(self._X):
            self._X[i] = (float(self._X[i])*float(self._symbolRate) + float(self._symbolRate)*half_rolloff)
        
        self._X.insert(0, 0)    
        self._Y.insert(0, 0)
        self._X.append(float(self._symbolRate)*(1+roll_off))    
        self._Y.append(0)
        
        for i, value in enumerate(self._X):
            self._X[i] = float(self._X[i])+offset
        
    
    """MUTATEUR"""
    ##################################################

    def setSymbolRate(self, symbol_rate):
        self._symbolRate = symbol_rate
    
    def setGroup(self, group):
        self._group = group
    
    def setAccessType(self, access_type):
        self._access_type = access_type
        
    def setModcod(self, modcod):
        self._str_modcod = modcod
        self._list_modcod = self.parse_modcod(modcod)
        
    def setRatio(self, ratio):
        self._str_ratio = ratio
        self._ratio = self.parse_ratio(ratio)
        
    
    """ACCESSEUR"""
    ##################################################

    def getSymbolRate(self):
        return self._symbolRate
    
    def get_str_modcod(self):
        return self._str_modcod

    def getModeCode(self):
        return self._list_modcod

    def getGroup(self):
        return int(self._group)
        
    def get_old_group(self):    
        ret=""
        if self._group == 1:
            ret="Standard"
        elif self._group == 2:
            ret="Premium"
        elif self._group == 3:
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

    def getX(self):
        return self._X
        
    def getY(self):
        return self._Y

    def getBandwidth(self, roll_off):
        """
        Return the total bandwith of th carrier
        to get only the symbol rate use getSymbolRate()
        """
        return float(self._symbolRate)*(roll_off+1)
    
    ##################################################
    
    def __str__(self):
        return "Symbol Rate : " + str(self._symbolRate)+"\n"\
        "Group : " + str(self._group)+"\n"\
        "Access Type : " + str(self._access_type)+"\n"\
        "MODCOD : " + str(self._str_modcod)+"\n"\
        "Ratio : " + str(self._ratio)+"\n"
    
    
##################################################
if __name__ == '__main__':

    p1 = Carrier(12, 1, 'VCM',"2;5-8" , "4;6")

    print(p1.getModeCode())
    print(p1.getRatio())

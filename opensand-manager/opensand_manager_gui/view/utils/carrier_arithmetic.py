#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
# Copyright © 2019 CNES
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
carrier_arithmetic.py - the configuration tab view
"""


from opensand_manager_core.utils import BANDWIDTH
from opensand_manager_core.carriers_band import CarriersBand

from opensand_manager_core.utils import DAMA, SCPC, ALOHA, VCM, ACM, CCM


class CarrierArithmetic:
    """ Element for the resouces configuration tab """

    def __init__(self, list_carrier, model, link):
        self._list_carrier = list_carrier
        self._roll_off = 0
        self._bandwidth = 0
        self._min_bitrate = 0
        self._max_bitrate = 0
        self._model = model
        self._link = link

        
    def update_graph(self, ax, roll_off):
        """Update ax graph of the carrier representation"""
        
        #get all carriers
        color = {0: 'b', 
                 1: 'g', 
                 2: 'c', 
                 3: 'm', 
                 4: 'y', 
                 5: 'k', 
                 6: 'r',}
        access_type = {ACM: '-',
                       CCM: '-.',
                       VCM: '--',
                       SCPC: ':',
                       ALOHA: '--',
                       DAMA: '-',}
                
        self._bandwidth = 0
        #Trace the graphe
        for carrier in self._list_carrier:
            for nb_carrier in range(1, carrier.get_nb_carriers() + 1):
                carrier.calculate_xy(roll_off, self._bandwidth)
                ax.plot(carrier.get_x(), 
                        carrier.get_y(), 
                        color[carrier.get_category()] +
                        access_type[carrier.get_access_type()])
                # bandwidth in MHz
                self._bandwidth = self._bandwidth + carrier.get_bandwidth(roll_off) \
                        / (1E6 * carrier.get_nb_carriers())
        if self._bandwidth != 0:
            ax.axis([float(-self._bandwidth)/6, 
                     self._bandwidth + float(self._bandwidth)/6,
                     0, 1.5])
        bp, = ax.plot([0, 0, self._bandwidth, self._bandwidth], 
                      [0, 1, 1, 0], 'r-', 
                      label = BANDWIDTH, 
                      linewidth = 3.0)
        ax.legend([bp], [BANDWIDTH])
        ax.grid(True)

    def update_rates(self, fmt_group):
        """Update Rate"""
        config = self._model.get_conf().get_configuration()
        ret_lnk_std = self._model.get_conf().get_return_link_standard()
        carriers_band = CarriersBand() 
        carriers_band.modcod_def(self._model.get_scenario(), 
                                 config, ret_lnk_std, False)
        
        for carrier in self._list_carrier:
            carriers_band.add_carrier(carrier, carrier.get_old_category(config))

        for grp_id in fmt_group:
            carriers_band.add_fmt_group(grp_id,
                                        fmt_group[grp_id])

        for element in self._list_carrier:
            element.set_rates(carriers_band.get_carrier_bitrates(element))
        self._min_bitrate = \
                carriers_band.get_min_bitrate(element.get_old_category(config), 
                                                          element.get_access_type())
        self._max_bitrate = \
                carriers_band.get_max_bitrate(element.get_old_category(config),
                                                           element.get_access_type())

    
    def set_roll_off(self, roll_off):
        self._roll_off = roll_off

    def get_list_carrier(self):
        return self._list_carrier

    def get_bandwidth(self):
        return self._bandwidth

    def get_max_bitrate(self):
        return self._max_bitrate

    def get_min_bitrate(self):
        return self._min_bitrate


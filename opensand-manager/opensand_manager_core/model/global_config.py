#!/usr/bin/env python2
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
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>
# Author: Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>

"""
global_configuration.py - the global configuration description
"""

import os
import shutil
import operator

from opensand_manager_core.utils import OPENSAND_PATH, \
                                        SPOT, ID, GW, \
                                        SAT, DAMA, \
                                        DVB_S2, DVB_RCS, DVB_RCS2
from opensand_manager_core.model.host_advanced import AdvancedHostModel
from opensand_manager_core.model.files import Files
from opensand_manager_core.my_exceptions import XmlException, ModelException
from opensand_manager_core.opensand_xml_parser import XmlParser

DEFAULT_CONF = OPENSAND_PATH + "core_global.conf"
GLOBAL_XSD = OPENSAND_PATH + "core_global.xsd"
CONF_NAME = "core_global.conf"

MODCOD_DEF_S2="modcod_def_s2"
MODCOD_DEF_RCS="modcod_def_rcs"
MODCOD_DEF_RCS2="modcod_def_rcs2"

class GlobalConfig(AdvancedHostModel):
    """ Global OpenSAND configuration, displayed in configuration tab """
    def __init__(self, scenario):
        AdvancedHostModel.__init__(self, 'global', scenario)
        self._payload_type = ''
        self._delay_type = ''
        self._emission_std = ''
        #self._dama = ''
        self._forward_down = {}
        self._return_up = {}
        self._modcods = None

    def load(self, scenario):
        """ load the global configuration """
        # create the host configuration directory
        conf_path = scenario
        self._scenario = scenario
        if not os.path.isdir(conf_path):
            try:
                os.makedirs(conf_path, 0755)
            except OSError, (_, strerror):
                raise ModelException("failed to create directory '%s': %s" %
                                     (conf_path, strerror))

        self._conf_file = os.path.join(conf_path, CONF_NAME)
        # copy the configuration template in the destination directory
        # if it does not exist
        if not os.path.exists(self._conf_file):
            try:
                shutil.copy(DEFAULT_CONF, self._conf_file)
            except IOError, msg:
                raise ModelException("failed to copy %s configuration file in "
                                     "'%s': %s" % (self._name, self._conf_file,
                                                   msg))

        self._xsd = GLOBAL_XSD

        try:
            self._configuration = XmlParser(self._conf_file, self._xsd)
            if self._files is None:
                # load the files
                self._files = Files(self._name, self._configuration, scenario)
            else:
                self._files.load(scenario, self._configuration)
        except IOError, msg:
            raise ModelException("cannot load configuration: %s" % msg)
        except XmlException, msg:
            raise ModelException("failed to parse configuration: %s"
                                 % msg)

    def _load_all_modcods(self):
        '''
        Load modcods from files
        '''
        # Load modcods and default modcods
        self._modcods = {}
        self._modcods[DVB_S2] = load_modcods(self.get_param(MODCOD_DEF_S2))
        self._modcods[DVB_RCS] = load_modcods(self.get_param(MODCOD_DEF_RCS))
        self._modcods[DVB_RCS2] = load_modcods(self.get_param(MODCOD_DEF_RCS2), True)

    def cancel(self):
        self.load(self._scenario)

    def save(self):
        """ save the configuration """
        try:
            self._configuration.set_value(self._payload_type,
                                          "//satellite_type")
            #self._configuration.set_value(self._dama, "//dama_algorithm")
            self.set_stack('forward_down_encap_schemes',
                           self._forward_down, 'encap')
            self.set_stack('return_up_encap_schemes',
                           self._return_up, 'encap')
            self._configuration.write()
        except XmlException:
            raise

    def new_host(self, name, instance):
        """ handle a new host """
        # nothing to do if SAT
        if name.startswith(SAT):
            return
        self._configuration.add_host(instance)
        try:
            self._configuration.write()
            self._files.load(self._scenario, self._configuration)
        except XmlException:
            raise

    def remove_host(self, name, instance):
        """ remove a host """
        # nothing to do if SAT
        if name.startswith(SAT):
            return
        self._configuration.remove_host(instance)
        try:
            self._configuration.write()
            self._files.load(self._scenario, self._configuration)
        except XmlException:
            raise

    def new_gw(self, name, instance, net_config):
        """ handle a new gateway """
        exist =  False
        for section in self._configuration.get_sections():
            for child in section.iterchildren():
                if (child.tag == GW and child.get(ID) == instance) or \
                   (child.tag == SPOT and child.get(GW) == instance):
                    exist =  True
                    continue

            if not exist:
                self._configuration.add_gw("//"+section.tag, instance)
        # update the configuration elements
        self.update_conf()

        try:
            self._configuration.write()
            self._files.load(self._scenario, self._configuration)
        except XmlException:
            raise

    def remove_gw(self, name, instance):
        """ remove a gw """
        for section in self._configuration.get_sections():
            for child in section.getchildren():
                if child.tag ==  SPOT or child.tag == GW:
                    self._configuration.remove_gw("//"+section.tag, instance)
                    break

        try:
           self._configuration.write()
           self._files.load(self._scenario, self._configuration)
        except XmlException:
            raise


    def set_payload_type(self, val):
        """ set the payload_type value """
        self._payload_type = val

    def get_payload_type(self):
        """ get the payload_type value """
        return self.get_param('satellite_type')

    def get_return_link_standard(self):
        """ get the return_link_std value """
        return self.get_param('return_link_standard')

    def get_rcs2_burst_length(self):
        """ get the rcs2_burst_length value """
        return self.get_param('rcs2_burst_length')

    def set_delay_type(self, val):
        """ set the delay_type value """
        self._delay_type = val

    def get_delay_type(self):
        """ get the delay_type value """
        return self.get_param('delay')

    def set_emission_std(self, val):
        """ set the payload_type value """
        self._emission_std = val

    def get_emission_std(self):
        """ get the payload_type value """
#        if self.get_dama() != 'MF2-TDMA':
#           return "DVB-RCS"
#       else:
#           return "DVB-S2"
        return "DVB-RCS"

#    def set_dama(self, val):
#        """ set the dama value """
#        self._dama = val
#
#    def get_dama(self):
#        """ get the dama value """
#        return self.get_param("dama_algorithm")

    def set_return_up_encap(self, stack):
        """ set the return_up_encap_schemes values """
        self._return_up = stack

    def get_return_up_encap(self):
        """ get the return_up_encap_schemes values """
        encap = self.get_stack("return_up_encap_schemes", 'encap')
        return encap

    def set_forward_down_encap(self, stack):
        """ set the forward_down_encap_schemes values """
        self._forward_down = stack

    def get_forward_down_encap(self):
        """ get the forward_down_encap_schemes values """
        encap = self.get_stack("forward_down_encap_schemes", 'encap')
        return encap

    def get_global_delay(self):
        """ get the use constant_global_delay value """
        return self.get_param("common/global_constant_delay")

    def get_param(self, name):
        """ get a parameter in the XML configuration file """
        val = None
        key = self._configuration.get("//" + name)
        if key is not None:
            try:
                val = self._configuration.get_value(key)
            except:
                pass
        return val

    def get_modcods(self, link_std, burst_len=None):
        '''
        Get modcods of a link standard.

        Args:
            link_std:  DVB_S2, DVB_RCS or DVB_RCS2

        Returns:
            modcods
        '''
        if self._modcods is None:
            self._load_all_modcods()

        if burst_len is not None:
            return filter_modcods(self._modcods[link_std], burst_len)

        return self._modcods[link_std]

    def get_return_link_default_modcods(self):
        '''
        Get the default MODCODs for DVB-RCS2
        '''
        if self.get_return_link_standard() == DVB_RCS:
            return [ get_higher_modcod(self.get_modcods(DVB_RCS)) ]
        elif self.get_return_link_standard() == DVB_RCS2:
            return filter_modcods(self.get_modcods(DVB_RCS2,
                                  self.get_rcs2_burst_length()))
        return None

def load_modcods(path, has_burst_length=False):
    '''
    Get the modcods list from file

    Args:
        path:          the modcods path
        burst_length:  the burst length
                       (only if return link standard is DVB-RCS2)

    Returns:
        the modcods list
    '''
    modcods = []
    with open(path, 'r') as fd:
        for line in fd:
            if (line.startswith("/*") or
                line.isspace() or
                line.startswith('nb_fmt')):
                continue
            elts = line.split()
            if not elts[0].isdigit:
                continue
            if not has_burst_length and len(elts) != 5:
                continue
            elif has_burst_length and len(elts) != 6:
                continue
            # id, modulation, coding_rate, spectral_efficiency, required Es/N0
            # [, burst_length]
            modcods.append(elts)
    return modcods

def filter_modcods(modcods, burst_length=None):
    '''
    Get the modcods with specific burst length

    Args:
        modcods:       the modcods list
        burst_length:  the burst length

    Returns:
        list of modcods with specified burst length
    '''
    if burst_length is None:
        return modcods

    return [
            modcod for modcod in modcods
            if modcod[5] == burst_length
    ]

def get_higher_modcod(modcods):
    '''
    Get the higher modcod from a list

    Args:
        modcods:  the modcods list

    Returns:
        the higher modcod of the list
    '''
    if not modcods:
        return None

    return max(modcods, key=operator.itemgetter(0))

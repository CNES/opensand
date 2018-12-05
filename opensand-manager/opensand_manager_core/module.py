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
# Author: Aurelien DELRIEU <adelrieu@toulouse.viveris.com>


"""
encap_module.py - Encapsulation module for OpenSAND Manager
"""

import os
import shutil
from collections import namedtuple

from opensand_manager_core.utils import GW, SAT, ST, GLOBAL, GW_types
from opensand_manager_core.model.files import Files
from opensand_manager_core.opensand_model import OPENSAND_PATH
from opensand_manager_core.my_exceptions import ModelException, XmlException
from opensand_manager_core.opensand_xml_parser import XmlParser

modules = []

class MetaModule(type):
    """ meta class used to easily load the modules in Manager """
    def __init__(metacls, name, bases, dct):
        """ auto-register """
        super(MetaModule, metacls).__init__(name, bases, dct)
        base_dct = bases[0].__dict__
        module_name = dct['_name']
        if not "_type" in dct:
            module_type = base_dct['_type']
        else:
            module_type = dct['_type']
        if module_name is not None and module_type is not None:
            modules.append(metacls)

class OpenSandModule(object):
    """ the physical layer modules for OpenSAND Manager """
    # The custom metaclass
    __metaclass__ = MetaModule
    _name = None
    _type = None

    def __init__(self):
        self._xml = None
        self._xsd = None
        self._description = ''
        # a list of host on which the configuration should be edited or
        # ['global'] is the configuration is the same for all
        self._targets = None

        # should not be modified
        self._parser = None
        self._config = None
        self._files = None


    def get_xml(self):
        """ get the configuration file name
            should return None if there is no configuration """
        return self._xml

    def get_xsd(self):
        """ get the XSD file name
            should return None if there is no configuration """
        return self._xsd

    def get_name(self):
        """ get the  module name """
        return self._name

    def get_type(self):
        """ get the module type """
        return self._type

    def set_conf_view(self, notebook):
        """ set the configuration notebook """
        self._config = notebook

    def get_conf_view(self):
        """ get the configuration notebook """
        return self._config

    def get_config_parser(self):
        """ get the XML configuration parser """
        return self._parser

    def get_description(self):
        """ get the module description """
        return self._description

    def get_targets(self):
        """ get the hosts on which the configuration should be edited """
        return self._targets

    def update(self, scenario, component, host_name=None):
        """ reload the module configuration """
        if self._xml is None:
            return

        if component in GW_types:
            component = GW

        if not component in self._targets:
            raise ModelException("component is not in %s plugin targets" %
                                 (self._name))

        if component != GLOBAL:
            default_host_path = os.path.join(OPENSAND_PATH,
                                             component)
            host_path = os.path.join(scenario, host_name)
        else:
            default_host_path = OPENSAND_PATH 
            host_path = scenario

        default_plugin_path = os.path.join(default_host_path, 'plugins')
        plugins_path = os.path.join(host_path, 'plugins')
        xml_path = os.path.join(plugins_path, self._xml)
        xsd_path = os.path.join(default_plugin_path, self._xsd)
        # create the plugins path if necessary
        if not os.path.exists(plugins_path):
            try:
                os.makedirs(plugins_path, 0755)
            except OSError, (_, strerror):
                raise ModelException("cannot create directory '%s': %s"
                                     % (plugins_path, strerror))
        # create the configuration file if necessary
        if not os.path.exists(xml_path):
            try:
                default_path = os.path.join(default_plugin_path, self._xml)
                shutil.copy(default_path, xml_path)
            except IOError, (_, strerror):
                raise ModelException("cannot copy %s plugin configuration from "
                                     "'%s' to '%s': %s" % (self._name,
                                                           default_path,
                                                           xml_path, strerror))

        try:
            self._config = None
            self._parser = XmlParser(xml_path, xsd_path)
            name = host_name
            if name is None:
                name = component
            if self._files is None:
                self._files = Files(name, self._parser, scenario)
            else:
                self._files.load(scenario, self._parser)
        except IOError, msg:
            raise ModelException("cannot load module %s configuration: \n\t%s" %
                                 (self._name, msg))
        except XmlException, msg:
            raise ModelException("failed to parse module %s configuration file:"
                                 "\n\t%s" % (self._name, msg))


    def get_files(self):
        """ get the files """
        return self._files

    def save(self):
        """ save the module """
        notebook = self.get_conf_view()
        if notebook is not None:
            notebook.save()

### Encapsulation ###

EncapModuleContext = namedtuple("EncapModuleContext", 
                                "satellite_type "
                                "return_link_standard "
                                "link")
# satellite_type: type of satellite (transparent or regenerative)
# return_link_standard: standard of the return link (dvb-rcs or dvb-rcs2)
# link: link to apply the configuration (forward or return link)

EncapModuleConfig = namedtuple("EncapModuleConfig",
                               "mandatory_down "
                               "handle_upper_block "
                               "allowed_upper_blocks")
# mandatory_down: requires a downer encapsulation
# handle_upper_block: is the upper encapsulation
# allowed_upper_blocks: list of the allowed upper encapsulation

class EncapModule(OpenSandModule):
    """ the encapsulation module for OpenSAND Manager """
    _name = None
    _type = 'encap'

    TRANSPARENT = "transparent"
    REGENERATIVE = "regenerative"

    DVB_RCS = "dvb-rcs"
    DVB_RCS2 = "dvb-rcs2"

    FORWARD_LINK = "dvb-s2"
    RETURN_LINK = "dvb-rcs"

    def __init__(self):
        super(EncapModule, self).__init__()
        self._encap_config = {}
        self._targets = [GLOBAL]

    def _add_config(self, satellite_type, return_link_std, link, 
                    mandatory_down, handle_upper_block, allowed_upper_blocks = []):
        key = EncapModuleContext(satellite_type, return_link_std, link)
        val = EncapModuleConfig(mandatory_down, handle_upper_block,
                                allowed_upper_blocks)
        self._encap_config[key] = val

    def get_config(self, satellite_type, return_link_std, link):
        key = EncapModuleContext(satellite_type, return_link_std, link)
        return self._encap_config.get(key)
    
### Lan Adaptation ###

class LanAdaptationModule(OpenSandModule):
    """ the lan adaptation module for OpenSAND Manager """
    _name = None
    _type = 'lan_adaptation'

    def __init__(self):
        OpenSandModule.__init__(self)
        self._upper = []
        self._handle_upper_bloc = False
        self._iface_type = None

        self._condition = {
                            # is the module a header suppression or compression
                            # tool
                            'header_modif': False,
                          }

        self._targets = {ST, GW} | GW_types 

    def get_available_upper_protocols(self, unused=None):
        """ get the protocols it can encapsulate """
        return self._upper

    def get_condition(self, condition):
        """ get a specific condition """
        if not condition in self._condition:
            return None
        else:
            return self._condition[condition]

    def handle_upper_bloc(self):
        """ check if the module can handle a packet from upper bloc """
        return self._handle_upper_bloc

    def get_interface_type(self):
        """ get the interface type """
        return self._iface_type

### Physical Layer ###

class AttenuationModule(OpenSandModule):
    """ the attenuation models module for OpenSAND Manager """
    _name = None
    _type = 'attenuation'

    def __init__(self):
        OpenSandModule.__init__(self)
        self._targets = {GW, ST} | GW_types

class MinimalModule(OpenSandModule):
    """ the minimal conditions module for OpenSAND Manager """
    _name = None
    _type = 'minimal'

    def __init__(self):
        OpenSandModule.__init__(self)
        self._targets = {SAT, GW, ST} | GW_types
        
class ErrorModule(OpenSandModule):
    """ the error inesrtions module for OpenSAND Manager """
    _name = None
    _type = 'error'

    def __init__(self):
        OpenSandModule.__init__(self)
        self._targets = {SAT, GW, ST} | GW_types

### Satellite delay module ###

class SatDelayModule(OpenSandModule):
    """ the satellite delay module for OpenSAND Manager """
    _name = None
    _type = 'satdelay'

    def __init__(self):
        OpenSandModule.__init__(self)
        self._targets = {GW, ST} | GW_types

from opensand_manager_core.modules import *

def load_modules(component):
    """ load all modules """
    loaded = []
    for metacls in modules:
        module = metacls()
        if component in module.get_targets():
            loaded.append(module)
    return loaded



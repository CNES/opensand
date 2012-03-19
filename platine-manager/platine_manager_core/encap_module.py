#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
encap_module.py - Encapsulation module for Platine Manager
"""

import types

encap_methods = {}


class MetaEncap(type):
    """ meta class used to easily load the modules in Manager """
    def __init__(metacls, name, bases, dct):
        """ auto-register device """
        super(MetaEncap, metacls).__init__(name, bases, dct)
        encap_name = dct['_name']
        if encap_name is not None:
            encap_methods[encap_name] = metacls

class EncapModule(object):
    """ the encapsulation module for Platine Manager """

    # The custom metaclass
    __metaclass__ = MetaEncap
    _name = None

    def __init__(self):
        self._upper = {
                       'transparent':(),
                       'regenerative':()
                      }

        self._xml = '%s.conf' % self._name.lower()
        self._xsd = '%s.xsd' % self._name.lower()
        self._description = ''
        self._condition = {
                            # down encap scheme is mandatory
                            'mandatory_down': False,
                            # DVB-RCS is supported as lower layer
                            'dvb-rcs': True,
                            # DVB-S2 is supported as lower layer
                            'dvb-s2': True,
                            # is the module an IP option
                            'ip_option': False,
                          }

    # should not be modified
        self._parser = None
        self._config = None

    def get_xml(self):
        """ get the configuration file name
            should return None if there is no configuration """
        return self._xml

    def get_xsd(self):
        """ get the XSD file name
            should return None if there is no configuration """
        return self._xsd

    def get_available_upper_protocols(self, satellite_type):
        """ get the protocols it can encapsulate """
        return self._upper[satellite_type]

    def get_name(self):
        """ get the encapsulation protocol """
        return self._name

    def set_conf_view(self, notebook):
        """ set the configuration notebook """
        self._config = notebook

    def get_conf_view(self):
        """ get the configuration notebook """
        return self._config

    def set_config_parser(self, parser):
        """ set the XML configuration parser """
        self._config = None
        self._parser = parser

    def get_config_parser(self):
        """ get the XML configuration parser """
        return self._parser

    def get_description(self):
        """ get the module description """
        return self._description

    def get_condition(self, condition):
        """ get a specific condition """
        if not condition in self._condition:
            return None
        else:
            return self._condition[condition]


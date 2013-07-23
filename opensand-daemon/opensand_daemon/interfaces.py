#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2013 TAS
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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
interfaces.py - The OpenSAND interfaces management
"""

import netifaces
import logging

from ConfigParser import Error
from ipaddr import IPNetwork
import socket
import fcntl
import struct

from opensand_daemon.nl_utils import NlInterfaces, NlError, NlExists, NlRoute

#macros
LOGGER = logging.getLogger('sand-daemon')

TUN_NAME = "opensand_tun"
BR_NAME = "opensand_br"

# TODO DHCP and sysctl instead of script ?

def get_mac_address(interface):
    """ get the MAC address of an interface"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(sock.fileno(), 0x8927,  struct.pack('256s',
                      interface[:15]))
    return ''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1]



class OpenSandInterfaces(object):
    """ the OpenSAND interfaces object """
    def __init__(self, conf, name):
        self._name = name
        self._type = conf.get('network', 'config_level').lower()
        if self._type != 'advanced' and self._type != 'automatic':
            raise Error("unknown network configuration type %s "
                        "(choose between advanced or automatic)" %
                        self._type)
        self._emu_iface = ''
        self._emu_ipv4 = None
        self._lan_iface = ''
        self._lan_ipv4 = None
        self._lan_ipv6 = None
        self._ifaces = NlInterfaces()
        self._nladd = {}
        self._ifdown = []
        if name != 'ws':
            self.init_emu(conf)
        if name != 'sat':
            self.init_lan(conf)

        if name != 'ws' and name != 'sat':
            self.init_tun_tap()

        if name != 'ws':
            self.check_sysctl()

    def init_emu(self, conf):
        """ init the emulation interface """
        self._emu_iface = conf.get('network', 'emu_iface')
        if self._type == 'advanced':
            self._emu_ipv4 = IPNetwork(conf.get('network', 'emu_ipv4'))
            try:
                # if the address exists this is not an error, do not keep it in
                # self._nladd to avoid removing it at exit
                try:
                    self._ifaces.add_address(str(self._emu_ipv4), self._emu_iface)
                except NlExists:
                    LOGGER.info("address %s already exists on %s" %
                                (self._emu_ipv4, self._emu_iface))
                finally:
                    self._nladd[str(self._emu_ipv4)] = self._emu_iface
                # if interface is already up this is not an error, do not keep
                # it in self._ifdown to avoid changing its state at exit
                try:
                    self._ifaces.up(self._emu_iface)
                except NlExists:
                    LOGGER.debug("interface %s is already up" % self._emu_iface)
                finally:
                    self._ifdown.append(self._emu_iface)
            except NlError, msg:
                LOGGER.error("unable to set addresses: %s" % msg)
                # delete the address that were already set and set adequate
                # interface down
                self.release()
            except BaseException, msg:
                LOGGER.error("error when changing interface %s" %
                             self._emu_iface)
        else:
            try:
                # get the interface addresses
                addresses = netifaces.ifaddresses(self._emu_iface)
                # retrieve IPv4 and IPv6 addresses
                emu_ipv4 = addresses[netifaces.AF_INET]
                if len(emu_ipv4) > 1:
                    LOGGER.warning("more than one IPv4 addresses for emulation "
                                   "interface (%s), pick the first one: %s" %
                                   (self._emu_iface, emu_ipv4[0]['addr']))
                self._emu_ipv4 = IPNetwork("%s/%s" % (emu_ipv4[0]['addr'],
                                                      emu_ipv4[0]['netmask']))
            except ValueError, msg:
                LOGGER.error("cannot get emulation interfaces addresses: %s" % msg)
                raise
            except KeyError, val:
                LOGGER.error("an address with family %s is not set for emulation "
                             "interface" % netifaces.address_families[val])
                raise

    def init_lan(self, conf):
        """ init the lan interface """
        self._lan_iface = conf.get('network', 'lan_iface')
        if self._type == 'advanced':
            self._lan_ipv4 = IPNetwork(conf.get('network', 'lan_ipv4'))
            self._lan_ipv6 = IPNetwork(conf.get('network', 'lan_ipv6'))
            try:
                # if the addressexists this is not an error, do not keep it in
                # self._nladd to avoid removing it at exit
                try:
                    self._ifaces.add_address(str(self._lan_ipv4), self._lan_iface)
                except NlExists:
                    LOGGER.info("address %s already exists on %s" %
                                (self._lan_ipv4, self._lan_iface))
                finally:
                    self._nladd[str(self._lan_ipv4)] = self._lan_iface
                try:
                    self._ifaces.add_address(str(self._lan_ipv6), self._lan_iface)
                except NlExists:
                    LOGGER.info("address %s already exists on %s" %
                                (self._lan_ipv6, self._lan_iface))
                finally:
                    self._nladd[self._lan_ipv6] = self._lan_iface
                # if interface is already up this is not an error, do not keep
                # it in self._ifdown to avoid changing its state at exit
                try:
                    self._ifaces.up(self._lan_iface)
                except NlExists:
                    LOGGER.debug("interface %s is already up" % self._lan_iface)
                finally:
                    self._ifdown.append(self._lan_iface)
            except NlError, msg:
                LOGGER.error("unable to set addresses: %s" % msg)
                # delete the address that were already set
                self.release()
                raise NlError(msg)
            except BaseException, msg:
                LOGGER.error("error when changing interface %s: %s" %
                             (self._lan_iface, msg))
                raise
        else:
            try:
                # get the interface addresses
                addresses = netifaces.ifaddresses(self._lan_iface)
                # retrieve IPv4 and IPv6 addresses
                lan_ipv4 = addresses[netifaces.AF_INET]
                if len(lan_ipv4) > 1:
                    LOGGER.warning("more than one IPv4 addresses for lan "
                                   "interface (%s), pick the first one: %s" %
                                   (self._lan_iface, lan_ipv4[0]['addr']))
                self._lan_ipv4 = IPNetwork("%s/%s" % (lan_ipv4[0]['addr'],
                                                      lan_ipv4[0]['netmask']))
                lan_ipv6 = addresses[netifaces.AF_INET6]
                # remove IPv6 link addresses
                copy = list(lan_ipv6)
                for ip in copy:
                    addr = ip['addr']
                    if addr.endswith('%' + self._lan_iface):
                        del lan_ipv6[ip]
                if len(lan_ipv6) > 1:
                    LOGGER.warning("more than one IPv6 addresses for lan "
                                   "interface (%s), pick the first one: %s" %
                                   (self._lan_iface, lan_ipv6[0]['addr']))
                self._lan_ipv6 = IPNetwork("%s/%s" % (lan_ipv6[0]['addr'],
                                                      lan_ipv6[0]['netmask']))
            except ValueError, msg:
                LOGGER.error("cannot get emulation interfaces addresses: %s" % msg)
                raise
            except KeyError, val:
                LOGGER.error("an address with family %s is not set for emulation "
                             "interface" % netifaces.address_families[val])
                raise

    def init_tun_tap(self):
        """ init the TUN/TAP interfaces """
        # only set the correct address on TUN and bridge, the core will

        # compute the TUN and bridge addresses
        mask = self._lan_ipv4.prefixlen
        # we do as if mask = 24 for smaller masks
        if mask < 24:
            modulo = 251
        # we need a mask smaller than 30 to get enough IP addresses
        elif mask > 28:
            # TODO
            raise NlError
        else:
            # - 5 to avoid 255 as add
            modulo = pow(2, 32 - mask) - 5
        # IPv4 address = net.add
        net, add = str(self._lan_ipv4.ip).rsplit('.', 1)
        # tun is 2 more than interface
        # bridge is 3 more than interface
        # for add we add 1 after modulo to avoid 0
        tun_ipv4 = IPNetwork("%s.%s/%s" % (net,
                                           str(((int(add) + 1) % modulo) + 1),
                                           mask))
        br_ipv4 = IPNetwork("%s.%s/%s" % (net,
                                          str(((int(add) + 2) % modulo) + 1),
                                          mask))

         # compute the TUN and bridge addresses
        mask = self._lan_ipv6.prefixlen
        # we do as if mask = 24 for smaller masks
        if mask < 112:
            modulo = 0xFFFB
        # we need a mask smaller than 30 to get enough IP addresses
        elif mask > 124:
            # TODO
            raise NlError
        else:
            # - 5 to avoid 255 as add
            modulo = pow(2, 128 - mask) - 5
        # IPv6 address = net.add
        net, add = str(self._lan_ipv6.ip).rsplit(':', 1)
        # tun is 2 more than interface
        # bridge is 3 more than interface
        # for add we add 1 after modulo to avoid 0
        tun_ipv6 = IPNetwork("%s:%x/%s" % (net,
                                           ((int(add, 16) + 1) % modulo) + 1, mask))
        br_ipv6 = IPNetwork("%s:%x/%s" % (net,
                                          ((int(add, 16) + 2) % modulo) + 1, mask))

        # the interfaces are used by opensand so we don't care if address is
        # already set or not
        try:
            try:
                self._ifaces.add_address(str(tun_ipv4), TUN_NAME)
            except NlExists:
                pass
            try:
                self._ifaces.add_address(str(tun_ipv6), TUN_NAME)
            except NlExists:
                pass
            try:
                self._ifaces.add_address(str(br_ipv4), BR_NAME)
            except NlExists:
                pass
            try:
                self._ifaces.add_address(str(br_ipv6), BR_NAME)
            except NlExists:
                pass
        except NlError:
            LOGGER.error("error when configuring TUN and Bridge")

        # delete default routes on tun and bridge
        # (these are routes with kernel protocol)
        tun_route = NlRoute(TUN_NAME)
        br_route = NlRoute(BR_NAME)
        try:
            route = "%s/%s" % (tun_ipv4.network, tun_ipv4.prefixlen)
            tun_route.delete(route, None, True)
        except NlError, msg:
            LOGGER.warning("unable to delete IPv4 default route %s for %s: %s" %
                           (route, TUN_NAME, msg))
        try:
            route = "%s/%s" % (tun_ipv6.network, tun_ipv6.prefixlen)
            tun_route.delete(route, None, True)
        except NlError, msg:
            LOGGER.warning("unable to delete IPv6 default route %s for %s: %s" %
                           (route, TUN_NAME, msg))
        try:
            route = "%s/%s" % (br_ipv4.network, br_ipv4.prefixlen)
            br_route.delete(route, None, True)
        except NlError, msg:
            LOGGER.warning("unable to delete IPv4 default route %s for %s: %s" %
                           (route, BR_NAME, msg))
        try:
            route = "%s/%s" % (br_ipv6.network, br_ipv6.prefixlen)
            br_route.delete(route, None, True)
        except NlError, msg:
            LOGGER.warning("unable to delete IPv6 default route %s for %s: %s" %
                           (route, BR_NAME, msg))

    def check_sysctl(self):
        """ check sysctl values and log if the value may lead to errors """
        if self._name != 'sat':
            for iface in [self._emu_iface, self._lan_iface]:
                with open("/proc/sys/net/ipv4/conf/%s/forwarding" % iface, 'ro') \
                     as sysctl:
                    if sysctl.read().rstrip('\n') != "1":
                        LOGGER.warning("IPv4 forwarding on interface %s is "
                                       "disabled, you won't be able to route "
                                       "packets toward WS behind this host" %
                                       iface)
            for iface in [self._lan_iface]:
                with open("/proc/sys/net/ipv6/conf/%s/forwarding" % iface, 'ro') \
                     as sysctl:
                    if sysctl.read().rstrip('\n') != "1":
                        LOGGER.warning("IPv6 forwarding on interface %s is "
                                       "disabled, you won't be able to route "
                                       "packets toward WS behind this host" %
                                       iface)
            with open("/proc/sys/net/ipv4/ip_forward", 'ro') as sysctl:
                if sysctl.read().rstrip('\n') != "1":
                    LOGGER.warning("IPv4 ip_forward is disabled, you should enable "
                                   "it")
        for val in ["wmem_max", "rmem_max", "wmem_default", "rmem_default"]:
            with open("/proc/sys/net/core/%s" % val, 'ro') as sysctl:
                if int(sysctl.read().rstrip('\n')) < 1048580:
                    LOGGER.warning("%s should be set to a value greater than "
                                   "1048580 in order to support high bitrates "
                                   "through OpenSAND" % val)

    def get_descr(self):
        """ get the addresses elements """
        descr = {}
        mac = ''
        # for ST and GW sometimes bridge takes address of lan interface
        # once it is added to it so we need to know both MAC addresses
        if self._name != 'ws':
            descr.update({'emu_iface': self._emu_iface,
                          'emu_ipv4': str(self._emu_ipv4),
                         })
            if self._name != 'sat':
                mac = get_mac_address(BR_NAME)
        if self._name != 'sat':
            mac = mac + " " + get_mac_address(self._lan_iface)
        if self._name != 'sat':
            descr.update({'lan_iface': self._lan_iface,
                          'lan_ipv4': self._lan_ipv4,
                          'lan_ipv6': self._lan_ipv6,
                          'mac': mac,
                         })
        return descr

    def release(self):
        """ remove interfaces """
        for add in self._nladd:
            try:
                self._ifaces.del_address(add, self._nladd[add])
            except:
                continue
        for iface in self._ifdown:
            try:
                self._ifaces.down(iface)
            except:
                continue

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

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

"""
interfaces.py - The OpenSAND interfaces management
"""

import netifaces
import logging
import socket
import fcntl
import struct
import threading
from ConfigParser import Error
from ipaddr import IPNetwork

from opensand_daemon.my_exceptions import InstructionError
from opensand_daemon.nl_utils import NlInterfaces, NlError, NlExists, \
                                     NlMissing, NlRoute


#macros
LOGGER = logging.getLogger('sand-daemon')

TUN_NAME = "opensand_tun"
TAP_NAME = "opensand_tap"
BR_NAME = "opensand_br"

# TODO DHCP and sysctl instead of script ?

def get_mac_address(interface):
    """ get the MAC address of an interface"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(sock.fileno(), 0x8927,  struct.pack('256s',
                      interface[:15]))
    return ''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1]



class OpenSandIfaces(object):
    """ the OpenSAND interfaces object """

    # the OpenSandIfaces class attributes are shared between main and
    # command threads (initialized/released in main, used in command)
    # to check initialization in command thread as it is done in main before
    # thread startup
    _name = ''
    _type = ''
    _emu_iface = ''
    _emu_ipv4 = None
    _lan_iface = ''
    _lan_ipv4 = None
    _lan_ipv6 = None
    _tun_ipv4 = None
    _tun_ipv6 = None
    _ifaces = NlInterfaces()
    _nladd = {}
    _ifdown = []
    _lock = threading.Lock()

    def __init__(self):
        pass

    def load(self, conf, name):
        """ load the interfaces """
        # no lock here because it is done before threads start
        OpenSandIfaces._name = name
        OpenSandIfaces._type = conf.get('network', 'config_level').lower()
        if OpenSandIfaces._type != 'advanced' and \
           OpenSandIfaces._type != 'automatic':
            raise Error("unknown network configuration type %s "
                        "(choose between advanced or automatic)" %
                        OpenSandIfaces._type)
        OpenSandIfaces._ifaces = NlInterfaces()
        if name != 'ws':
            self._init_emu(conf)
        if name != 'sat':
            self._init_lan(conf)

        if name != 'ws' and name != 'sat':
            self._init_tun()

        if name != 'ws':
            self._check_sysctl()

    def _init_emu(self, conf):
        """ init the emulation interface """
        OpenSandIfaces._emu_iface = conf.get('network', 'emu_iface')
        if OpenSandIfaces._type == 'advanced':
            OpenSandIfaces._emu_ipv4 = IPNetwork(conf.get('network',
                                                          'emu_ipv4'))
            try:
                # if the address exists this is not an error, do not keep it in
                # _nladd to avoid removing it at exit
                try:
                    OpenSandIfaces._ifaces.add_address(
                                    str(OpenSandIfaces._emu_ipv4),
                                    OpenSandIfaces._emu_iface)
                except NlExists:
                    LOGGER.info("address %s already exists on %s" %
                                (OpenSandIfaces._emu_ipv4,
                                 OpenSandIfaces._emu_iface))
                else:
                    OpenSandIfaces._nladd[str(OpenSandIfaces._emu_ipv4)] = \
                            OpenSandIfaces._emu_iface
                # if interface is already up this is not an error, do not keep
                # it in _ifdown to avoid changing its state at exit
                try:
                    OpenSandIfaces._ifaces.up(OpenSandIfaces._emu_iface)
                except NlExists:
                    LOGGER.debug("interface %s is already up" %
                                 OpenSandIfaces._emu_iface)
                else:
                    OpenSandIfaces._ifdown.append(OpenSandIfaces._emu_iface)
            except NlError, msg:
                LOGGER.error("unable to set addresses: %s" % msg)
                # delete the address that were already set and set adequate
                # interface down
                self.release()
            except Exception, msg:
                LOGGER.error("error when changing interface %s" %
                             OpenSandIfaces._emu_iface)
        else:
            try:
                # get the interface addresses
                addresses = netifaces.ifaddresses(OpenSandIfaces._emu_iface)
                # retrieve IPv4 and IPv6 addresses
                emu_ipv4 = addresses[netifaces.AF_INET]
                if len(emu_ipv4) > 1:
                    LOGGER.warning("more than one IPv4 addresses for emulation "
                                   "interface (%s), pick the first one: %s" %
                                   (OpenSandIfaces._emu_iface,
                                    emu_ipv4[0]['addr']))
                OpenSandIfaces._emu_ipv4 = IPNetwork("%s/%s" %
                                                     (emu_ipv4[0]['addr'],
                                                      emu_ipv4[0]['netmask']))
            except ValueError, msg:
                LOGGER.error("cannot get emulation interfaces addresses: %s" %
                             msg)
                raise
            except KeyError, val:
                LOGGER.error("an address with family %s is not set for "
                             "emulation interface" %
                             netifaces.address_families[val])
                raise

    def _init_lan(self, conf):
        """ init the lan interface """
        OpenSandIfaces._lan_iface = conf.get('network', 'lan_iface')
        if OpenSandIfaces._type == 'advanced':
            OpenSandIfaces._lan_ipv4 = IPNetwork(conf.get('network',
                                                          'lan_ipv4'))
            OpenSandIfaces._lan_ipv6 = IPNetwork(conf.get('network',
                                                          'lan_ipv6'))
            try:
                # if the address exists this is not an error, do not keep it in
                # _nladd to avoid removing it at exit
                try:
                    OpenSandIfaces._ifaces.add_address(
                                str(OpenSandIfaces._lan_ipv4),
                                OpenSandIfaces._lan_iface)
                except NlExists:
                    LOGGER.info("address %s already exists on %s" %
                                (OpenSandIfaces._lan_ipv4,
                                 OpenSandIfaces._lan_iface))
                else:
                    OpenSandIfaces._nladd[str(OpenSandIfaces._lan_ipv4)] = \
                                OpenSandIfaces._lan_iface
                try:
                    OpenSandIfaces._ifaces.add_address(
                                    str(OpenSandIfaces._lan_ipv6),
                                    OpenSandIfaces._lan_iface)
                except NlExists:
                    LOGGER.info("address %s already exists on %s" %
                                (OpenSandIfaces._lan_ipv6,
                                 OpenSandIfaces._lan_iface))
                else:
                    OpenSandIfaces._nladd[OpenSandIfaces._lan_ipv6] = \
                            OpenSandIfaces._lan_iface
                # if interface is already up this is not an error, do not keep
                # it in _ifdown to avoid changing its state at exit
                try:
                    OpenSandIfaces._ifaces.up(OpenSandIfaces._lan_iface)
                except NlExists:
                    LOGGER.debug("interface %s is already up" %
                                 OpenSandIfaces._lan_iface)
                else:
                    OpenSandIfaces._ifdown.append(OpenSandIfaces._lan_iface)
            except NlError, msg:
                LOGGER.error("unable to set addresses: %s" % msg)
                # delete the address that were already set
                self.release()
                raise NlError(msg)
            except Exception, msg:
                LOGGER.error("error when changing interface %s: %s" %
                             (OpenSandIfaces._lan_iface, msg))
                raise
        else:
            try:
                # get the interface addresses
                addresses = netifaces.ifaddresses(OpenSandIfaces._lan_iface)
                # retrieve IPv4 and IPv6 addresses
                lan_ipv4 = addresses[netifaces.AF_INET]
                if len(lan_ipv4) > 1:
                    LOGGER.warning("more than one IPv4 addresses for lan "
                                   "interface (%s), pick the first one: %s" %
                                   (OpenSandIfaces._lan_iface,
                                    lan_ipv4[0]['addr']))
                OpenSandIfaces._lan_ipv4 = IPNetwork("%s/%s" %
                                                     (lan_ipv4[0]['addr'],
                                                      lan_ipv4[0]['netmask']))
                lan_ipv6 = addresses[netifaces.AF_INET6]
                # remove IPv6 link addresses if there is another address
                # else use it
                # TODO check if it works with link local address
                if len(lan_ipv6) > 1:
                    copy = list(lan_ipv6)
                    for ip in copy:
                        addr = ip['addr']
                        if addr.endswith('%' + OpenSandIfaces._lan_iface):
                            lan_ipv6.remove(ip)
                if len(lan_ipv6) > 1:
                    LOGGER.warning("more than one IPv6 addresses for lan "
                                   "interface (%s), pick the first one: %s" %
                                   (OpenSandIfaces._lan_iface,
                                    lan_ipv6[0]['addr']))
                # remove the interface name after '%' for link local addresses
                addr_v6 = lan_ipv6[0]['addr'].rsplit('%', 1)[0]
                # IPNetwork does not convert IPv6 mask from address to number
                mask = lan_ipv6[0]['netmask']
                mask = mask.split(':')
                masklen = 0
                for b in [m for m in mask if len(m) > 0]:
                    count = bin(int(b, 16))
                    count = count.strip('0')
                    count = count.lstrip('b')
                    masklen += len(count)
                    
                OpenSandIfaces._lan_ipv6 = IPNetwork("%s/%s" % (addr_v6,
                                                                masklen))
            except ValueError, msg:
                LOGGER.error("cannot get emulation interfaces addresses: %s" %
                             msg)
                raise
            except KeyError, val:
                LOGGER.error("an address with family %s is not set for "
                             "emulation interface" %
                             netifaces.address_families[val])
                raise

    def _init_tun(self):
        """ init the TUN interfaces """
        # only set the correct address on TUN

        # compute the TUN address
        mask = OpenSandIfaces._lan_ipv4.prefixlen
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
        net, add = str(OpenSandIfaces._lan_ipv4.ip).rsplit('.', 1)
        # tun is 2 more than interface
        # we add 1 after modulo to avoid 0
        OpenSandIfaces._tun_ipv4 = IPNetwork("%s.%s/%s" % (net,
                                                           str(((int(add) + 1) %
                                                                modulo) + 1),
                                                           mask))

         # compute the TUN and addresse
        mask = OpenSandIfaces._lan_ipv6.prefixlen
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
        net, add = str(OpenSandIfaces._lan_ipv6.ip).rsplit(':', 1)
        # tun is 2 more than interface
        # we add 1 after modulo to avoid 0
        OpenSandIfaces._tun_ipv6 = IPNetwork("%s:%x/%s" % (net,
                                                           ((int(add, 16) + 1) %
                                                            modulo) + 1,
                                                           mask))

        # the interfaces are used by opensand so we don't care if address is
        # already set or not
        try:
            try:
                OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._tun_ipv4),
                                                   TUN_NAME)
            except NlExists:
                pass
            try:
                OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._tun_ipv6),
                                                   TUN_NAME)
            except NlExists:
                pass
        except NlError:
            LOGGER.error("error when configuring TUN and Bridge")

        self._remove_default_routes()


    def _remove_default_routes(self):
        """ delete default routes on tun interface """
        # delete default routes on tun (for bridge this is ok at it remains the
        # ain interface for both emulation network and terminal network)
        # (these are routes with kernel protocol)
        tun_route = NlRoute(TUN_NAME)
        try:
            route = "%s/%s" % (OpenSandIfaces._tun_ipv4.network,
                               OpenSandIfaces._tun_ipv4.prefixlen)
            tun_route.delete(route, None, True)
        except NlMissing:
            LOGGER.info("IPv4 default route %s for %s already deleted" %
                        (route, TUN_NAME))
        except NlError, msg:
            LOGGER.error("unable to delete IPv4 default route %s for %s: %s" %
                        (route, TUN_NAME, msg))
        try:
            route = "%s/%s" % (OpenSandIfaces._tun_ipv6.network,
                               OpenSandIfaces._tun_ipv6.prefixlen)
            tun_route.delete(route, None, True)
        except NlMissing:
            LOGGER.info("IPv6 default route %s for %s already deleted" %
                        (route, TUN_NAME))
        except NlError, msg:
            LOGGER.error("unable to delete IPv6 default route %s for %s: %s" %
                        (route, TUN_NAME, msg))

    def _check_sysctl(self):
        """ check sysctl values and log if the value may lead to errors """
        if OpenSandIfaces._name != 'sat':
            for iface in [OpenSandIfaces._emu_iface, OpenSandIfaces._lan_iface]:
                with open("/proc/sys/net/ipv4/conf/%s/forwarding" % iface,
                          'ro') as sysctl:
                    if sysctl.read().rstrip('\n') != "1":
                        LOGGER.warning("IPv4 forwarding on interface %s is "
                                       "disabled, you won't be able to route "
                                       "packets toward WS behind this host" %
                                       iface)
            for iface in [OpenSandIfaces._lan_iface]:
                with open("/proc/sys/net/ipv6/conf/%s/forwarding" % iface,
                          'ro') as sysctl:
                    if sysctl.read().rstrip('\n') != "1":
                        LOGGER.warning("IPv6 forwarding on interface %s is "
                                       "disabled, you won't be able to route "
                                       "packets toward WS behind this host" %
                                       iface)
            with open("/proc/sys/net/ipv4/ip_forward", 'ro') as sysctl:
                if sysctl.read().rstrip('\n') != "1":
                    LOGGER.warning("IPv4 ip_forward is disabled, you should "
                                   "enable it")

    def get_descr(self):
        """ get the addresses elements """
        descr = {}
        mac = ''
        if OpenSandIfaces._name != 'ws':
            descr.update({'emu_iface': OpenSandIfaces._emu_iface,
                          'emu_ipv4': str(OpenSandIfaces._emu_ipv4),
                         })
            if OpenSandIfaces._name != 'sat':
                if OpenSandIfaces._ifaces.exists(BR_NAME):
                    mac = get_mac_address(BR_NAME)
        if OpenSandIfaces._name != 'sat':
            descr.update({'lan_iface': OpenSandIfaces._lan_iface,
                          'lan_ipv4': OpenSandIfaces._lan_ipv4,
                          'lan_ipv6': OpenSandIfaces._lan_ipv6,
                         })
            if mac != '':
                descr.update({'mac': mac})
        return descr

    def setup_interfaces(self, is_l2):
        """ set interfaces for emulation """
        if OpenSandIfaces._name in ['sat', 'ws']:
            return
        OpenSandIfaces._lock.acquire()
        try:
            # for L3 we need to set the tun interface up and set the lan address on
            # the lan interface
            if not is_l2:
                self._setup_l3()
            # for L2 we need to remove the lan interface address and to set it on
            # the bridge
            elif is_l2:
                self._setup_l2()
            else:
                raise InstructionError("cannot set interfaces for unknwow layer")
        except Exception:
            raise
        finally:
            OpenSandIfaces._lock.release()

    def _setup_l3(self):
        """ set interface in IP mode """
        # add IPv4 address on lan interface (should be already done)
        try:
            OpenSandIfaces._ifaces.add_address(
                        str(OpenSandIfaces._lan_ipv4),
                        OpenSandIfaces._lan_iface)
        except NlExists:
            LOGGER.info("address %s already exists on %s" %
                        (OpenSandIfaces._lan_ipv4,
                         OpenSandIfaces._lan_iface))
        # add IPv6 address on lan interface (should be already done)
        try:
            OpenSandIfaces._ifaces.add_address(
                            str(OpenSandIfaces._lan_ipv6),
                            OpenSandIfaces._lan_iface)
        except NlExists:
            LOGGER.info("address %s already exists on %s" %
                        (OpenSandIfaces._lan_ipv6,
                         OpenSandIfaces._lan_iface))
        # set TUN interface up
        try:
            OpenSandIfaces._ifaces.up(TUN_NAME)
        except NlExists:
            LOGGER.debug("interface %s is already up" % TUN_NAME)

        self._remove_default_routes()

    def _setup_l2(self):
        """ set interface in Ethernet mode """
        # remove IPv4 address on lan interface
        try:
            OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv4),
                                               OpenSandIfaces._lan_iface)
        except NlMissing:
            LOGGER.info("address %s already removed on %s" %
                        (OpenSandIfaces._lan_ipv4,
                         OpenSandIfaces._lan_iface))
        # add IPv4 lan address on bridge
        try:
            OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._lan_ipv4),
                                               BR_NAME)
        except NlExists:
            LOGGER.info("address %s already exists on %s" %
                        (OpenSandIfaces._lan_ipv4, BR_NAME))
        # remove IPv6 address on lan interface
        try:
            OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv6),
                                               OpenSandIfaces._lan_iface)
        except NlMissing:
            LOGGER.info("address %s already removed on %s" %
                        (OpenSandIfaces._lan_ipv6,
                         OpenSandIfaces._lan_iface))
        # add IPv6 lan address on bridge
        try:
            OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._lan_ipv6),
                                               BR_NAME)
        except NlExists:
            LOGGER.info("address %s already exists on %s" %
                        (OpenSandIfaces._lan_ipv6, BR_NAME))
        # set TAP interface up
        try:
            OpenSandIfaces._ifaces.up(TAP_NAME)
        except NlExists:
            LOGGER.debug("interface %s is already up" % TAP_NAME)
        # set bridge interface up
        try:
            OpenSandIfaces._ifaces.up(BR_NAME)
        except NlExists:
            LOGGER.debug("interface %s is already up" % BR_NAME)
            
        # TODO add lan in bridge here and do not do that in core anymore
        

    def release(self):
        """ remove interfaces """
        for add in OpenSandIfaces._nladd:
            try:
                OpenSandIfaces._ifaces.del_address(add,
                                                   OpenSandIfaces._nladd[add])
            except:
                continue
        for iface in OpenSandIfaces._ifdown:
            try:
                OpenSandIfaces._ifaces.down(iface)
            except:
                continue

    def standby(self):
        """ set interfaces for emulation """
        if OpenSandIfaces._name in ['sat', 'ws']:
            return
        OpenSandIfaces._lock.acquire()
        # set opensand interfaces down and in case of layer 2 scenario, move
        # back address on lan interface
        try:
            self._standby()
        except Exception:
            raise
        finally:
            OpenSandIfaces._lock.release()

    def _standby(self):
        """ internal wrapper to simplify lock management """
        # remove IPv4 address on bridge if it exists
        if OpenSandIfaces._ifaces.exists(BR_NAME):
            try:
                OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv4),
                                                   BR_NAME)
            except NlMissing:
                LOGGER.info("address %s already removed on %s" %
                            (OpenSandIfaces._lan_ipv4, BR_NAME))
        # set back IPv4 address on lan interface
        try:
            OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._lan_ipv4),
                                               OpenSandIfaces._lan_iface)
        except NlExists:
            LOGGER.info("address %s already exists on %s" %
                        (OpenSandIfaces._lan_ipv4, OpenSandIfaces._lan_iface))

        # remove IPv6 address on bridge if it exists
        if OpenSandIfaces._ifaces.exists(BR_NAME):
            try:
                OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv6),
                                                   BR_NAME)
            except NlMissing:
                LOGGER.info("address %s already removed on %s" %
                            (OpenSandIfaces._lan_ipv6, BR_NAME))

        # set back IPv6 address on lan interface
        try:
            OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._lan_ipv6),
                                               OpenSandIfaces._lan_iface)
        except NlExists:
            LOGGER.info("address %s already exists on %s" %
                        (OpenSandIfaces._lan_ipv6, OpenSandIfaces._lan_iface))
        # set all opensand interfaces down
        try:
            OpenSandIfaces._ifaces.down(TUN_NAME)
        except NlMissing:
            LOGGER.info("interface %s is already down" % TUN_NAME)
        except Exception, msg:
            LOGGER.error("cannot set interface %s down (%s)" % (TUN_NAME, msg))
        if OpenSandIfaces._ifaces.exists(BR_NAME):
            try:
                OpenSandIfaces._ifaces.down(BR_NAME)
            except NlMissing:
                LOGGER.info("interface %s is already down" % BR_NAME)
            except Exception, msg:
                LOGGER.error("cannot set interface %s down (%s)" % (BR_NAME, msg))
        if OpenSandIfaces._ifaces.exists(TAP_NAME):
            try:
                OpenSandIfaces._ifaces.down(TAP_NAME)
            except NlMissing:
                LOGGER.info("interface %s is already down" % TAP_NAME)
            except Exception:
                # TODO find a way to set TAP down
                LOGGER.warning("cannot set TAP interface down, this always happend,"
                               " we need to find a solution (waiting for feedback "
                               "from the libnl mailing list)")
                pass

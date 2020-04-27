#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2020 TAS
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
# Author: Joaquin MUGUERZA / Viveris Technologies <jmuguerza@toulouse.viveris.com>
# Author: Aurélien DELRIEU / Viveris Technologies <aurelien.delrieu@viveris.fr>
# Author: Franklin SIMO <armelfrancklin.simotegueu@viveris.fr>

"""
interfaces.py - The OpenSAND interfaces management
"""


import logging
import socket
import fcntl
import struct
import threading
import subprocess
import os.path
from lxml import etree

from ConfigParser import Error
from ipaddr import IPNetwork, IPv4Address, IPv4Network

from opensand_daemon.my_exceptions import InstructionError
from opensand_daemon.nl_utils import NlInterfaces, NlError, NlExists, \
                                     NlMissing, NlRoute


#macros
LOGGER = logging.getLogger('sand-daemon')

TAP_NAME = "opensand_tap"
BR_NAME = "opensand_br"

CONF_DIR = "/etc/opensand/"
CORE_FILE = "core.conf"

# TODO DHCP and sysctl instead of script ?


def get_mac_address(interface):
    """ get the MAC address of an interface"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(sock.fileno(), 0x8927,  struct.pack('256s',
                      interface[:15]))
    sock.close()
    return ''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1]


# TODO when restarting daemon while OpenSAND is running, this always
#      consider we are in L3 but we could be in L2
class OpenSandIfaces(object):
    """ the OpenSAND interfaces object """

    # the OpenSandIfaces class attributes are shared between main and
    # command threads (initialized/released in main, used in command)
    # to check initialization in command thread as it is done in main before
    # thread startup
    _name = ''
    _emu_iface = ''
    _emu_ipv4 = None
    _lan_iface = ''
    _lan_ipv4 = None
    _lan_ipv6 = None
    _br_ipv4 = None
    _br_ipv6 = None
    _ifaces = NlInterfaces()
    _lock = threading.Lock()

    def __init__(self):
        pass

    def load(self, conf, name):
        """ load the interfaces """
        # no lock here because it is done before threads start
        OpenSandIfaces._name = name
        OpenSandIfaces._ifaces = NlInterfaces()
        if name not in { 'ws', 'gw-net-acc' }:
            self._init_emu(conf)

        if name not in { 'sat', 'gw-phy' }:
            self._init_lan(conf)

        if name not in { 'ws', 'sat', 'gw-phy' }:
            self._init_tap(conf)

        if name != 'ws':
            self._check_sysctl()

    def _init_emu(self, conf):
        """ init the emulation interface """
        OpenSandIfaces._emu_iface = conf.get('network', 'emu_iface')
        OpenSandIfaces._emu_ipv4 = IPNetwork(conf.get('network', 'emu_ipv4'))
        try:
            # if the address exists this is not an error
            try:
                LOGGER.info("set address {} to iface {}".format(
                    str(OpenSandIfaces._emu_ipv4), OpenSandIfaces._emu_iface))
                OpenSandIfaces._ifaces.add_address(
                                str(OpenSandIfaces._emu_ipv4),
                                OpenSandIfaces._emu_iface)
            except NlExists:
                LOGGER.debug("address %s already exists on %s" %
                            (OpenSandIfaces._emu_ipv4,
                             OpenSandIfaces._emu_iface))
            # if interface is already up this is not an error
            try:
                LOGGER.info("set up iface {}".format(
                    OpenSandIfaces._emu_iface))
                OpenSandIfaces._ifaces.up(OpenSandIfaces._emu_iface)
            except NlExists:
                LOGGER.debug("interface %s is already up" %
                             OpenSandIfaces._emu_iface)
        except NlError, msg:
            LOGGER.error("unable to set addresses: %s" % msg)
            # delete the address that were already set and set adequate
            # interface down
            self.release()
        except Exception, msg:
            LOGGER.error("error when changing interface %s" %
                         OpenSandIfaces._emu_iface)

    def _init_lan(self, conf):
        """ init the lan interface """
        OpenSandIfaces._lan_iface = conf.get('network', 'lan_iface')
        addr = conf.get('network', 'lan_ipv4')
        OpenSandIfaces._lan_ipv4 = IPNetwork(addr) if addr else None
        addr = conf.get('network', 'lan_ipv6')
        OpenSandIfaces._lan_ipv6 = IPNetwork(addr) if addr else None

        # if interface is already up this is not an error
        # if interface is '' or None this is not an error, it means that interface is not required
        if OpenSandIfaces._lan_iface not in [None, '']:
           try:
               LOGGER.info("set up iface {}".format(
                   OpenSandIfaces._lan_iface))
               OpenSandIfaces._ifaces.up(OpenSandIfaces._lan_iface)
           except NlExists:
               LOGGER.debug("interface %s is already up" %
                         OpenSandIfaces._lan_iface)
           except NlError, msg:
               LOGGER.error("unable to set up: %s" % msg)
               self.release()
           except Exception, msg:
               LOGGER.error("error when changing interface %s" %
                            OpenSandIfaces._lan_iface)
        else:
           LOGGER.debug("interface lan is not specified")

    def _init_tap(self, conf):
        """ init the TAP interfaces """
        addr = conf.get('network', 'int_ipv4')
        OpenSandIfaces._br_ipv4 = IPNetwork(addr) if addr else None
        addr = conf.get('network', 'int_ipv6')
        OpenSandIfaces._br_ipv6 = IPNetwork(addr) if addr else None

        # set TAP interface up
        try:
            LOGGER.info("set up iface {}".format(
                TAP_NAME))
            OpenSandIfaces._ifaces.up(TAP_NAME)
        except NlExists:
            LOGGER.debug("interface %s is already up" % TAP_NAME)
        except NlError, msg:
            LOGGER.error("unable to set up: %s" % msg)
            self.release()
        except Exception, msg:
            LOGGER.error("error when changing interface %s" %
                         TAP_NAME)

        # set bridge interface up
        try:
            LOGGER.info("set up iface {}".format(
                BR_NAME))
            OpenSandIfaces._ifaces.up(BR_NAME)
        except NlExists:
            LOGGER.debug("interface %s is already up" % BR_NAME)
        except NlError, msg:
            LOGGER.error("unable to set up: %s" % msg)
            self.release()
        except Exception, msg:
            LOGGER.error("error when changing interface %s" %
                         BR_NAME)

        # TAP iface is already in bridge, no need to attach it

    def _check_sysctl(self):
        """ check sysctl values and log if the value may lead to errors """
        if OpenSandIfaces._name == 'sat' or  OpenSandIfaces._name == 'gw-phy':
            return

        with open("/proc/sys/net/ipv4/ip_forward", 'ro') as sysctl:
            if sysctl.read().rstrip('\n') != "1":
                LOGGER.warning("IPv4 ip_forward is disabled, you should "
                               "enable it, so be able to route "
                               "packets toward WS behind this host")

        for iface in [OpenSandIfaces._lan_iface]:
            if OpenSandIfaces._ifaces.exists(iface):
               with open("/proc/sys/net/ipv4/conf/%s/forwarding" % iface,
                         'ro') as sysctl:
                   if sysctl.read().rstrip('\n') != "1":
                      LOGGER.warning("IPv4 forwarding on interface %s is "
                                     "disabled, you won't be able to route "
                                     "packets toward WS behind this host" % 
                                     iface)

    def get_descr(self):
        """ get the addresses elements """
        descr = {}
        mac = ''
        if OpenSandIfaces._name not in {'ws', 'gw-net-acc'}:
            descr.update({'emu_iface': OpenSandIfaces._emu_iface,
                          'emu_ipv4': str(OpenSandIfaces._emu_ipv4),
                         })
            if OpenSandIfaces._name not in {'sat', 'gw-phy'}:
                if OpenSandIfaces._ifaces.exists(BR_NAME):
                    mac = get_mac_address(BR_NAME)

        elif OpenSandIfaces._name in {'gw-net-acc'}:
            if OpenSandIfaces._ifaces.exists(BR_NAME):
                mac = get_mac_address(BR_NAME)

        else:
            if OpenSandIfaces._ifaces.exists(OpenSandIfaces._lan_iface):
               mac = get_mac_address(OpenSandIfaces._lan_iface)

        if OpenSandIfaces._name not in {'sat', 'gw-phy'}:
            if OpenSandIfaces._ifaces.exists(OpenSandIfaces._lan_iface):
                descr.update({'lan_iface': OpenSandIfaces._lan_iface,
                              'lan_ipv4': OpenSandIfaces._lan_ipv4 if OpenSandIfaces._lan_ipv4 is not None else '',
                              'lan_ipv6': OpenSandIfaces._lan_ipv6 if OpenSandIfaces._lan_ipv6 is not None else ''
                             })
            descr.update({'int_ipv4': OpenSandIfaces._br_ipv4 if OpenSandIfaces._br_ipv4 is not None else '',
                          'int_ipv6': OpenSandIfaces._br_ipv6 if OpenSandIfaces._br_ipv6 is not None else '',
                         })

            if mac != '':
                descr.update({'mac': mac})

        return descr

    def setup_interfaces(self):
        """ set interfaces for emulation """
        if OpenSandIfaces._name in ['sat', 'ws', 'gw-phy']:
            return

        # open xml
        tree = etree.parse(os.path.join(CONF_DIR, CORE_FILE))
        root = tree.getroot()
        childs = root.xpath("/configuration/global/lan_adaptation_schemes/lan_scheme")
        if childs is None:
            raise InstructionError("cannot get LAN adaptation scheme from XML file")
        if 1 < len(childs):
            raise InstructionError("too manny LAN adaptation scheme")
        is_l2 = False
        for child in childs:
            pos = child.get("pos")
            if pos is None or pos != "0":
                continue
            is_l2 = (child.get('proto') != 'IP')
            
        LOGGER.debug("is layer 2? %s" % "yes" if is_l2 else "no")

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
        if OpenSandIfaces._lan_iface not in [None,'']:
           try:
               if OpenSandIfaces._lan_ipv4 is not None:
                   # add IPv4 address on lan interface
                   try:
                       LOGGER.info("set ipv4 address {} to lan iface {}".format(
                           str(OpenSandIfaces._lan_ipv4),
                           OpenSandIfaces._lan_iface,
                       ))
                       OpenSandIfaces._ifaces.add_address(
                                   str(OpenSandIfaces._lan_ipv4),
                                   OpenSandIfaces._lan_iface)
                   except NlExists:
                       LOGGER.debug("address %s already exists on %s" %
                           (OpenSandIfaces._lan_ipv4, OpenSandIfaces._lan_iface)
                       )
               if OpenSandIfaces._lan_ipv6 is not None:
                   # add IPv6 address on lan interface
                   try:
                       LOGGER.info("set ipv6 address {} to lan iface {}".format(
                           str(OpenSandIfaces._lan_ipv6),
                           OpenSandIfaces._lan_iface,
                       ))
                       OpenSandIfaces._ifaces.add_address(
                                       str(OpenSandIfaces._lan_ipv6),
                                       OpenSandIfaces._lan_iface)
                   except NlExists:
                       LOGGER.debug("address %s already exists on %s" %
                           (OpenSandIfaces._lan_ipv6, OpenSandIfaces._lan_iface)
                       )
           except NlError:
               LOGGER.error("error when configuring %s" % OpenSandIfaces._lan_iface)
               raise

        # the interfaces are used by opensand so we don't care if address is
        # already set or not
        try:
            if OpenSandIfaces._br_ipv4 is not None:
                # add IPv4 address on bridge
                try:
                    LOGGER.info("set ipv4 address {} to bridge {}".format(
                        str(OpenSandIfaces._br_ipv4),
                        BR_NAME,
                    ))
                    OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._br_ipv4),
                                                       BR_NAME)
                except NlExists:
                    LOGGER.debug("address %s already exists on %s" %
                        (OpenSandIfaces._br_ipv4, BR_NAME)
                    )
            if OpenSandIfaces._br_ipv6 is not None:
                # add IPv6 address on bridge
                try:
                    LOGGER.info("set ipv6 address {} to bridge {}".format(
                        str(OpenSandIfaces._br_ipv6),
                        BR_NAME,
                    ))
                    OpenSandIfaces._ifaces.add_address(str(OpenSandIfaces._br_ipv6),
                                                       BR_NAME)
                except NlExists:
                    LOGGER.debug("address %s already exists on %s" %
                        (OpenSandIfaces._br_ipv6, BR_NAME)
                    )
        except NlError:
            LOGGER.error("error when configuring bridge")
            raise

        # remove lan iface from bridge
        if OpenSandIfaces._lan_iface not in [None, '']:
           OpenSandIfaces._ifaces.detach_iface(OpenSandIfaces._lan_iface)

    def _setup_l2(self):
        """ set interface in Ethernet mode """
        if OpenSandIfaces._lan_iface not in [None,'']:
           try:
               if OpenSandIfaces._lan_ipv4 is not None:
                   # remove IPv4 address on lan interface
                   try:
                       OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv4),
                                                          OpenSandIfaces._lan_iface)
                   except NlMissing:
                       LOGGER.debug("address %s already removed on %s" %
                           (OpenSandIfaces._lan_ipv4, OpenSandIfaces._lan_iface)
                       )

               if OpenSandIfaces._lan_ipv6 is not None:
                   # remove IPv6 address on lan interface
                   try:
                       OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv6),
                                                          OpenSandIfaces._lan_iface)
                   except NlMissing:
                       LOGGER.debug("address %s already removed on %s" %
                           (OpenSandIfaces._lan_ipv6, OpenSandIfaces._lan_iface)
                       )
           except NlError:
               LOGGER.error("error when configuring %s" % OpenSandIfaces._lan_iface)
               raise

        try:
            if OpenSandIfaces._br_ipv4 is not None:
                # remove IPv4 address on bridge
                try:
                    OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._br_ipv4),
                                                       BR_NAME)
                except NlMissing:
                    LOGGER.debug("address %s already removed on %s" %
                        (OpenSandIfaces._br_ipv4, BR_NAME)
                    )

            if OpenSandIfaces._br_ipv6 is not None:
                # remove IPv6 address on bridge
                try:
                    OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._br_ipv6),
                                                       BR_NAME)
                except NlMissing:
                    LOGGER.debug("address %s already removed on %s" %
                        (OpenSandIfaces._br_ipv6, BR_NAME)
                    )
        except NlError:
            LOGGER.error("error when configuring bridge")
            raise
            
        # add lan iface in bridge is not '' or not None
        if OpenSandIfaces._lan_iface not in [None,'']:
           OpenSandIfaces._ifaces.attach_iface(OpenSandIfaces._lan_iface, BR_NAME)

    def release(self):
        """ set interfaces for emulation """
        if OpenSandIfaces._name in ['sat', 'ws', 'gw-phy']:
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
        if OpenSandIfaces._lan_iface not in [None,'']:
           if OpenSandIfaces._lan_ipv4 is not None:
               # remove IPv4 address on lan interface
               try:
                   OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv4),
                                                      OpenSandIfaces._lan_iface)
               except NlMissing:
                   LOGGER.debug("address %s already removed on %s" %
                       (OpenSandIfaces._lan_ipv4, OpenSandIfaces._lan_iface)
                   )

           if OpenSandIfaces._lan_ipv6 is not None:
               # remove IPv6 address on lan interface
               try:
                   OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._lan_ipv6),
                                                   OpenSandIfaces._lan_iface)
               except NlMissing:
                   LOGGER.debug("address %s already removed on %s" %
                       (OpenSandIfaces._lan_ipv6, OpenSandIfaces._lan_iface)
                   )

        if OpenSandIfaces._ifaces.exists(BR_NAME):
            # remove IPv4 address on bridge if it exists
            if OpenSandIfaces._br_ipv4 is not None:
                try:
                    OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._br_ipv4),
                                                       BR_NAME)
                except NlMissing:
                    LOGGER.debug("address %s already removed on %s" %
                        (OpenSandIfaces._lan_ipv4, BR_NAME)
                    )

            # remove IPv6 address on bridge if it exists
            if OpenSandIfaces._br_ipv6 is not None:
                try:
                    OpenSandIfaces._ifaces.del_address(str(OpenSandIfaces._br_ipv6),
                                                       BR_NAME)
                except NlMissing:
                    LOGGER.debug("address %s already removed on %s" %
                        (OpenSandIfaces._lan_ipv6, BR_NAME)
                    )

        # remove lan iface from bridge if is not '' or not None
        if OpenSandIfaces._lan_iface not in [None,'']:
           OpenSandIfaces._ifaces.detach_iface(OpenSandIfaces._lan_iface)

#!/usr/bin/env python3
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

"""
@file     opensand_network.py
@brief    Set up the network configuration to an OpenSAND entity.
@author   Aurélien DELRIEU <aurelien.delrieu@viveris.fr>
"""


import argparse
import network_utils as nu
import ipaddress
from functools import partial


ETH_TYPE = 'eth'
IP_TYPE = 'ip'
ALL_TYPES = [ IP_TYPE, ETH_TYPE ]
DEFAULT_TYPE = ALL_TYPES[0]


class NetConf:
    '''
    Class to configure the network of an link
    '''
    def __init__(self, name):
        '''
        Initialize network configuration.
        '''
        self._name = name
        self._netns = None

    def configure(self, *args, **kwargs):
        '''
        Configure the network of the link.
        '''
        raise NotImplementedError

    def revert(self, *args, **kwargs):
        '''
        Revert the network configuration of the link.
        '''
        raise NotImplementedError


class EmulatedLinkNetConf(NetConf):
    '''
    Class to configure the network of an emulated link
    '''
    def configure(self, emu_iface, emu_addr):
        '''
        Configure the network.

        Args:
            emu_iface  the existing interface to configure for the emulated link
            emu_addr   the address to set to the emulated link interface
        '''
        if emu_iface not in nu.list_ifaces(self._netns):
            raise ValueError('No interface "{}"'.format(emu_iface))
        nu.set_up(emu_iface, self._netns)
        nu.flush_address(emu_iface, self._netns)
        nu.add_address(emu_iface, emu_addr, self._netns)

    def revert(self, emu_iface):
        '''
        Revert the network configuration.

        Args:
            emu_iface  the existing interface to configure for the emulated link
        '''
        if emu_iface not in nu.list_ifaces(self._netns):
            raise ValueError('No interface "{}"'.format(emu_iface))
        nu.flush_address(emu_iface, self._netns)


class IPv4LinkNetConf(NetConf):
    '''
    Class to configure the network of an IPv4 link
    '''
    def configure(self, net_iface, net_addr, int_addr):
        '''
        Configure the network.

        Args:
            net_iface  the existing interface to configure the IPv4 network
            net_addr   the address to set to the IPv4 network interface
            int_addr   the address to set to the internal IPv4 network interface
        '''
        if net_iface not in nu.list_ifaces(self._netns):
            raise ValueError('No interface "{}"'.format(net_iface))

        tap_iface = '{}tap'.format(self._name)
        br_iface = '{}br'.format(self._name)
        nu.set_up(net_iface, self._netns)
        nu.flush_address(net_iface, self._netns)
        nu.add_address(net_iface, net_addr, self._netns)
        nu.create_tap_iface(tap_iface, self._netns)
        nu.set_up(tap_iface, self._netns)
        nu.create_bridge(br_iface, [ tap_iface ], self._netns)
        nu.set_up(br_iface, self._netns)
        nu.add_address(br_iface, int_addr, self._netns)

    def revert(self, net_iface):
        '''
        Revert the network configuration.

        Args:
            net_iface  the existing interface to configure the IPv4 network
        '''
        if net_iface not in nu.list_ifaces(self._netns):
            raise ValueError('No interface "{}"'.format(net_iface))

        tap_iface = '{}tap'.format(self._name)
        br_iface = '{}br'.format(self._name)
        nu.delete_iface(br_iface, self._netns)
        nu.delete_iface(tap_iface, self._netns)
        nu.flush_address(net_iface, self._netns)
        nu.flush_address(emu_iface, self._netns)


class EthernetLinkNetConf(NetConf):
    '''
    Class to configure the network of an Ethernet link
    '''
    def configure(self, net_iface):
        '''
        Configure the network.

        Args:
            net_iface  the existing interface to configure the IPv4 network
        '''
        if net_iface not in nu.list_ifaces(self._netns):
            raise ValueError('No interface "{}"'.format(net_iface))

        tap_iface = '{}tap'.format(self._name)
        br_iface = '{}br'.format(self._name)
        nu.set_up(net_iface, self._netns)
        nu.flush_address(net_iface, self._netns)
        nu.create_tap_iface(tap_iface, self._netns)
        nu.set_up(tap_iface, self._netns)
        nu.create_bridge(br_iface, [ tap_iface, net_iface ], self._netns)
        nu.set_up(br_iface, self._netns)

    def revert(self, net_iface):
        '''
        Revert the network configuration.

        Args:
            net_iface  the existing interface to configure the Ethernet network
        '''
        if net_iface not in nu.list_ifaces(self._netns):
            raise ValueError('No interface "{}"'.format(net_iface))

        tap_iface = '{}tap'.format(self._name)
        br_iface = '{}br'.format(self._name)
        nu.delete_iface(br_iface, self._netns)
        nu.delete_iface(tap_iface, self._netns)
        nu.flush_address(net_iface, self._netns)
        nu.flush_address(emu_iface, self._netns)


def ipv4_address_netdigit(text):
    '''
    Check a text represents an IPv4 address and a net digit

    Args:
        text   text to check
    '''
    addr = ipaddress.ip_network(text)
    return addr.compressed


def existing_iface(text):
    '''
    Check a text represents an existing interface

    Args:
        text   text to check
    '''
    if text not in ni.interfaces():
        raise ValueError('No "{}" interface'.format(text))
    return text


def udp_port(text):
    '''
    Check a text represents a UDP port

    Args:
        text   text to check
    '''
    value = int(text)
    if value <= 0:
        raise ValueError('UDP port must be strictly positive')
    return value


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Set up the network configuration to an OpenSAND entity',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    entity_cmd = parser.add_subparsers(
        dest='entity',
        metavar='entity',
        help='the OpenSAND entity type',
    )
    entity_cmd.required = True

    st_parser = entity_cmd.add_parser(
        'st',
        help='Satellite Terminal',
    )
    gw_parser = entity_cmd.add_parser(
        'gw',
        help='Gateway',
    )
    sat_parser = entity_cmd.add_parser(
        'sat',
        help='Satellite',
    )
    gw_net_acc_parser = entity_cmd.add_parser(
        'gw_net_acc',
        help='Gateway (network and access layers)',
    )
    gw_phy_parser = entity_cmd.add_parser(
        'gw_phy',
        help='Gateway (physical layer)',
    )

    for p in [ st_parser, gw_parser, gw_net_acc_parser, gw_phy_parser ]:
        p.add_argument(
            '-i',
            '--id',
            type=int,
            required=True,
            help='the OpenSAND entity identifier',
        )

    for p in [ st_parser, gw_parser, gw_net_acc_parser ]:
        p.add_argument(
            '-t',
            '--type',
            choices=ALL_TYPES,
            default=DEFAULT_TYPE,
            help='the type of the terrestrial network',
        )
        p.add_argument(
            '-n',
            '--net-iface',
            type=existing_iface,
            required=True,
            help='the terrestrial network interface',
        )
        p.add_argument(
            '--net-addr',
            type=ipv4_address_netdigit,
            default=None,
            help='the terrestrial network address (format: "ADDRESS/NET_DIGIT")',
        )

    for p in [ st_parser, gw_parser, gw_phy_parser, sat_parser ]:
        p.add_argument(
            '-e',
            '--emu-iface',
            type=existing_iface,
            required=True,
            help='the emulation interface',
        )
        p.add_argument(
            '-a',
            '--emu-addr',
            type=ipv4_address_netdigit,
            required=True,
            help='the emulation address (format: "ADDRESS/NET_DIGIT")',
        )

    for p in [ gw_net_acc_parser, gw_phy_parser ]:
        p.add_argument(
            '-o',
            '--interconnect-iface',
            type=int,
            required=True,
            help='the interconnect interface',
        )
        p.add_argument(
            '--interconnect-addr',
            type=ipv4_address_netdigit,
            required=True,
            help='the interconnect address (format: "ADDRESS/NET_DIGIT")',
        )

    for p in [ st_parser, gw_parser, gw_net_acc_parser ]:
        p.add_argument(
            '--internal-addr',
            type=ipv4_address_netdigit,
            default=None,
            help='the internal address (format: "ADDRESS/NET_DIGIT")',
        )

    for p in [ st_parser, gw_parser, gw_net_acc_parser, gw_phy_parser, sat_parser ]:
        p.add_argument(
            '-r',
            '--revert',
            action='store_false',
            dest='configure',
            help='revert the current network configuration',
        )

    args = parser.parse_args()
    actions = []
    if args.entity == 'sat':
        link = EmulatedLinkNetConf(args.entity)
        if args.configure:
            actions.append(partial(link.configure, args.emu_iface, args.emu_addr))
        else:
            actions.append(partial(link.revert, args.emu_iface))

    elif args.entity in [ 'st', 'gw' ]:
        link = EmulatedLinkNetConf(args.entity)
        if args.configure:
            actions.append(partial(link.configure, args.emu_iface, args.emu_addr))
        else:
            actions.append(partial(link.revert, args.emu_iface))

        if args.type == IP_TYPE:
            link = IPv4LinkNetConf('{}{}'.format(args.entity, args.id))
            if args.configure:
                actions.append(partial(link.configure, args.net_iface, args.net_addr, args.int_addr))
            else:
                actions.append(partial(link.revert, args.net_iface))

        elif args.type == ETH_TYPE:
            link = EthernetLinkNetConf('{}{}'.format(args.entity, args.id))
            if args.configure:
                actions.append(partial(link.configure, args.net_iface))
            else:
                actions.append(partial(link.revert, args.net_iface))

        else:
            raise ValueError('Unexpected type value "{}"'.format(args.type))

    elif args.entity == 'gw_net_acc':
        link = EmulatedLinkNetConf(args.entity)
        if args.configure:
            actions.append(partial(link.configure, args.interconnect_iface, args.interconnect_addr))
        else:
            actions.append(partial(link.revert, args.interconnect_iface))

        if args.type == IP_TYPE:
            link = IPv4LinkNetConf('{}{}'.format(args.entity, args.id))
            if args.configure:
                actions.append(partial(link.configure, args.net_iface, args.net_addr, args.int_addr))
            else:
                actions.append(partial(link.revert, args.net_iface))

        elif args.type == ETH_TYPE:
            link = EthernetLinkNetConf('{}{}'.format(args.entity, args.id))
            if args.configure:
                actions.append(partial(link.configure, args.net_iface))
            else:
                actions.append(partial(link.revert, args.net_iface))

        else:
            raise ValueError('Unexpected type value "{}"'.format(args.type))

    elif args.entiy == 'gw_phy':
        link = EmulatedLinkNetConf(args.entity)
        if args.configure:
            actions.append(partial(link.configure, args.emu_iface, args.emu_addr))
        else:
            actions.append(partial(link.revert, args.emu_iface))

        link = EmulatedLinkNetConf(args.entity)
        if args.configure:
            actions.append(partial(link.configure, args.interconnect_iface, args.interconnect_addr))
        else:
            actions.append(partial(link.revert, args.interconnect_iface))

    for action in actions:
        action()

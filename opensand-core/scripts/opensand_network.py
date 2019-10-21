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
import subprocess
import ipaddress
import re
import netifaces as ni


ETH_TYPE = 'eth'
IP_TYPE = 'ip'
ALL_TYPES = [ IP_TYPE, ETH_TYPE ]
DEFAULT_TYPE = ALL_TYPES[0]
DEFAULT_INTERNAL_NETNS = 'opensand'
DEFAULT_INTERNAL_NETWORK = '10.0.0.0/24'


def exec_cmd(cmd, error_msg):
    '''
    Execute a command and raise an error in case of non-null returned value

    Args:
        cmd        the command to execute
        error      the message to raise in case of error

    Return lines of the stdout
    '''
    print(cmd)
    proc = subprocess.run(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if proc.returncode < 0:
        raise Exception('{}: {}'.format(error, proc.stderr.decode()))
    return proc.stdout.decode().splitlines()


def create_netns(netns):
    exec_cmd(
        'ip netns add {}'.format(netns),
        'Netns creation failed',
    )


def delete_netns(netns):
    exec_cmd(
        'ip netns del {}'.format(netns),
        'Netns deletion failed',
    )


def exist_netns(netns):
    output = exec_cmd(
        'ip netns show {}'.format(netns),
        'Netns showing failed',
    )
    for line in output:
        match = re.search('([a-zA-Z0-9_\-]+) .*', line)
        if match and match.group(1) == netns:
            return True
    return False


def list_ifaces(netns=None):
    output = exec_cmd(
        '{}ip link list'.format(
            'ip netns exec {} '.format(netns) if netns else '',
        ),
        'Interface listing failed',
    )
    ifaces = []
    for line in output:
        match = re.search('([a-zA-Z0-9_\-]+): <.*', line)
        if match:
            ifaces.append(match.group(1))
    return ifaces


def set_up(iface, netns=None):
    exec_cmd(
        '{}ip link set {} up'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Setting up interface failed',
    )


def set_mac_address(iface, mac_address, netns=None):
    exec_cmd(
        '{}ip link set {} address {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
            mac_address,
        ),
        'MAC address setting failed',
    )


def flush_address(iface, netns=None):
    exec_cmd(
        '{}ip address flush dev {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Addresses flush failed',
    )


def add_address(iface, address, netns=None):
    exec_cmd(
        '{}ip address add {} dev {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            address,
            iface,
        ),
        'Address addition failed',
    )


def create_dummy_iface(iface, netns=None):
    exec_cmd(
        '{}ip link add name {} type dummy'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Dummy interface creation failed',
    )


def create_veth_ifaces_pair(iface, peeriface):
    exec_cmd(
        'ip link add name {} type veth peer name {}'.format(iface, peeriface),
        'Veth interfaces pair creation failed',
    )


def move_iface(iface, netns):
    exec_cmd(
        'ip link set {} netns {}'.format(iface, netns),
        'Interface moving to netns failed',
   )


def create_tap_iface(iface, netns=None):
    exec_cmd(
        '{}ip tuntap add mode tap {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Tap interface creation failed',
    )


def create_bridge(bridge, ifaces, netns=None):
    exec_cmd(
        '{}ip link add name {} type bridge'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            bridge,
        ),
        'Bridge interface creation failed',
    )
    for iface in ifaces:
        exec_cmd(
            '{}ip link set {} master {}'.format(
                'ip netns exec {} '.format(netns) if netns else '',
                iface,
                bridge,
            ),
            'Attaching iface to bridge failed',
        )
    exec_cmd(
        '{}brctl setageing {} 0'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            bridge,
        ),
        'Setting bridge ageing failed',
    )


def delete_iface(iface, netns=None):
    exec_cmd(
        '{}ip link del dev {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Interface deletion failed',
    )


def add_default_route(iface=None, gateway=None, netns=None):
    if not iface and not gateway:
        raise ValueError('Invalid parameters for default route addition')
    prefix = 'ip netns exec {} '.format(netns) if netns else ''
    exec_cmd(
        '{}ip route add default{}{}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            ' via {}'.format(gateway) if gateway else '',
            ' dev {}'.format(iface) if iface else '',
        ),
        'Default route addition failed',
    )


def add_route(destination, iface=None, gateway=None, netns=None):
    if not iface and not gateway:
        raise ValueError('Invalid parameters for default route addition')
    prefix = 'ip netns exec {} '.format(netns) if netns else ''
    exec_cmd(
        '{}ip route add {}{}{}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            destination,
            ' via {}'.format(gateway) if gateway else '',
            ' dev {}'.format(iface) if iface else '',
        ),
        'Default route addition failed',
    )


def enable_ip_forward(netns=None):
    exec_cmd(
        '{}sysctl net.ipv4.ip_forward=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
        ),
        'IP forward enabling failed',
    )
    

def enable_iface_ip_forward(iface, netns=None):
    exec_cmd(
        '{}sysctl net.ipv4.conf.{}.forwarding=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'IPv4 forward enabling failed',
    )
    exec_cmd(
        '{}sysctl net.ipv6.conf.{}.forwarding=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'IPv6 forward enabling failed',
    )
    

def enable_proxy_arp(netns=None):
    exec_cmd(
        '{}sysctl net.ipv4.conf.all.proxy_arp=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
        ),
        'Proxy ARP enabling failed',
    )


def setup_emu(
        label,
        emu_phy,
        addr,
        netns=None,
        netns_phy=None,
        revert=False,
    ):
    '''
    Set up the emulation

    Args:
        label       the entity label
        emu_phy     the emulation interface
        netns       the internal network namespace
        netns_phy   the network namespace of the emulation interface
        revert      False to apply the configuration, True to revert it
    '''
    if not revert:
        # Prepare physical net namespace
        if netns_phy and not exist_netns(netns_phy):
            create_netns(netns_phy)
        enable_ip_forward(netns_phy)

        # Prepare internal net namespace
        if netns and not exist_netns(netns):
            create_netns(netns)
        enable_ip_forward(netns)

        # Prepare EMU interface
        set_up(emu_phy, netns=netns_phy)
        enable_iface_ip_forward(emu_phy, netns=netns_phy)

        # Prepare additional interfaces
        if netns or netns_phy:
            create_veth_ifaces_pair('{}_emu'.format(label), '{}_emu_int'.format(label))
            if netns_phy:
                move_iface('{}_emu'.format(label), netns_phy)
            if netns:
                move_iface('{}_emu_int'.format(label), netns)
            set_up('{}_emu'.format(label), netns=netns_phy)
            set_up('{}_emu_int'.format(label), netns=netns)

            create_bridge('br_{}_emu'.format(label), ['{}_emu'.format(label), emu_phy], netns=netns_phy)
            set_up('br_{}_emu'.format(label), netns=netns_phy)

            flush_address('{}_emu_int'.format(label), netns=netns)
            add_address('{}_emu_int'.format(label), addr, netns=netns)

    else:
        # Clean additional interface
        if netns or netns_phy:
            delete_iface('br_{}_emu'.format(label), netns=netns_phy)
            delete_iface('{}_emu'.format(label), netns=netns_phy)

        # Clean internal net namespace if no more interface
        if netns:
            ifaces = list_ifaces(netns=netns)
            if 'lo' in ifaces:
                ifaces.remove('lo')
            if not ifaces:
                delete_netns(netns)

        # Clean physical net namespace if no more interface
        if netns_phy:
            ifaces = list_ifaces(netns=netns_phy)
            if 'lo' in ifaces:
                ifaces.remove('lo')
            if not ifaces:
                delete_netns(netns_phy)


def setup_lan_ip(
        label,
        lan_phy,
        mac_addr,
        addr,
        netns=None,
        netns_phy=None,
        revert=False,
    ):
    '''
    Set up the IP LAN

    Args:
        label       the entity label
        lan_phy     the LAN interface
        mac_addr    the MAC address
        addr        the IP address
        netns       the internal network namespace
        netns_phy   the network namespace of the emulation interface
        revert      False to apply the configuration, True to revert it
    '''
    if not revert:
        # Prepare physical net namespace
        if netns_phy and not exist_netns(netns_phy):
            create_netns(netns_phy)
        enable_ip_forward(netns_phy)
        enable_proxy_arp(netns_phy)

        # Prepare internal net namespace
        if netns and not exist_netns(netns):
            create_netns(netns)
        enable_ip_forward(netns)
        enable_proxy_arp(netns)

        # Prepare LAN physical interface
        set_up(lan_phy, netns=netns_phy)
        enable_iface_ip_forward(lan_phy, netns=netns_phy)

        # Prepare TAP interface
        create_tap_iface('{}_tap'.format(label), netns=netns)
        set_up('{}_tap'.format(label), netns=netns)

        # Prepare additional interfaces
        if netns or netns_phy:
            create_veth_ifaces_pair('{}_lan'.format(label), '{}_lan_int'.format(label))
            if netns_phy:
                move_iface('{}_lan'.format(label), netns_phy)
            if netns:
                move_iface('{}_lan_int'.format(label), netns)
            set_up('{}_lan'.format(label), netns=netns_phy)
            set_up('{}_lan_int'.format(label), netns=netns)

            set_mac_address('{}_lan'.format(label), mac_addr, netns=netns_phy)

            create_bridge('br_{}_tap'.format(label), ['{}_tap'.format(label), '{}_lan_int'.format(label)], netns=netns)
            set_up('br_{}_tap'.format(label), netns=netns)

            flush_address('{}_lan'.format(label), netns=netns_phy)
            add_address('{}_lan'.format(label), addr, netns=netns_phy)

        else:
            create_dummy_iface('{}_lan'.format(label))
            set_up('{}_lan'.format(label))
            enable_iface_ip_forward('{}_lan'.format(label))

            set_mac_address('{}_tap'.format(label), mac_addr)

            create_bridge('br_{}_lan'.format(label), ['{}_tap'.format(label), '{}_lan'.format(label)])
            set_up('br_{}_lan'.format(label))

            flush_address('br_{}_lan'.format(label))
            add_address('br_{}_lan'.format(label), addr)

    else:
        # Clean additional interfaces
        if netns or netns_phy:
            delete_iface('br_{}_tap'.format(label), netns=netns)
            delete_iface('{}_lan'.format(label), netns=netns_phy)
        else:
            delete_iface('br_{}_lan'.format(label), netns=netns)
            delete_iface('{}_lan'.format(label))

        # Clean TAP interface
        delete_iface('{}_tap'.format(label), netns=netns)

        # Clean internal net namespace if no more interface
        if netns:
            ifaces = list_ifaces(netns=netns)
            if 'lo' in ifaces:
                ifaces.remove('lo')
            if not ifaces:
                delete_netns(netns)

        # Clean physical net namespace if no more interface
        if netns_phy:
            ifaces = list_ifaces(netns=netns_phy)
            if 'lo' in ifaces:
                ifaces.remove('lo')
            if not ifaces:
                delete_netns(netns_phy)


def setup_lan_eth(
        label,
        lan_phy,
        mac_addr,
        netns=None,
        netns_phy=None,
        revert=False,
    ):
    '''
    Set up the Ethernet LAN

    Args:
        label       the entity label
        lan_phy     the LAN interface
        mac_addr    the MAC address
        netns       the internal network namespace
        netns_phy   the network namespace of the emulation interface
        revert      False to apply the configuration, True to revert it
    '''
    if not revert:
        # Prepare physical net namespace
        if netns_phy and not exist_netns(netns_phy):
            create_netns(netns_phy)
        enable_ip_forward(netns_phy)
        enable_proxy_arp(netns_phy)

        # Prepare internal net namespace
        if netns and not exist_netns(netns):
            create_netns(netns)
        enable_ip_forward(netns)
        enable_proxy_arp(netns)

        # Prepare LAN physical interface
        set_up(lan_phy, netns=netns_phy)
        enable_iface_ip_forward(lan_phy, netns=netns_phy)

        # Prepare TAP interface
        create_tap_iface('{}_tap'.format(label), netns=netns)
        set_up('{}_tap'.format(label), netns=netns)

        # Prepare additional interfaces
        if netns or netns_phy:
            create_veth_ifaces_pair('{}_lan'.format(label), '{}_lan_int'.format(label))
            if netns_phy:
                move_iface('{}_lan'.format(label), netns_phy)
            if netns:
                move_iface('{}_lan_int'.format(label), netns)
            set_up('{}_lan'.format(label), netns=netns_phy)
            set_up('{}_lan_int'.format(label), netns=netns)

            #set_mac_address('{}_lan'.format(label), mac_addr, netns=netns_phy)

            create_bridge('br_{}_tap'.format(label), ['{}_tap'.format(label), '{}_lan_int'.format(label)], netns=netns)
            set_up('br_{}_tap'.format(label), netns=netns)

            create_bridge('br_{}_lan'.format(label), [lan_phy, '{}_lan'.format(label)], netns=netns_phy)
            set_up('br_{}_lan'.format(label), netns=netns_phy)

        else:
            #set_mac_address('br_{}_tap'.format(label), mac_addr)

            create_bridge('br_{}_lan'.format(label), ['{}_tap'.format(label), lan_phy])
            set_up('br_{}_lan'.format(label))

    else:
        # Clean additional interfaces
        if netns or netns_phy:
            delete_iface('br_{}_lan'.format(label), netns=netns_phy)
            delete_iface('br_{}_tap'.format(label), netns=netns)
            delete_iface('{}_lan'.format(label), netns=netns_phy)
        else:
            delete_iface('br_{}_lan'.format(label))

        # Clean TAP interface
        delete_iface('{}_tap'.format(label), netns=netns)

        # Clean internal net namespace if no more interface
        if netns:
            ifaces = list_ifaces(netns=netns)
            if 'lo' in ifaces:
                ifaces.remove('lo')
            if not ifaces:
                delete_netns(netns)

        # Clean physical net namespace if no more interface
        if netns_phy:
            ifaces = list_ifaces(netns=netns_phy)
            if 'lo' in ifaces:
                ifaces.remove('lo')
            if not ifaces:
                delete_netns(netns_phy)


def setup_network_sat(
        emu_iface,
        internal_netns=DEFAULT_INTERNAL_NETNS,
        revert=False,
    ):
    '''
    Set up the network configuration to an Satellite Terminal or to a Gateway.

    Args:
        emu_iface         the emulation interface
        internal_netns    the internal network namespace
        revert            False to apply the configuration, True to revert it
    '''
    if not revert and (emu_iface not in ni.interfaces() or ni.AF_INET not in ni.ifaddresses(emu_iface) or not ni.ifaddresses(emu_iface)[ni.AF_INET]):
        raise ValueError('Emulation interface {} has no Ipv4 address'.format(emu_iface))

    if not revert:
        ifaddr = ni.ifaddresses(emu_iface)[ni.AF_INET][0]
    else:
        ifaddr = {'addr': '0.0.0.0', 'netmask': '255.255.255.255'}
    setup_emu(
            'sat',
            emu_iface,
            '{}/{}'.format(ifaddr['addr'], ifaddr['netmask']),
            internal_netns,
            None,
            revert,
        )


def setup_network_st_gw(
        entity,
        label,
        lan_iface,
        emu_iface,
        addr_idx,
        type=DEFAULT_TYPE,
        internal_netns=DEFAULT_INTERNAL_NETNS,
        internal_network=DEFAULT_INTERNAL_NETWORK,
        revert=False,
    ):
    '''
    Set up the network configuration to an Satellite Terminal or to a Gateway.

    Args:
        entity            the entity type (st or gw)
        label             the entity label
        addr_idx          the addess index (e.g. 10.0.0.<addr_idx>)
        lan_iface         the LAN interface
        type              the network type (Ethernet or IP)
        emu_iface         the emulation interface
        internal_netns    the internal network namespace
        internal_network  the internal network address
        revert            False to apply the configuration, True to revert it
    '''
    if not revert and (emu_iface not in ni.interfaces() or ni.AF_INET not in ni.ifaddresses(emu_iface) or not ni.ifaddresses(emu_iface)[ni.AF_INET]):
        raise ValueError('Emulation interface {} has no Ipv4 address'.format(emu_iface))
    if not revert and (addr_idx <= 0 or addr_idx > 255):
        raise ValueError('Address index must be a interger strictly positive and strictly lower than 255')

    if not revert:
        ifaddr = ni.ifaddresses(emu_iface)[ni.AF_INET][0]
    else:
        ifaddr = {'addr': '0.0.0.0', 'netmask': '255.255.255.255'}
    setup_emu(
            label,
            emu_iface,
            '{}/{}'.format(ifaddr['addr'], ifaddr['netmask']),
            internal_netns,
            None,
            revert,
        )
    if type == ETH_TYPE:
        setup_lan_eth(
                label,
                lan_iface,
                '00:00:00:00:00:{0:02x}'.format(addr_idx),
                internal_netns,
                None,
                revert,
            )

    elif type == IP_TYPE:
        network = ipaddress.ip_network(internal_network)
        setup_lan_ip(
                label,
                lan_iface,
                '00:00:00:00:00:{0:02x}'.format(addr_idx),
                '{}/{}'.format(network[addr_idx], network.prefixlen),
                internal_netns,
                None,
                revert,
            )


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
            help='the type of the LAN connection (IP or Ethernet)',
        )
        p.add_argument(
            '-l',
            '--lan-iface',
            type=str,
            required=True,
            help='the LAN interface',
        )

    for p in [ st_parser, gw_parser, gw_phy_parser, sat_parser ]:
        p.add_argument(
            '-e',
            '--emu-iface',
            type=str,
            required=True,
            help='the emulation interface (not required if all entities run locally)',
        )

#    for p in [ gw_net_acc_parser, gw_phy_parser ]:
#        p.add_argument(
#            '-o',
#            '--interconnect-iface',
#            type=int,
#            help='the interconnect interface (not required if all entities run locally)',
#        )

    for p in [ st_parser, gw_parser, gw_net_acc_parser ]:
        p.add_argument(
            '--internal-network',
            type=str,
            default=DEFAULT_INTERNAL_NETWORK,
            help='the internal network (format: "ADDRESS/NET_DIGIT")',
        )

    for p in [ st_parser, gw_parser, gw_net_acc_parser, gw_phy_parser, sat_parser ]:
        p.add_argument(
            '--disable-netns',
            action='store_true',
            help='disable the internal network namespace',
        )
        p.add_argument(
            '--internal-netns',
            type=str,
            default=DEFAULT_INTERNAL_NETNS,
            help='the internal network namespace',
        )
        p.add_argument(
            '-r',
            '--revert',
            action='store_true',
            help='revert the current network configuration',
        )

    args = parser.parse_args()
    if args.entity == 'sat':
        setup_network_sat(
            args.emu_iface,
            args.internal_netns if not args.disable_netns else None,
            args.revert,
        )

    elif args.entity in [ 'st', 'gw' ]:
        setup_network_st_gw(
            args.entity,
            '{}{}'.format(args.entity, args.id),
            args.lan_iface,
            args.emu_iface,
            args.id + 1,
            args.type,
            args.internal_netns if not args.disable_netns else None,
            args.internal_network,
            args.revert,
        )

    elif args.entity == 'gw_net_acc':
        raise NotImplementedError()
#        setup_network_gw_net_acc(
#            '{}{}'.format(args.entity, args.id),
#            args.id + 1,
#            args.lan_iface,
#            args.type,
#            args.interconnect_iface,
#            args.internal_netns if not args.disable_netns else None,
#            args.internal_network,
#            args.interconnect_network,
#            args.revert,
#        )

    elif args.entiy == 'gw_phy':
        raise NotImplementedError()
#        setup_network_gw_phy(
#            '{}{}'.format(args.entity, args.id),
#            args.interconnect_iface,
#            args.emu_iface,
#            args.internal_netns if not args.disable_netns else None,
#            args.internal_network,
#            args.interconnect_network,
#            args.revert,
#        )

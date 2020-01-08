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
@file     opensand_network_utils.py
@brief    Basic functions to configure network.
@author   Aurélien DELRIEU <aurelien.delrieu@viveris.fr>
"""


import subprocess
import re


ETH_TYPE = 'eth'
IP_TYPE = 'ip'
ALL_TYPES = [ IP_TYPE, ETH_TYPE ]
DEFAULT_TYPE = ALL_TYPES[0]

DEBUG = False
RAISE_ERROR = True


class NetworkUtilsError(Exception):
    pass


def __exec_cmd(cmd, error_msg):
    '''
    Execute a command and raise an error in case of non-null returned value

    Args:
        cmd        the command to execute
        error      the message to raise in case of error

    Return lines of the stdout
    '''
    if DEBUG:
        print(cmd)
    proc = subprocess.run(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if RAISE_ERROR and proc.returncode < 0:
        raise NetworkUtilsError('{}: {}'.format(error, proc.stderr.decode()))
    return proc.stdout.decode().splitlines()


def create_netns(netns):
    __exec_cmd(
        'ip netns add {}'.format(netns),
        'Netns creation failed',
    )


def delete_netns(netns):
    __exec_cmd(
        'ip netns del {}'.format(netns),
        'Netns deletion failed',
    )


def exist_netns(netns):
    output = __exec_cmd(
        'ip netns show {}'.format(netns),
        'Netns showing failed',
    )
    for line in output:
        match = re.search('([a-zA-Z0-9_\-]+) .*', line)
        if match and match.group(1) == netns:
            return True
    return False


def list_ifaces(netns=None):
    output = __exec_cmd(
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
    __exec_cmd(
        '{}ip link set {} up'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Setting up interface failed',
    )


def set_mac_address(iface, mac_address, netns=None):
    __exec_cmd(
        '{}ip link set {} address {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
            mac_address,
        ),
        'MAC address setting failed',
    )


def flush_address(iface, netns=None):
    __exec_cmd(
        '{}ip address flush dev {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Addresses flush failed',
    )


def add_address(iface, address, netns=None):
    __exec_cmd(
        '{}ip address add {} dev {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            address,
            iface,
        ),
        'Address addition failed',
    )


def create_dummy_iface(iface, netns=None):
    __exec_cmd(
        '{}ip link add name {} type dummy'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Dummy interface creation failed',
    )


def create_veth_ifaces_pair(iface, peeriface):
    __exec_cmd(
        'ip link add name {} type veth peer name {}'.format(iface, peeriface),
        'Veth interfaces pair creation failed',
    )


def move_iface(iface, netns):
    __exec_cmd(
        'ip link set {} netns {}'.format(iface, netns),
        'Interface moving to netns failed',
   )


def create_tap_iface(iface, netns=None):
    __exec_cmd(
        '{}ip tuntap add mode tap {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Tap interface creation failed',
    )


def create_bridge(bridge, ifaces, netns=None):
    __exec_cmd(
        '{}ip link add name {} type bridge'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            bridge,
        ),
        'Bridge interface creation failed',
    )
    for iface in ifaces:
        __exec_cmd(
            '{}ip link set {} master {}'.format(
                'ip netns exec {} '.format(netns) if netns else '',
                iface,
                bridge,
            ),
            'Attaching iface to bridge failed',
        )
    __exec_cmd(
        '{}brctl setageing {} 0'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            bridge,
        ),
        'Setting bridge ageing failed',
    )


def attach_iface(bridge, iface, netns=None):
    __exec_cmd(
        '{}ip link set {} master {}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
            bridge,
        ),
        'Attaching iface to bridge failed',
    )


def delete_iface(iface, netns=None):
    __exec_cmd(
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
    __exec_cmd(
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
    __exec_cmd(
        '{}ip route add {}{}{}'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            destination,
            ' via {}'.format(gateway) if gateway else '',
            ' dev {}'.format(iface) if iface else '',
        ),
        'Default route addition failed',
    )


def enable_ipv4_forward(netns=None):
    __exec_cmd(
        '{}sysctl net.ipv4.ip_forward=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
        ),
        'IPv4 forward enabling failed',
    )


def enable_ipv4_forwarding(iface='all', netns=None):
    __exec_cmd(
        '{}sysctl net.ipv4.conf.{}.forwarding=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Interface IPv4 forwarding enabling failed',
    )


def enable_ipv6_forward(netns=None):
    __exec_cmd(
        '{}sysctl net.ipv6.ip_forward=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
        ),
        'IPv6 forward enabling failed',
    )


def enable_ipv6_forwarding(iface='all', netns=None):
    __exec_cmd(
        '{}sysctl net.ipv6.conf.{}.forwarding=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Interface IPv6 forwarding enabling failed',
    )
    

def enable_ipv4_proxy_arp(iface='all', netns=None):
    __exec_cmd(
        '{}sysctl net.ipv4.conf.{}.proxy_arp=1'.format(
            'ip netns exec {} '.format(netns) if netns else '',
            iface,
        ),
        'Proxy ARP enabling failed',
    )

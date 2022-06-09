#!/usr/bin/env python3
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

# Author: Aurélien DELRIEU <aurelien.delrieu@viveris.fr>


"""Basic functions to configure network."""


import re
import shlex
from subprocess import run, PIPE, CalledProcessError


ETH_TYPE = 'eth'
IP_TYPE = 'ip'
ALL_TYPES = [ IP_TYPE, ETH_TYPE ]
DEFAULT_TYPE = ALL_TYPES[0]


VERBOSITY = 0
RAISE_ERROR = True


class NetworkUtilsError(Exception):
    pass


def __exec_cmd(error_msg, *cmd, netns=None):
    '''
    Execute a command and raise an error in case of non-null returned value

    Args:
        cmd        the command to execute
        error      the message to raise in case of error

    Return lines of the stdout
    '''
    if netns:
        cmd = ['ip', 'netns', 'exec', netns, *cmd]

    if VERBOSITY > 1:
        print(shlex.join(cmd))

    try:
        proc = run(cmd, stdout=PIPE, stderr=PIPE, encoding='utf-8', check=True)
    except CalledProcessError as error:
        if VERBOSITY:
            print(error.output, end='')
            print(error.stderr, end='')
        if RAISE_ERROR:
            raise NetworkUtilsError(f'{error_message}: {error.stderr}') from error
        else:
            return error.output.splitlines()
    except Exception as e:
        if VERBOSITY:
            print(e)
        if RAISE_ERROR:
            raise
        else:
            return []
    else:
        if VERBOSITY:
            print(proc.stdout, end='')
            print(proc.stderr, end='')
        return proc.stdout.splitlines()


def create_netns(netns):
    __exec_cmd(
        'Netns creation failed',
        'ip', 'netns', 'add', netns)


def delete_netns(netns):
    __exec_cmd(
        'Netns deletion failed',
        'ip', 'netns', 'del', netns)


def exist_netns(netns):
    output = __exec_cmd(
        'Netns showing failed',
        'ip', 'netns', 'show', netns)

    for line in output:
        match = re.search('([a-zA-Z0-9_\-]+) .*', line)
        if match and match.group(1) == netns:
            return True
    return False


def list_netns():
    return __exec_cmd(
        'Netns listing failed',
        'ip', 'netns', 'list')


def list_ifaces(netns=None):
    output = __exec_cmd(
        'Interface listing failed',
        'ip', 'link', 'list',
        netns=netns)

    ifaces = []
    for line in output:
        match = re.search('([a-zA-Z0-9_\-]+): <.*', line)
        if match:
            ifaces.append(match.group(1))
    return ifaces


def set_up(iface, netns=None):
    __exec_cmd(
        'Setting up interface failed',
        'ip', 'link', 'set', iface, 'up',
        netns=netns)


def set_mac_address(iface, mac_address, netns=None):
    __exec_cmd(
        'MAC address setting failed',
        'ip', 'link', 'set', iface, 'address', mac_address,
        netns=netns)


def flush_address(iface, netns=None):
    __exec_cmd(
        'Addresses flush failed',
        'ip', 'address', 'flush', 'dev', iface,
        netns=netns)


def add_address(iface, address, netns=None):
    __exec_cmd(
        'Address addition failed',
        'ip', 'address', 'add', address, 'dev', iface,
        netns=netns)


def create_dummy_iface(iface, netns=None):
    __exec_cmd(
        'Dummy interface creation failed',
        'ip', 'link', 'add', 'name', iface, 'type', 'dummy',
        netns=netns)


def create_veth_ifaces_pair(iface, peeriface):
    __exec_cmd(
        'Veth interfaces pair creation failed',
        'ip', 'link', 'add', 'name', iface, 'type', 'veth', 'peer', 'name', peeriface)


def move_iface(iface, netns):
    __exec_cmd(
        'Interface moving to netns failed',
        'ip', 'link', 'set', iface, 'netns', netns)


def create_tap_iface(iface, netns=None):
    __exec_cmd(
        'Tap interface creation failed',
        'ip', 'tuntap', 'add', 'mode', 'tap', iface,
        netns=netns)


def create_bridge(bridge, ifaces, netns=None):
    __exec_cmd(
        'Bridge interface creation failed',
        'ip', 'link', 'add', 'name', bridge, 'type', 'bridge',
        netns=netns)

    for iface in ifaces:
        attach_iface(bridge, iface, netns)

    __exec_cmd(
        'Setting bridge ageing failed',
        'brctl', 'setageing', bridge, '0',
        netns=netns)


def attach_iface(bridge, iface, netns=None):
    __exec_cmd(
        'Attaching iface to bridge failed',
        'ip', 'link', 'set', iface, 'master', bridge,
        netns=netns)


def delete_iface(iface, netns=None):
    __exec_cmd(
        'Interface deletion failed',
        'ip', 'link', 'del', 'dev', iface,
        netns=netns)


def add_default_route(iface=None, gateway=None, netns=None):
    if not (iface or gateway):
        raise TypeError('Invalid parameters for default route addition')

    via = ['via', gateway] if gateway else []
    dev = ['dev', iface] if iface else []
    __exec_cmd(
        'Default route addition failed',
        'ip', 'route', 'add', 'default', *via, *dev,
        netns=netns)


def add_route(destination, iface=None, gateway=None, netns=None):
    if not (iface or gateway):
        raise ValueError('Invalid parameters for route addition')

    via = ['via', gateway] if gateway else []
    dev = ['dev', iface] if iface else []
    __exec_cmd(
        'Route addition to {} failed'.format(destination),
        'ip', 'route', 'add', destination, *via, *dev,
        netns=netns)


def enable_ipv4_forward(netns=None):
    __exec_cmd(
        'IPv4 forward enabling failed',
        'sysctl', 'net.ipv4.ip_forward=1',
        netns=netns)


def enable_ipv4_forwarding(iface='all', netns=None):
    __exec_cmd(
        'Interface IPv4 forwarding enabling failed',
        'sysctl', 'net.ipv4.conf.{}.forwarding=1'.format(iface),
        netns=netns)


def enable_ipv6_forward(netns=None):
    __exec_cmd(
        'IPv6 forward enabling failed',
        'sysctl', 'net.ipv6.ip_forward=1',
        netns=netns)


def enable_ipv6_forwarding(iface='all', netns=None):
    __exec_cmd(
        'Interface IPv6 forwarding enabling failed',
        'sysctl', 'net.ipv6.conf.{}.forwarding=1'.format(iface),
        netns=netns)


def enable_ipv4_proxy_arp(iface='all', netns=None):
    __exec_cmd(
        'Proxy ARP enabling failed',
        'sysctl', 'net.ipv4.conf.{}.proxy_arp=1'.format(iface),
        netns=netns)

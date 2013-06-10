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

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
netlink.py - interface with netlink adresses and routes
"""

import netlink.route.capi as capi
import netlink.route.link as link
import netlink.core as netlink
import netlink.route.address as address

class NlError(netlink.NetlinkError):
    pass


# dissociate the Object exists error
class NlExists(netlink.NetlinkError):
    pass


class NlRoute(object):
    """Route object"""

    def __init__(self, iface_name):
        self._link = link.resolve(iface_name)
        self._sock = netlink.Socket()
        self._sock.connect(netlink.NETLINK_ROUTE)


    def add(self, dst, gw=None):
        """ add a new route """
        nh = capi.rtnl_route_nh_alloc()
        route = capi.rtnl_route_alloc()

        addr_dst = netlink.AbstractAddress(dst)
        ifidx = self._link.ifindex

        capi.rtnl_route_set_dst(route, addr_dst._nl_addr)
        capi.rtnl_route_nh_set_ifindex(nh, ifidx)
        if gw is not None:
            addr_gw = netlink.AbstractAddress(gw)
            capi.rtnl_route_nh_set_gateway(nh, addr_gw._nl_addr)
        capi.rtnl_route_add_nexthop(route, nh)
        ret = capi.rtnl_route_add(self._sock._sock, route, 0)
        if ret == -6:
            raise NlExists(ret)
        if ret < 0:
            raise NlError(ret)
        

    def delete(self, dst, gw=None, proto_kernel=False):
        """ delete a route """
        nh = capi.rtnl_route_nh_alloc()
        route = capi.rtnl_route_alloc()

        addr_dst = netlink.AbstractAddress(dst)
        ifidx = self._link.ifindex

        capi.rtnl_route_set_dst(route, addr_dst._nl_addr)
        capi.rtnl_route_nh_set_ifindex(nh, ifidx)
        if gw is not None:
            addr_gw = netlink.AbstractAddress(gw)
            capi.rtnl_route_nh_set_gateway(nh, addr_gw._nl_addr)
        capi.rtnl_route_add_nexthop(route, nh)
        if proto_kernel:
            capi.rtnl_route_set_protocol(route, 2) # RTPROT_KERNEL = 2
        ret = capi.rtnl_route_delete(self._sock._sock, route, 0)
        if ret == -6:
            raise NlExists(ret)
        if ret < 0:
            raise NlError(ret)


class NlInterfaces(object):
    """Interface handling object"""

    def __init__(self):
        self._sock = netlink.Socket()
        self._sock.connect(netlink.NETLINK_ROUTE)
        self._cache = link.LinkCache()
        self._cache.refill(self._sock)

    def add_address(self, addr, iface):
        """ add a new address """
        ifidx = self._cache[iface].ifindex
        ad = address.Address()
        ad.local = addr
        ad.ifindex = ifidx
        ret = capi.rtnl_addr_add(self._sock._sock, ad._rtnl_addr, 0)
        if ret == -6:
            raise NlExists(ret)
        if ret < 0:
            raise NlError(ret)

    def del_address(self, addr, iface):
        """ delete an address """
        ifidx = self._cache[iface].ifindex
        ad = address.Address()
        ad.local = addr
        ad.ifindex = ifidx
        ret = capi.rtnl_addr_delete(self._sock._sock, ad._rtnl_addr, 0)
        if ret == -6:
            raise NlExists(ret)
        if ret < 0:
            raise NlError(ret)

    def up(self, iface):
        """ set a link up """
        link = self._cache[iface]

        if 'up' in link.flags:
            raise NlExists(-6)
        else:
            link.flags = ['up']
        link.change()

    def down(self, iface):
        """ set a link up """
        link = self._cache[iface]

        link.flags = ['-up']
        link.change()


if __name__ == '__main__':
    ### ADDRESSES ###
    IFACES = NlInterfaces()
    try:
        IFACES.add_address('192.168.18.5/24','eth0')
    except NlError, msg:
        print "cannot add address:", msg
    else:
        print "Address added"

    try:
        IFACES.del_address('192.168.18.5/24','eth0')
    except NlError, msg:
        print "cannot remove address:", msg
    else:
        print "Address removed"

    try:
        IFACES.add_address('2001:660:6602:101::5/64','eth0')
    except NlError, msg:
        print "cannot add address:", msg
    else:
        print "Address added"

    try:
        IFACES.del_address('2001:660:6602:101::5/64','eth0')
    except NlError, msg:
        print "cannot remove address:", msg
    else:
        print "Address removed"
        
    try:
        IFACES.down('eth0')
        IFACES.up('eth0')
    except NlError, msg:
        print "cannot set iface up/down:", msg
    else:
        print "Address iface setted up and down"

    ### ROUTES ###
    ROUTE = NlRoute("eth0")
    try:
        ROUTE.add("192.168.18.0/24")
    except NlError, msg:
        print "cannot add route: ", msg
    else:
        print "Route added"

    try:
        ROUTE.delete("192.168.18.0/24")
    except NlError, msg:
        print "cannot delete route: ", msg
    else:
        print "Route removed"

    try:
        ROUTE.add("2001:660:6602::101:5")
    except NlError, msg:
        print "cannot add route: ", msg
    else:
        print "Route added"

    try:
        ROUTE.delete("2001:660:6602::101:5")
    except NlError, msg:
        print "cannot delete route: ", msg
    else:
        print "Route removed"


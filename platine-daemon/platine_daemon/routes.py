#!/usr/bin/env python 
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
#
#
# This file is part of the Platine testbed.
#
#
# Platine is free software : you can redistribute it and/or modify it under the
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
routes.py - Manage routes for Platine
"""


import threading
import logging
import pickle
import os
from ipaddr import IPNetwork
from platine_daemon.nl_utils import NlRoute, NlError, NlExists

#macros
LOGGER = logging.getLogger('PtDmon')
ROUTE_FILE = "/var/cache/platine-daemon/routes"


class PlatineRoutes(object):
    """ manage the routes for Platine """

    # the PlatineRoutes class attributes are shared between command and service
    # threads
    _routes_lock = threading.Lock()
    _route_hdl = None
    _routes_v4 = {} # the available host and IPv4 adresses
    _routes_v6 = {} # the available host and IPv6 adresses
    _started = False
    _initialized = False
    _iface = None

    def __init__(self):
        pass

    def load(self, iface):
        PlatineRoutes._routes_lock.acquire()
        if iface is not None:
            PlatineRoutes._route_hdl = NlRoute(iface)
            PlatineRoutes._iface = iface

        # read the routes file
        routes = {}
        try:
            route_file = open(ROUTE_FILE, 'rb')
        except IOError, (errno, strerror):
            LOGGER.debug("the route file '%s' cannot be read: "
                         "assume the process are stopped, "
                         "keep an empty route list" % ROUTE_FILE)
            LOGGER.debug("route list is initialized")
        else:
            try:
                routes = pickle.load(route_file)
            except EOFError:
                LOGGER.info("route file is empty")
                PlatineRoutes._started = False
                LOGGER.debug("route list is initialized")
            except pickle.PickleError, error:
                LOGGER.error("unable to load the route list: " + str(error))
            else:
                # find routes, assume the platform is started
                PlatineRoutes._started = True
                for host in routes:
                    PlatineRoutes._routes_v4[host] = routes[host][0]
                    PlatineRoutes._routes_v6[host] = routes[host][1]
                LOGGER.debug("route list is initialized")
            finally:
                route_file.close()

        PlatineRoutes._initialized = True

        PlatineRoutes._routes_lock.release()

    def is_initialized(self):
        """ is the route object initialized """
        return PlatineRoutes._initialized

    def add_distant_host(self, name, v4, v6):
        """ add a new distant host """
        PlatineRoutes._routes_lock.acquire()
        LOGGER.debug("new distant host %s with addresses %s and %s" %
                     (name, v4, v6))
        if not name in PlatineRoutes._routes_v4: 
            net = IPNetwork(v4)
            prefix_v4 = "%s/%s" % (net.network, net.prefixlen)
            PlatineRoutes._routes_v4[name] = prefix_v4
        if not name in PlatineRoutes._routes_v6: 
            net = IPNetwork(v4)
            prefix_v4 = "%s/%s" % (net.network, net.prefixlen)
            net = IPNetwork(v6)
            prefix_v6 = "%s/%s" % (net.network, net.prefixlen)
            PlatineRoutes._routes_v4[name] = prefix_v4
            PlatineRoutes._routes_v6[name] = prefix_v6

            if PlatineRoutes._started:
                LOGGER.debug("Platform is started, add a route for this host")
                try:
                    self.add_route(name, v4, v6)
                except (NlError, NlExists):
                    PlatineRoutes._routes_lock.release()
                    raise
                self.serialize()
        PlatineRoutes._routes_lock.release()

    def remove_distant_host(self, host):
        """ remove a distant host """
        PlatineRoutes._routes_lock.acquire()
        LOGGER.debug("remove distant host %s" % host)
        if PlatineRoutes._started:
            LOGGER.debug("Platform is started, remove the route for this host")
            v4 = None
            v6 = None
            try:
                v4 = PlatineRoutes._routes_v4[host]
            except KeyError:
                pass
            try:
                v6 = PlatineRoutes._routes_v6[host]
            except KeyError:
                pass
            try:
                self.remove_route(host, v4, v6),
            except NlError:
                pass
            if v4:
                del PlatineRoutes._routes_v4[host]
            if v6:
                del PlatineRoutes._routes_v6[host]
            self.serialize()
        else:
            del PlatineRoutes._routes_v4[host]
            del PlatineRoutes._routes_v6[host]
        PlatineRoutes._routes_lock.release()

    def setup_routes(self):
        """ apply the routes when started """
        PlatineRoutes._routes_lock.acquire()
        PlatineRoutes._started = True
        LOGGER.info("set route before starting platform")
        self.serialize()
        for host in set(PlatineRoutes._routes_v4.keys() +
                        PlatineRoutes._routes_v6.keys()):
            v4 = None
            v6 = None
            try:
                v4 = PlatineRoutes._routes_v4[host]
            except KeyError:
                pass
            try:
                v6 = PlatineRoutes._routes_v6[host]
            except KeyError:
                pass
            try:
                self.add_route(host, v4, v6),
            except (NlError, NlExists):
                PlatineRoutes._routes_lock.release()
                raise
        PlatineRoutes._routes_lock.release()

    def remove_routes(self):
        """ remove the current routes when stopped """
        PlatineRoutes._routes_lock.acquire()
        PlatineRoutes._started = False
        LOGGER.info("remove route after stopping platform")
        for host in set(PlatineRoutes._routes_v4.keys() +
                        PlatineRoutes._routes_v6.keys()):
            v4 = None
            v6 = None
            try:
                v4 = PlatineRoutes._routes_v4[host]
            except KeyError:
                pass
            try:
                v6 = PlatineRoutes._routes_v6[host]
            except KeyError:
                pass
            try:
                self.remove_route(host, v4, v6)
            except NlError:
                pass
        try:
            os.remove(ROUTE_FILE)
        except OSError:
            pass
        PlatineRoutes._routes_lock.release()

    def add_route(self, host, route_v4, route_v6):
        """ add a new route """
        LOGGER.info("add routes for host %s toward %s and %s via %s" %
                    (host, route_v4, route_v6, PlatineRoutes._iface))
        try:
            if route_v4:
                PlatineRoutes._route_hdl.add(route_v4)
        except NlExists:
            LOGGER.info("route already exists on %s" % host)
            del PlatineRoutes._routes_v4[host]
        except NlError, msg:
            LOGGER.error("fail to add IPv4 route for %s: %s" % (host, msg))
            # remove host from routes to avoid deleting it on stop
            del PlatineRoutes._routes_v4[host]
            raise
        try:
            if route_v6:
                PlatineRoutes._route_hdl.add(route_v6)
        except NlExists:
            LOGGER.info("route already exists on %s" % host)
            del PlatineRoutes._routes_v6[host]
        except NlError, msg:
            LOGGER.error("fail to add IPv6 route for %s: %s" % (host, msg))
            # remove host from routes to avoid deleting it on stop
            del PlatineRoutes._routes_v6[host]
            raise

    def remove_route(self, host, route_v4, route_v6):
        """ remove a route """
        LOGGER.info("remove route for host %s toward %s and %s via %s" %
                    (host, route_v4, route_v6, PlatineRoutes._iface))
        try:
            if route_v4:
                PlatineRoutes._route_hdl.delete(route_v4)
        except NlError, msg:
            LOGGER.error("fail to delete route for %s: %s" % (host, msg))
            raise
        finally:
            # try to remove the IPv6 route anyway
            try:
                if route_v6:
                    PlatineRoutes._route_hdl.delete(route_v6)
            except NlError, msg:
                LOGGER.error("fail to delete route for %s: %s" % (host, msg))
                raise


    def serialize(self):
        """ serialize the routes in order to keep them in case
            of daemon restart """
        routes = {}
        for host in set(PlatineRoutes._routes_v4.keys() +
                        PlatineRoutes._routes_v6.keys()):
            v4 = None
            v6 = None
            try:
                v4 = PlatineRoutes._routes_v4[host]
            except KeyError:
                pass
            try:
                v6 = PlatineRoutes._routes_v6[host]
            except KeyError:
                pass
            routes[str(host)] = (v4, v6)
        if len(routes) == 0:
            return
        try:
            route_file = open(ROUTE_FILE, 'wb')
            pickle.dump(routes, route_file)
        except IOError, (errno, strerror):
            LOGGER.error("unable to create %s file (%d: %s)" % (ROUTE_FILE,
                          errno, strerror))
        except pickle.PickleError, error:
            LOGGER.error("unable to serialize route list: " + str(error))
            route_file.close()
        else:
            route_file.close()



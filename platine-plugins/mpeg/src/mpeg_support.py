#!/usr/bin/env python 
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2012 CNES
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
mpeg_support.py - Add MPEG2-TS support to a Platine daemon
"""
import os
import sys
import ConfigParser
import debconf

PLUGIN_NAME = 'MPEG2-TS'
CONF_FILENAME = '/etc/platine/daemon.conf'

if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] not in ("add", "remove"):
        print "USAGE: %s add/remove" % sys.argv[0]
        sys.exit(1)

    parser = ConfigParser.SafeConfigParser()

    if len(parser.read(CONF_FILENAME)) == 0:
        # if the daemon is not installed (eg. if there is only the manager on the host)
        print 'ERROR: cannot read configuration file ' + CONF_FILENAME
        sys.exit(1)

    plugins = ''
    try:
        plugins = parser.get('service', 'modules')
    except ConfigParser.Error, msg:
        pass

    try:
        os.environ['DEBIAN_HAS_FRONTEND']
        db = debconf.Debconf()
        db_plugins = db.get("platine-daemon/service/modules")
    except debconf.DebconfError, (val, err):
        print str("Debconf error: " + err)
        sys.exit(1)
    except:
        os.execv(debconf._frontEndProgram, [debconf._frontEndProgram] + sys.argv)

    if sys.argv[1] == "add":
        if plugins.find(PLUGIN_NAME) != -1:
            print "Plugin already supported by daemon"
            sys.exit(1)
        try:
            plugins = plugins + " " + PLUGIN_NAME
            parser.set('service', 'modules', plugins)
        except ConfigParser.Error, msg:
            print 'ERROR: cannot set plugins in configuration file (%s)' % msg
            sys.exit(1)

        if db_plugins.find(PLUGIN_NAME) == -1:
            db_plugins = db_plugins + " " + PLUGIN_NAME
    else:
        try:
            plugins = plugins.replace(PLUGIN_NAME, "")
            if plugins.isspace() or plugins == '':
                parser.remove_option('service', 'modules')
            else:
                parser.set('service', 'modules', plugins)
        except ConfigParser.Error, msg:
            print 'ERROR: cannot set plugins in configuration file (%s)' % msg
            sys.exit(1)

        if db_plugins.find(PLUGIN_NAME) != -1:
            db_plugins = db_plugins.replace(PLUGIN_NAME, "")

    # modify debconf database
    try:
        db.set("platine-daemon/service/modules", db_plugins)
    except debconf.DebconfError, (val, err):
        print str("Debconf error: " + err)
        sys.exit(1)

    # write the configuration file
    with open(CONF_FILENAME, 'wb') as config_file:
        parser.write(config_file)

    sys.exit(0)



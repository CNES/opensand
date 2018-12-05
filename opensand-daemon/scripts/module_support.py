#!/usr/bin/env python2 
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2018 CNES
# Copyright © 2018 TAS
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
module_support.py - Add a module support to a OpenSAND daemon
"""
import os
import sys
import ConfigParser
import debconf

DAEMON_BIN = '/usr/bin/sand-daemon'
CONF_FILENAME = '/etc/opensand/daemon.conf'

if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[1] not in ("add", "remove"):
        print "USAGE: %s add/remove module_name" % sys.argv[0]
        sys.exit(1)

    parser = ConfigParser.SafeConfigParser()
    module_name = sys.argv[2]
    conf_file = False

    if not os.path.isfile(DAEMON_BIN):
        # if the daemon is not installed (eg. if there is only the manager on the host)
        sys.exit(1)

    # try to read the conf file (may be inexistant if the daemon was never launched)
    if len(parser.read(CONF_FILENAME)) > 0:
        conf_file = True

    plugins = ''
    try:
        plugins = parser.get('service', 'modules')
    except ConfigParser.Error, msg:
        pass

    try:
        os.environ['DEBIAN_HAS_FRONTEND']
        db = debconf.Debconf()
        db_plugins = db.get("opensand-daemon/service/modules")
    except debconf.DebconfError, (val, err):
        print str("Debconf error: " + err)
        sys.exit(1)
    except:
        os.execv(debconf._frontEndProgram, [debconf._frontEndProgram] + sys.argv)

    if sys.argv[1] == "add":
        if plugins.find(module_name) != -1:
            print "Plugin already supported by daemon"
            sys.exit(1)
        try:
            plugins = plugins + " " + module_name
            if conf_file:
                parser.set('service', 'modules', plugins)
        except ConfigParser.Error, msg:
            print 'ERROR: cannot set plugins in configuration file (%s)' % msg
            sys.exit(1)

        if db_plugins.find(module_name) == -1:
            db_plugins = db_plugins + " " + module_name
    else:
        try:
            plugins = plugins.replace(module_name, "")
            if plugins.isspace() or plugins == '':
                parser.remove_option('service', 'modules')
            elif conf_file:
                parser.set('service', 'modules', plugins)
        except ConfigParser.Error, msg:
            print 'ERROR: cannot set plugins in configuration file (%s)' % msg
            sys.exit(1)

        if db_plugins.find(module_name) != -1:
            db_plugins = db_plugins.replace(module_name, "")

    # modify debconf database
    try:
        db.set("opensand-daemon/service/modules", db_plugins)
    except debconf.DebconfError, (val, err):
        print str("Debconf error: " + err)
        sys.exit(1)

    # write the configuration file
    with open(CONF_FILENAME, 'wb') as config_file:
        parser.write(config_file)

    sys.exit(0)


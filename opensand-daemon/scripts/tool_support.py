#!/usr/bin/env python2 
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 CNES
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
tool_support.py - Add a tool support to a OpenSAND daemon
"""
import os
import sys
import ConfigParser
import debconf

CONF_FILENAME = '/etc/opensand/daemon.conf'
# TODO we cannot use this with HOST like that, this should be a parameter !
HOST=""

if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[1] not in ("add", "remove"):
        print "USAGE: %s add/remove tool_name" % sys.argv[0]
        sys.exit(1)

    parser = ConfigParser.SafeConfigParser()
    tool_name = sys.argv[2]

    if len(parser.read(CONF_FILENAME)) == 0:
        # if the daemon is not installed (eg. if there is only the manager on the host)
        print 'ERROR: cannot read configuration file ' + CONF_FILENAME
        sys.exit(1)

    try:
        host_name = parser.get('service', 'name')
        if host_name not in HOST:
            print "ERROR: this host does not support the tool"
            sys.exit(1)
    except ConfigParser.Error, msg:
        print 'ERROR: cannot get host name in configuration file (%s)' % msg


    tools = ''
    try:
        tools = parser.get('service', 'tools')
    except ConfigParser.Error, msg:
        pass

    try:
        os.environ['DEBIAN_HAS_FRONTEND']
        db = debconf.Debconf()
        db_tools = db.get("opensand-daemon/service/tools")
    except debconf.DebconfError, (val, err):
        print str("Debconf error: " + err)
        sys.exit(1)
    except:
        os.execv(debconf._frontEndProgram, [debconf._frontEndProgram] + sys.argv)

    if sys.argv[1] == "add":
        if tools.find(tool_name) != -1:
            print "Tool already supported by daemon"
            sys.exit(1)
        try:
            tools = tools + " " + tool_name
            parser.set('service', 'tools', tools)
        except ConfigParser.Error, msg:
            print 'ERROR: cannot set tools in configuration file (%s)' % msg
            sys.exit(1)

        if db_tools.find(tool_name) == -1:
            db_tools = db_tools + " " + tool_name
    else:
        try:
            tools = tools.replace(tool_name, "")
            if tools.isspace() or tools == '':
                parser.remove_option('service', 'tools')
            else:
                parser.set('service', 'tools', tools)
        except ConfigParser.Error, msg:
            print 'ERROR: cannot set tools in configuration file (%s)' % msg
            sys.exit(1)

        if db_tools.find(tool_name) != -1:
            db_tools = db_tools.replace(tool_name, "")

    # modify debconf database
    try:
        db.set("opensand-daemon/service/tools", db_tools)
    except debconf.DebconfError, (val, err):
        print str("Debconf error: " + err)
        sys.exit(1)

    # write the configuration file
    with open(CONF_FILENAME, 'wb') as config_file:
        parser.write(config_file)

    sys.exit(0)


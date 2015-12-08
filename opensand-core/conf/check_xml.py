#!/usr/bin/env python2 
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2015 TAS
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
test.py - check the XML configuration with the corresponding XSD file
"""


from lxml import etree
import sys

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Usage: %s xml_file xsd_file" % sys.argv[0]
        sys.exit(1)
    print "use schema: %s, xml: %s" % (sys.argv[2], sys.argv[1])
    schema = etree.XMLSchema(etree.parse(sys.argv[2]))
    doc = etree.parse(sys.argv[1])
    if not schema.validate(doc):
        print "ERRORs:"
        print schema.error_log
        sys.exit(1)
    else:
        print "OK"
        sys.exit(0)



#!/usr/bin/env python 
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

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



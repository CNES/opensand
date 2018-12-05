#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
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
# Author: Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>

"""
generate_conf_wiki.py - Transform the configuration into a wiki readable page
"""

from opensand_manager_core.utils import SPOT
from opensand_manager_core.opensand_xml_parser import XmlParser

DO_HIDE=False

def line_title():
    line = "^  {:>20}  ^  {:>40}  ^  {:>20}  ^  {:>20}  ^ {:>30}  " \
           "^  {:>20}  ^  {:>20}  ^  {:>20}  ^  {:>20}  ^"
    return line.format("Domain", "Description", "By spot", "Parameter type", 
                       "Parameter name", "Value type", "Default value", 
                       "Range of values", "Purpose")


def line(domain, description, bspot, ptype, pname,  vtype, vdefault, vrange, purpose):
    line = "|  {:>20}  |  {:>40}  |  {:>20}  |  {:>20}  | {:>30}  " \
           "|  {:>20}  |  {:>20}  |  {:>20}  |  {:>20}  |"
    if description is None:
        description = "-"
    if bspot is None:
        bspot = ""
    if ptype is None:
        ptype = "-"
    if vtype is None:
        vtype = "-"
    if vdefault is None:
        vdefault = "-"
    if vrange is None:
        vrange = "-"
    if purpose is None:
        purpose = "-"
    description = description.replace('\n', '\\\\')
    description = description.replace('\t', '    ')
    purpose = purpose.replace('\n', '\\\\')
    purpose = purpose.replace('\t', '    ')
    
    return line.format(domain, description, bspot, ptype, pname,
                       vtype, vdefault, vrange, purpose)
# For debug
#    return line.format(domain[:20], description[:40], bspot[:20], ptype[:20],
#                       pname[:30], vtype[:20], vdefault[:20], vrange[:20],
#                       purpose[:20])

def print_line (domain, description, bspot, KEY):
    ptype = ""
    pname = ""
    vtype = ""
    vdefault = ""
    vrange = ""
    purpose = ""
    
    if not PARSER.is_table(KEY):
        ptype = "PARAM"
        pname = PARSER.get_name(KEY)
        vdefault = PARSER.get_value(KEY)
        unit = PARSER.get_unit(pname)
        if vdefault is not None and unit is not None:
            vdefault += " " + unit
        vtype = param_type(PARSER.get_type(pname))
        vrange = param_range(PARSER.get_type(pname))
        purpose = PARSER.get_documentation(pname)

        print line(domain, description, bspot, ptype, pname,  vtype,
                   vdefault, vrange, purpose)
        domain = ":::"
        description = ":::"
        bspot = ":::" 
    else:
        ptype = "TABLE of %s" % PARSER.get_name(KEY)
        for ELT in PARSER.get_table_elements(KEY):
            for (pname, vdefault) in PARSER.get_element_content(ELT).items():
                elt_name = PARSER.get_name(ELT)
                unit = PARSER.get_unit(pname, elt_name)
                if vdefault is not None and unit is not None:
                    vdefault += " " + unit
                att_type = PARSER.get_attribute_type(pname, elt_name)
                purpose = PARSER.get_documentation(pname, elt_name)
                vtype = param_type(att_type)
                vrange = param_range(att_type)
                print line(domain, description, bspot, ptype, pname, 
                           vtype, vdefault, vrange, purpose)
                ptype = ":::"
                domain = ":::"
                description = ":::"
                bspot = ":::" 
                # only one line is necessary
            break


def param_type(vtype):
    val = None
    if vtype is not None:
        val = vtype['type']
    if val == "numeric":
        val = "integer"
        if vtype['step'] < 1:
            val = "decimal"
    return val

def param_range(vtype):
    if vtype is None:
        return None
    prange = ""
    if vtype['type'] == "enum":
        for val in vtype['enum']:
            prange += val + "\\\\ "
    elif vtype['type'] == "numeric":
        if "min" in vtype:
            prange += ">= %s\\\\ " % vtype['min']
        if "max" in vtype:
            prange += "<= %s\\\\ " % vtype['max']

    prange = prange.rstrip(" ")
    prange = prange.rstrip("\\")
    if prange == "":
        prange = None
    return prange

if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print "Usage: %s xml_file xsd_file" % sys.argv[0]
        sys.exit(1)

    PARSER = XmlParser(sys.argv[1], sys.argv[2])
    
    print line_title()

    domain = ""
    description = ""
    bspot = ""
    for SECTION in PARSER.get_sections():
        domain = PARSER.get_name(SECTION)
        done = False
        if DO_HIDE and (PARSER.do_hide(domain) or PARSER.do_hide_adv(domain,
                                                                    False)):
            continue
        description = PARSER.get_documentation(domain)
        bspot = ""
        try:
            description = description.split('>')[1].strip("</b>")
        except:
            pass
        for KEY in PARSER.get_keys(SECTION):
            if DO_HIDE and PARSER.do_hide(PARSER.get_name(KEY)) or \
               PARSER.do_hide_adv(PARSER.get_name(KEY), False):
                continue
            if PARSER.get_name(KEY) == SPOT and not done:
                bspot = "X"  
                for PARAM in PARSER.get_keys(KEY):
                    print_line(domain, description, bspot, PARAM)
                    domain = ":::"
                    description = ":::"
                    bspot = ":::"
                done = True
            else:        
                print_line(domain, description, bspot, KEY)
            
            domain = ":::"
            description = ":::"
            bspot = ":::"

    sys.exit(0)




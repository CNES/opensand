#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
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
opensand_xml_parser.py - the XML parser and builder for OpenSAND configuration

  The OpenSAND configuration XML format is:
  <?xml version="1.0" encoding="UTF-8"?>
  <configuration component='COMPO'>
    <SECTION>
      <KEY>VAL</KEY>
      <TABLES>
        <TABLE ATTRIBUTE1="VAL1" ATTRIBUTE2="VAL2" />
        <TABLE ATTRIBUTE1="VAL1'" ATTRIBUTE2="VAL2'" />
      </TABLES>
    </SECTION>
  </configuration>
"""

from copy import deepcopy

from lxml import etree
from opensand_manager_core.my_exceptions import XmlException


NAMESPACES = {"xsd":"http://www.w3.org/2001/XMLSchema"}

COMMON_XSD = "/usr/share/opensand/common.xsd"

class XmlParser:
    """ XML parser for OpenSAND configuration """
    def __init__ (self, xml, xsd):
        self._tree = None
        self._filename = xml
        self._xsd = xsd
        self._xsd_parser = None
        self._common_xsd = None
        self._schema = None
        try:
            self._tree = etree.parse(xml)
            self._schema = etree.XMLSchema(etree.parse(xsd))
            self._xsd_parser = etree.parse(xsd)
            self._common_xsd = etree.parse(COMMON_XSD)
        except IOError, err:
            raise
        except (etree.XMLSyntaxError, etree.XMLSchemaParseError), err:
            raise XmlException(err.error_log.last_error.message)

        if not self._schema.validate(self._tree):
            raise XmlException(self._schema.error_log.last_error.message)

        root = self._tree.getroot()
        if(root.tag != "configuration"):
            raise XmlException("Not a configuration file, root element is %s" %
                               root.tag)

    def get_sections(self):
        """ get all the sections in the configuration file """
        root = self._tree.getroot()
        return [sect for sect in root.iterchildren()
                     if sect.tag is not etree.Comment]

    def get_name(self, node):
        """ get the name of any XML node """
        return node.tag

    def get_keys(self, section):
        """ get all the keys in a section """
        return [key for key in section.iterchildren()
                    if key.tag is not etree.Comment]

    def is_table(self, key):
        """ check if the key is a table one """
        return len(key) > 0

    def get_value(self, key):
        """ get the value stored in a key """
        if self.is_table(key):
            raise XmlException("Key is a table, cannot get its value")
        return key.text

    def get_table_elements(self, key):
        """ get the list of element in a table """
        return [elt for elt in key.iterchildren()
                    if elt.tag is not etree.Comment]

    def get_element_content(self, element):
        """ get a dictionnary containing the attributes and values of a list
            element """
        return element.attrib

    def get_path(self, elt):
        """ get the xpath of an XML attribute or key """
        return self._tree.getpath(elt)

    def set_value(self, val, path, att=None):
        """ set the value of a key """
        elt = self._tree.xpath(path)
        if len(elt) != 1:
            raise XmlException("wrong path %s" % path)

        if att is not None:
            if not self.is_table(elt):
                raise XmlException("wrong path: %s is not a table" % elt.tag)
            att_list = self.get_element_content(elt[0])
            if not att in att_list:
                raise XmlException("wrong path: %s is not a valid attribute" %
                                   att)
            att_list[att] = str(val)
        else:
            elt[0].text = str(val)

    def set_values(self, val, xpath, attribute):
        """ set a value on one or more paths """
        elts = self.get_all(xpath)
        if len(elts) < 1:
            raise XmlException("Cannot find %s path" % xpath)

        for elem in elts:
            att_list = self.get_element_content(elem)
            if not attribute in att_list:
                raise XmlException("wrong path: %s is not a valid attribute" %
                                   attribute)
            att_list[attribute] = val

    def get(self, xpath):
        """ get a XML element with its path """
        elts = self._tree.xpath(xpath)
        if len(elts) == 1:
            return elts[0]
        return None

    def get_all(self, xpath):
        """ get all matching elements """
        return self._tree.xpath(xpath)

    def del_element(self, xpath):
        """ delete an element from its path """
        elts = self._tree.xpath(xpath)
        if len(elts) == 1:
            elt = elts[0]
            elt.getparent().remove(elt)

    def add_line(self, xpath):
        """ add a line in the table identified its path """
        tables = self._tree.xpath(xpath)
        if len(tables) != 1:
            raise XmlException("wrong path: %s is not valid" % xpath)
        table = tables[0]
        children = table.getchildren()
        if len(children) == 0:
            raise XmlException("wrong path: %s is not a table" % xpath)
        i = 0
        # copy the first line for the base, but skip comments
        while(i < len(children)):
            child = children[i]
            i += 1
            if not child.tag is etree.Comment:
                break
        new = deepcopy(child)
        for att in new.attrib.keys():
            new.attrib[att] = ''
        table.append(new)

    def create_line(self, attributes, key, xpath):
        """ create a new line in a table """
        table = self.get(xpath)
        if table is None:
            raise XmlException("wrong path: %s is not valid" % xpath)
        table.append(etree.Element(key, attributes))

    def remove_line(self, xpath):
        """ remove a line in the table identified by its path """
        tables = self._tree.xpath(xpath)
        if len(tables) != 1:
            raise XmlException("wrong path: %s is not valid" % xpath)
        table = tables[0]
        children = table.getchildren()
        if len(children) == 0:
            raise XmlException("wrong path: %s is not a table" % xpath)
        child = children[0]
        table.remove(child)

    def write(self, filename=None):
        """ write the new configuration in file """
        if not self._schema.validate(self._tree):
            error = self._schema.error_log.last_error.message
            self.__init__(self._filename, self._xsd)
            raise XmlException("the new values are incorrect: %s" %
                               error)

        if filename is  None:
            filename = self._filename
        with open(filename, 'w') as conf:
            conf.write(etree.tostring(self._tree, pretty_print=True,
                                      encoding=self._tree.docinfo.encoding,
                                      xml_declaration=True))

    def get_files(self):
        """ get the file elements and their value """
        files = {}
        # get elements of type file
        nodes = self.get_file_elements("element")
        for node in nodes:
            elems = self.get_all("//%s" % node)
            for elem in elems:
                files["%s/text()" % self.get_path(elem)] = self.get_value(elem)
        # get attributes of type file
        nodes = self.get_file_elements("attribute")
        for node in nodes:
            elems = self.get_all("//*[@%s]" % node)
            for elem in elems:
                path = self.get_path(elem)
                files["%s/@%s" % (path, node)] = \
                        self._tree.xpath("%s/@%s" % (path, node))[0]
        return files

    #### functions for XSD parsing ###

    def get_type(self, name):
        """ get an element type in the XSD document """
        elem = self.get_element(name, True)
        if elem is None:
            return None

        elem_type = elem.get("type")
        if elem_type is None:
            return None

        if elem_type.startswith("xsd:"):
            ret = self.parse_numeric(elem_type)
            if ret is not None:
                return ret
            else:
                return {"type": elem_type.replace("xsd:", "", 4)}
        elif elem_type == "file":
            return {"type": "file"}
        else:
            return self.get_simple_type(elem_type)

    def get_attribute_type(self, name, parent_name):
        """ get an attribute type in the XSD document """
        attribute = self.get_attribute(name, parent_name)
        if attribute is None:
            return None

        att_type = attribute.get("type")
        if att_type is None:
            return None

        if att_type.startswith("xsd:"):
            ret = self.parse_numeric(att_type)
            if ret is not None:
                return ret
            else:
                return {"type": att_type.replace("xsd:", "", 4)}
        elif att_type == "file":
            return {"type": "file"}
        else:
            return self.get_simple_type(att_type)

    def get_documentation(self, name, parent_name=None):
        """ get the description associated to an element """
        if parent_name is not None:
            elem = self.get_attribute(name, parent_name)
        else:
            elem = self.get_element(name)

        if elem is None:
            return None

        doc = elem.xpath("xsd:annotation/xsd:documentation",
                         namespaces=NAMESPACES)
        if len(doc) != 1:
            # search in references
            elem = self.get_reference(name)
            if elem is not None:
                doc = elem.xpath("xsd:annotation/xsd:documentation",
                                 namespaces=NAMESPACES)
            if len(doc) != 1:
                return None

        text = doc[0].text
        # remove indentation
        text = ' '.join(text.split())
        # set tabulations and newlines
        text = text.replace('\\n', '\n').replace('\\t', '\t')
        return text

    def get_unit(self, name, parent_name=None):
        """ get the unit of an element """
        if parent_name is not None:
            elem = self.get_attribute(name, parent_name)
        else:
            elem = self.get_element(name)

        if elem is None:
            return None

        unit = elem.xpath("xsd:annotation/xsd:documentation/unit",
                          namespaces=NAMESPACES)
        if len(unit) != 1:
            # search in references
            elem = self.get_reference(name)
            if elem is not None:
                unit = elem.xpath("xsd:annotation/xsd:documentation/unit",
                                  namespaces=NAMESPACES)
            if len(unit) != 1:
                return None

        text = unit[0].text
        # remove indentation
        text = ' '.join(text.split())
        # set tabulations and newlines
        text = text.replace('\\n', '\n').replace('\\t', '\t')
        return text

    def do_hide_dev(self, name, dev_mode):
        """ check if some widget should be hidden in non dev mode """
        # check if element should be hidden in non dev_mode
        elem = self.get_element(name)
        if elem is None:
            return False
        dev = elem.xpath("xsd:annotation/xsd:documentation/dev",
                         namespaces=NAMESPACES)
        if len(dev) != 1:
            # search in references
            elem = self.get_reference(name)
            if elem is not None:
                dev = elem.xpath("xsd:annotation/xsd:documentation/dev",
                                 namespaces=NAMESPACES)
        if len(dev) == 1 and not dev_mode:
            val = dev[0].text
            if val == "true":
                return True

        return False

    def do_hide(self, name):
        """ check if som widget should be hidden """
        # check if element has a hide parameter activated
        elem = self.get_element(name)
        if elem is None:
            return False
        hide = elem.xpath("xsd:annotation/xsd:documentation/hide",
                          namespaces=NAMESPACES)
        if len(hide) != 1:
            # search in references
            elem = self.get_reference(name)
            if elem is not None:
                hide = elem.xpath("xsd:annotation/xsd:documentation/hide",
                                  namespaces=NAMESPACES)
        if len(hide) == 1:
            val = hide[0].text
            if val == "true":
                return True

        return False

    def get_reference(self, name):
        """ get a reference in the XSD document """
        elem = []
        elem += self._common_xsd.xpath("//xsd:element[@ref = $val]",
                                       namespaces=NAMESPACES,
                                       val = name)
        elem += self._xsd_parser.xpath("//xsd:element[@ref = $val]",
                                       namespaces=NAMESPACES,
                                       val = name)
        if len(elem) == 0:
            return None

        # take the first element it should be better than nothing
        return elem[0]

    def get_element(self, name, with_type=False):
        """ get an element in the XSD document """
        elems = []
        elems += self._common_xsd.xpath("//xsd:element[@name = $val]",
                                        namespaces=NAMESPACES,
                                        val = name)
        elems += self._xsd_parser.xpath("//xsd:element[@name = $val]",
                                        namespaces=NAMESPACES,
                                        val = name)

        # sometimes there are 2 elements because debug keys got the same name,
        # take the first one with or without type
        if len(elems) == 1:
            return elems[0]
        elif len(elems) != 0:
            for elem in elems:
                if elem.get('type') is None and not with_type:
                    return elem
                elif elem.get('type') is not None and with_type:
                    return elem
            # return the first one, it should be better than nothing
            return elems[0]
        return None

    def get_attribute(self, name, parent_name):
        """ get an attribute in the XSD document """
        attribs = []
        attribs += self._common_xsd.xpath("//xsd:attribute[@name = $val]",
                                         namespaces=NAMESPACES,
                                         val = name)
        attribs += self._xsd_parser.xpath("//xsd:attribute[@name = $val]",
                                          namespaces=NAMESPACES,
                                          val = name)
        if attribs is None or len(attribs) == 0:
            return None

        # some elements can have the same attribute, get the attribute for the
        # desired element
        # first check if parent has a complex type
        cplx_name = self.get_complex_name(parent_name)
        if cplx_name is not None:
            parent_name = cplx_name
        for attrib in attribs:
            if attrib.getparent().getparent().get("name") == parent_name:
                return attrib

    def get_complex_name(self, name):
        """ get a completType element in the XSD document """
        elem = []
        elem += self._common_xsd.xpath("//xsd:element[@name = $val]",
                                       namespaces=NAMESPACES,
                                       val = name)
        elem += self._xsd_parser.xpath("//xsd:element[@name = $val]",
                                       namespaces=NAMESPACES,
                                       val = name)
        if len(elem) == 0:
            return None

        # we have now the type of the complex element in the type attribute
        # take the first element it should be better than nothingi
        cplx_name = elem[0].get('type')
        return cplx_name

    def get_simple_type(self, name):
        """ get a simple type and the associated attributes
            in a XSD document """
        # simpleType
        elems = []
        elems += self._common_xsd.xpath("//xsd:simpleType[@name = $val]",
                                        namespaces=NAMESPACES,
                                        val = name)
        elems += self._xsd_parser.xpath("//xsd:simpleType[@name = $val]",
                                        namespaces=NAMESPACES,
                                        val = name)
        if len(elems) == 0:
            return None

        # take the first element it should be better than nothing
        elem = elems[0]

        # restriction
        restrictions = elem.xpath("xsd:restriction",
                                  namespaces=NAMESPACES)
        if len(restrictions) != 1:
            return None

        restriction = restrictions[0]

        base = restriction.get("base")
        if base is None:
            return None

        # TODO also handle long, double, float, etc..
        ret = self.parse_numeric(base)
        if ret is not None:
            # get the minimum or maximum value if specified
            min_inc = restriction.xpath("xsd:minInclusive",
                                        namespaces=NAMESPACES)
            if len(min_inc) == 1:
                ret["min"] = min_inc[0].get("value")
            max_inc = restriction.xpath("xsd:maxInclusive",
                                        namespaces=NAMESPACES)
            if len(max_inc) == 1:
                ret['max'] = max_inc[0].get("value")
            fraction_digit = restriction.xpath("xsd:fractionDigits",
                                               namespaces=NAMESPACES)
            if len(fraction_digit) == 1:
                ret['step'] = "0.%s1" % ("0" *
                                         (int(fraction_digit[0].get("value")) -
                                          1))
            return ret
        elif base == "xsd:string":
            enum = restriction.xpath("xsd:enumeration",
                                     namespaces=NAMESPACES)
            values = []
            if enum is not None and len(enum) > 0:
                for elem in enum:
                    values.append(elem.get("value"))
            if len(values) != 0:
                return {
                          "type": "enum",
                          "enum": values
                       }
            else:
                return {"type": "string"}
        else:
            return {"type": base.replace("xsd:", "", 4)}

    def get_minoccurs(self, table_name):
        """ get minOccurs value for table elements """
        values = []
        values += self._common_xsd.xpath("//xsd:element[@ref='%s']/@minOccurs" %
                                         table_name, namespaces=NAMESPACES)
        values += self._xsd_parser.xpath("//xsd:element[@ref='%s']/@minOccurs" %
                                         table_name, namespaces=NAMESPACES)
        values += self._common_xsd.xpath("//xsd:element[@name='%s']/@minOccurs" %
                                         table_name, namespaces=NAMESPACES)
        values += self._xsd_parser.xpath("//xsd:element[@name='%s']/@minOccurs" %
                                         table_name, namespaces=NAMESPACES)
        if len(values) > 0:
            # take the first element it should be better than nothing
            return int(values[0])
        else:
            return 0

    def get_maxoccurs(self, table_name):
        """ get maxOccurs value for table elements """
        values = []
        values +=  self._common_xsd.xpath("//xsd:element[@ref='%s']/@maxOccurs"
                                          % table_name, namespaces=NAMESPACES)
        values +=  self._xsd_parser.xpath("//xsd:element[@ref='%s']/@maxOccurs"
                                          % table_name, namespaces=NAMESPACES)
        values +=  self._common_xsd.xpath("//xsd:element[@name='%s']/@maxOccurs"
                                          % table_name, namespaces=NAMESPACES)
        values +=  self._xsd_parser.xpath("//xsd:element[@name='%s']/@maxOccurs"
                                          % table_name, namespaces=NAMESPACES)
        if len(values) > 0 and values[0] != "unbounded":
            # take the first element it should be better than nothing
            return int(values[0])
        elif len(values):
            # unbounded: return a high value
            return 100
        else:
            return 1

    def get_file_elements(self, elem):
        """ get elements or attributes with type: 'file' """
        values = []
        values +=  self._common_xsd.xpath("//xsd:%s[@type='file']/@name" %
                                          elem, namespaces=NAMESPACES)
        values +=  self._xsd_parser.xpath("//xsd:%s[@type='file']/@name" %
                                          elem, namespaces=NAMESPACES)
        return values

    def parse_numeric(self, base):
        """ parse a numeric xsd type """
        base = base.strip("xsd:")
        if base in ["byte",
                    "decimal"
                    "int",
                    "integer",
                    "long",
                    "negativeInteger",
                    "nonNegativeInteger",
                    "nonPositiveInteger",
                    "positiveInteger",
                    "short",
                    "unsignedLong",
                    "unsignedInt",
                    "unsignedShort",
                    "unsignedByte"]:
            min_val = None
            max_val = None
            step = 1
            # get the minimum or maximum value that can be infered
            if base in ["nonNegativeInteger",
                        "unsignedLong",
                        "unsignedInt",
                        "unsignedShort",
                        "unsignedByte"]:
                min_val = 0
            elif base == "nonPositiveInteger":
                max_val = 0
            elif base == "positiveInteger":
                min_val = 1
            elif base == "negativeInteger":
                max_val = -1
            elif base == "decimal":
                # arbitrarily fixed
                step = 0.01

            ret = {
                      "type": "numeric",
                      "step": step,
                   }
            if min_val is not None:
                ret["min"] = min_val
            if max_val is not None:
                ret["max"] = max_val
            return ret
        return None


    #### functions for XSLT transform ###

    def transform(self, xslt, write=True):
        """ Transform the configuration with XSLT """
        transf = etree.XSLT(etree.parse(xslt))
        new_xml = transf(self._tree)
        self._tree = new_xml
        if write:
            self.write()

if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print "Usage: %s xml_file xsd_file [xslt_file]" % sys.argv[0]
        sys.exit(1)
    print "use schema: %s and xml: %s" % (sys.argv[2],
                                          sys.argv[1])

    PARSER = XmlParser(sys.argv[1], sys.argv[2])

    for SECTION in PARSER.get_sections():
        print PARSER.get_name(SECTION)
        for KEY in PARSER.get_keys(SECTION):
            if not PARSER.is_table(KEY):
                print "\t%s=%s" % (PARSER.get_name(KEY),
                                   PARSER.get_value(KEY))
            else:
                print "\t%s" % PARSER.get_name(KEY)
                for ELT in PARSER.get_table_elements(KEY):
                    print "\t\t%s -> %s" % (PARSER.get_name(ELT),
                                            PARSER.get_element_content(ELT))
    try:
        if sys.argv[3] != "":
            PARSER.transform(sys.argv[3], False)
    except IndexError:
        pass

    sys.exit(0)





